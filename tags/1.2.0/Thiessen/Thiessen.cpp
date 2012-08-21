/**
 * OpenGeoDa TM, Copyright (C) 2011 by Luc Anselin - all rights reserved
 *
 * This file is part of OpenGeoDa.
 * 
 * OpenGeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenGeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/msgdlg.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "VorDataType.h"
#include "Thiessen.h"


Thiessen::Thiessen(int ndata, std::vector<double>& x,
				   std::vector<double>& y, myBox B) 
: PREC(52), gbgsp(0), gensp(0), shlsp(0),
m_GENSIZE(ndata), m_EDGESIZE(ndata * 3 + 10),
numgen(EMP), numedge(EMP), vpt_ptr(0), numsame(0) 
{

	myPolygon*	PolyIx;		// list of polygons with their edges
	myPoint*	P;			// list of all vertices
	int			nP;			// # of vertices
	LineSegment *V, *D;		// V: Voronoi edges, D: Delauny edges
	int			nEdges, edgesize;
	int			i;

	edgesize = ndata * 3 + 10;

	V = new LineSegment [edgesize];
	D = new LineSegment [edgesize];
	
	gen = NULL; v_pt = NULL; edge1 = NULL;
	edptr = NULL; bucket = NULL; slab = NULL;
	garbage = NULL; gennote = NULL; shelledge = NULL;

	gen = new generator [ndata];
    v_pt = new voronoi_pt [(ndata+3)*2];
	edge1 = new diredge [edgesize*4];
	edptr = new p_diredge [edgesize];
	bucket = new cell [ndata];
	slab  = new cell[ndata];
	garbage = new int [edgesize];
	gennote = new int [ndata];
	shelledge = new p_diredge [edgesize]; 

	PolyIx = new myPolygon [ndata];
	nPolygon = ndata;
	
	for (i=0;i < ndata;i++) {
		PolyIx[i].n=0;
		PolyIx[i].p = new int;
	}

	MakeQHull(ndata, x, y, &nEdges, V, D, B, PolyIx);
	P = new myPoint [nEdges * 3];
	CollectPoints(nEdges, V, P, &nP);
	// adding boundary points
	P[nP+0].x = B.p1.x; P[nP+0].y = B.p1.y; // BOTTOM LEFT
	P[nP+1].x = B.p2.x; P[nP+1].y = B.p1.y; // BOTOOM RIGHT
	P[nP+2].x = B.p2.x; P[nP+2].y = B.p2.y; // TOP RIGHT
	P[nP+3].x = B.p1.x; P[nP+3].y = B.p2.y; // TOP LEFT

	error = 0;
	Polygons = GeneratePolygons(ndata, V,nP, P,PolyIx,B);


	delete [] gen;
	gen = NULL;
	delete [] v_pt;
	v_pt = NULL;
	delete [] edge1;
	edge1 = NULL;
	delete [] bucket;
	bucket = NULL;
	delete [] shelledge;
	shelledge = NULL;
	delete [] gennote;
	gennote = NULL;
	delete [] slab;
	slab = NULL;
	delete [] D;
	D = NULL;
	delete [] V;
	V = NULL;
	delete [] PolyIx;
	PolyIx = NULL;
}

Thiessen::~Thiessen() 
{
//	kill(Polygons);
}

int Thiessen::ccw(int ga, int gb, int gc)
{//in
	double xa,xb,xc;
	double ya,yb,yc;
	double la,lb,lc;
	double xi,xj,xk,yi,yj,yk;
	double dxi,dxj;
	double dyi,dyj;
	double p1,p2;
	double res;
	if (ga<0 || gb<0 || gc<0 ||
	      ga>=numgen || gb>=numgen || gc>=numgen){
	    printf
		("\nerror : illegal generator call in fn. CCW");
	    printf("\n  %d %d %d\n",ga,gb,gc);
	    return(0);
	}
	xa=dp(gen[ga].x_co); ya=dp(gen[ga].y_co);
	xb=dp(gen[gb].x_co); yb=dp(gen[gb].y_co);
	xc=dp(gen[gc].x_co); yc=dp(gen[gc].y_co);

	la=(xb-xc)*(xb-xc)+(yb-yc)*(yb-yc);
	lb=(xc-xa)*(xc-xa)+(yc-ya)*(yc-ya);
	lc=(xa-xb)*(xa-xb)+(ya-yb)*(ya-yb);
	if (la>=lb){
	    if (la>=lc){ xk=xa; xi=xb; xj=xc; yk=ya; yi=yb; yj=yc;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}
	else{
	    if (lb>=lc){ xk=xb; xi=xc; xj=xa; yk=yb; yi=yc; yj=ya;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}

	dxi=dp(xi-xk); dyi=dp(yi-yk);
	dxj=dp(xj-xk); dyj=dp(yj-yk);
	p1=dp(dxi*dyj);
	p2=dp(dxj*dyi);
	res=dp(p1-p2);
	if (res>0) return(1);
	else return(0);
}


int Thiessen::incircle(int ga, int gb, int gc, int gd)
{
	double xa,xb,xc,xd;
	double ya,yb,yc,yd;
	double la,lb,lc;
	double xi,xj,xk,yi,yj,yk;
	double dxi,dxj,dxl;
	double dyi,dyj,dyl;
	double sqi,sqj;
	double j2,j3,j4;
	double jj2,jj3,jj4;
	double res;
	if (ga<0 || gb<0 || gc<0 || gd<0 ||
	      ga>=numgen || gb>=numgen || gc>=numgen || gd>=numgen){
	    return(0);
	}
	xa=dp(gen[ga].x_co); ya=dp(gen[ga].y_co);
	xb=dp(gen[gb].x_co); yb=dp(gen[gb].y_co);
	xc=dp(gen[gc].x_co); yc=dp(gen[gc].y_co);
	xd=dp(gen[gd].x_co); yd=dp(gen[gd].y_co);

	la=(xb-xc)*(xb-xc)+(yb-yc)*(yb-yc);
	lb=(xc-xa)*(xc-xa)+(yc-ya)*(yc-ya);
	lc=(xa-xb)*(xa-xb)+(ya-yb)*(ya-yb);
	if (la>=lb){
	    if (la>=lc){ xk=xa; xi=xb; xj=xc; yk=ya; yi=yb; yj=yc;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}
	else{
	    if (lb>=lc){ xk=xb; xi=xc; xj=xa; yk=yb; yi=yc; yj=ya;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}

	dxi=dp(xi-xk); dyi=dp(yi-yk);
	dxj=dp(xj-xk); dyj=dp(yj-yk);
	dxl=dp(xd-xk); dyl=dp(yd-yk);
	sqi=dp(dp(dp(dxi*dxi)+dp(dyi*dyi))/2.0);
	sqj=dp(dp(dp(dxj*dxj)+dp(dyj*dyj))/2.0);
	j2=dp(dp(dyi*sqj)-dp(dyj*sqi));
	j3=dp(dp(dxi*sqj)-dp(dxj*sqi));
	j4=dp(dp(dxi*dyj)-dp(dxj*dyi));
	jj2=dp(dp(2.0*j2)*dxl);
	jj3=dp(dp(2.0*j3)*dyl);
	jj4=dp(j4*dp(dp(dxl*dxl)+dp(dyl*dyl)));
	res=(-1.0)*dp(dp(jj2-jj3)+jj4);
	if (res>0) return(1);
	else return(0);
}


/*
    other global variables
 */

void Thiessen::ptr2n(diredge *ded, int *pe, int *pr)
{
	diredge *tmp;
	int i;
	tmp=edptr[ded->e];
	for(i=0;ded>tmp;i++,tmp++);
	*pe=ded->e;
	*pr=i;
}


/*
    Guibas & Stolfi's edge functions
 */
diredge* Thiessen::rot(diredge *ded)
{
	if (ded>=edptr[ded->e]+3)
	     return(edptr[ded->e]);
        else return(ded+1);
}


diredge* Thiessen::onext(diredge *ded)
{
        return(edptr[ded->next_e]+(ded->next_r));
}


diredge* Thiessen::sym(diredge *ded)
{
	if (ded>=edptr[ded->e]+2)
	     return(ded-2);
	else return(ded+2);
}


diredge* Thiessen::rotinv(diredge *ded)
{
	if (ded==edptr[ded->e])
	     return(ded+3);
	else return(ded-1);
}


diredge* Thiessen::oprev(diredge *ded)
{
	diredge *tmp;
	tmp=(ded>=edptr[ded->e]+3)? edptr[ded->e] : ded+1;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	tmp=(ded>=edptr[ded->e]+3)? edptr[ded->e] : ded+1;
	return(tmp);
}


diredge* Thiessen::dnext(diredge *ded)
{
	diredge *tmp;
	tmp=(ded>=edptr[ded->e]+2)? ded-2 : ded+2;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	tmp=(ded>=edptr[ded->e]+2)? ded-2 : ded+2;
	return(tmp);
}


diredge* Thiessen::dprev(diredge *ded)
{
	diredge *tmp;
	tmp=(ded==edptr[ded->e])? ded+3 : ded-1;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	tmp=(ded==edptr[ded->e])? ded+3 : ded-1;
	return(tmp);
}


diredge* Thiessen::lnext(diredge *ded)
{
	diredge *tmp;
	tmp=(ded==edptr[ded->e])? ded+3 : ded-1;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	tmp=(ded>=edptr[ded->e]+3)? edptr[ded->e] : ded+1;
	return(tmp);
}


diredge* Thiessen::lprev(diredge *ded)
{
	diredge *tmp;
	tmp=edptr[ded->next_e]+(ded->next_r);
	ded=(tmp>=edptr[tmp->e]+2)? tmp-2 : tmp+2;
	return(ded);
}


diredge* Thiessen::rnext(diredge *ded)
{
	diredge *tmp;
	tmp=(ded>=edptr[ded->e]+3)? edptr[ded->e] : ded+1;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	tmp=(ded==edptr[ded->e])? ded+3 : ded-1;
	return(tmp);
}


diredge* Thiessen::rprev(diredge *ded)
{
	diredge *tmp;
	tmp=(ded>=edptr[ded->e]+2)? ded-2 : ded+2;
	ded=edptr[tmp->next_e]+(tmp->next_r);
	return(ded);
}


/*
    stacks
 */
// stack GARBAGE


void Thiessen::gbgclear()
{
	gbgsp=0;
}

int Thiessen::gbgpush(int e)
{
	if (gbgsp<m_EDGESIZE)
	    return(garbage[gbgsp++]=e);
	else {
	    printf("\nerror : stack GARBAGE full\n");
	    gbgclear();
	    return(0);
	}
}


int Thiessen::gbgpop()
{
	if (gbgsp>0)
	    return(garbage[--gbgsp]);
	else {
	    printf("\nerror : stack GARBAGE empty\n");
	    gbgclear();
	    return(0);
	}
}



// stack SHELLEDGE

void Thiessen::shlclear()
{
	shlsp=0;
}


int Thiessen::shlpush(diredge *ded)
{
	if (shlsp<m_EDGESIZE) {
	    shelledge[shlsp]=ded;
	    shlsp++;
	    return(1);
	}
	else {
	    shlclear();
	    return(0);
	}
}


diredge* Thiessen::shlpop()
{
	if (shlsp>0)
	    return(shelledge[--shlsp]);
	else {
	    printf("\nerror : stack SHELLEDGE empty\n");
	    shlclear();
	    return(shelledge[shlsp]);
	}
}



// stack GENNOTE


void Thiessen::genclear()
{
	gensp=0;
}


int Thiessen::genpush(int gen)
{
	if (gensp<m_GENSIZE)
	    return(gennote[gensp++]=gen);
	else {
	    printf("\nerror : stack GENNOTE full\n");
	    genclear();
	    return(0);
	}
}


int Thiessen::genpop()
{
	if (gensp>0)
	    return(gennote[--gensp]);
	else {
	    printf("\nerror : stack GENNOTE empty\n");
	    genclear();
	    return(0);
	}
}



/*
    Guibas & Stolfi's 
	topological oprators
 */
int Thiessen::splice(diredge *a, diredge *b)
{//in: edptr
	diredge *alp, *bet;
	diredge *tmp;
	int t;
	int a_r, b_r, sub;

	tmp=edptr[a->e];	for(a_r=0; a>tmp; tmp++,a_r++);
	if (tmp!=a) a_r--;
	tmp=edptr[b->e];	for(b_r=0; b>tmp; tmp++,b_r++);
	if (tmp!=b) b_r--;
	if ((a_r-b_r)>=0)  sub=(a_r-b_r); else sub=(b_r-a_r);
	if ((sub % 2)==1) {
	    printf
	      ("\nerror : splice between primal edge and dual edge.");
            printf("\n  splice (%d %d)%d->%d & (%d %d)%d->%d",
		a->e,a_r,org(a),dest(a),b->e,b_r,org(b),dest(b));
	    return(0);
	}

	tmp=edptr[a->next_e]+(a->next_r);
	alp=(tmp>=edptr[tmp->e]+3)? edptr[tmp->e] : tmp+1;
	tmp=edptr[b->next_e]+(b->next_r);
	bet=(tmp>=edptr[tmp->e]+3)? edptr[tmp->e] : tmp+1;
	
	t=a->next_e; a->next_e=b->next_e; b->next_e=t;
	t=a->next_r; a->next_r=b->next_r; b->next_r=t;

	t=alp->next_e; alp->next_e=bet->next_e; bet->next_e=t;
	t=alp->next_r; alp->next_r=bet->next_r; bet->next_r=t;

	return(1);
}


diredge* Thiessen::makeedge()
{//in: edptr
	diredge *ded;
	int e;
	e=gbgpop();
	ded=edptr[e];
	(ded  )->origin= EMP; (ded+2)->origin= EMP;
	(ded  )->next_e=e; (ded  )->next_r=0;
	(ded+1)->next_e=e; (ded+1)->next_r=3;
	(ded+2)->next_e=e; (ded+2)->next_r=2;
	(ded+3)->next_e=e; (ded+3)->next_r=1;
	ded->shell= EMP; (ded+2)->shell= EMP;
	return(ded);
}


void Thiessen::deleteedge(diredge *ded)
{
	splice(ded,oprev(ded));
	splice(sym(ded),oprev(sym(ded)));
	gbgpush(ded->e);
}


diredge* Thiessen::connect(diredge *d_a, diredge *d_b)
{//free
	diredge *ded;
	ded=makeedge();
	org(ded)=dest(d_a);
	dest(ded)=org(d_b);
	splice(ded,lnext(d_a));
	splice(sym(ded),d_b);
	return(ded);
}


/*
    find starting line 
	(lower or left common tangent)
 */
void Thiessen::normstl(diredge *first1,diredge *first2, diredge *(*pst1),
					   diredge *(*pst2))
{
	int ch1,ch2,latch1,latch2;
	 diredge *st1,*st2;

	st1=first1; st2=first2;
	latch1=0; latch2=0;
	do{
	    /*
	        move S's side end
	     */
	    ch2=0;
	    if (latch2==0 && dest(oprev(st2))!= WALL){
	        if (ccw(org(st1),dest(oprev(st2)),org(st2))){
	            st2=rnext(st2);
	            ch2=1;
	            while(dest(oprev(st2))!= WALL){
	                if (ccw(org(st1),dest(oprev(st2)),org(st2)))
	                    st2=rnext(st2);
			else break;
		    }
		    goto next1;
		}
	    }
	    while(dest(st2)!= WALL){
	        if (ccw(org(st1),dest(st2),org(st2))){
	            st2=rprev(st2);
	            ch2=1;
	            latch2=1;
		}
		else break;
	    }
next1:;

	    /*
	        move F's side end
	     */
	    ch1=0;
	    if (latch1==0 && dest(onext(st1))!= WALL){
	        if (ccw(org(st1),dest(onext(st1)),org(st2))){
	            st1=lprev(st1);
	            ch1=1;
	            while(dest(onext(st1))!= WALL){
	                if (ccw(org(st1),dest(onext(st1)),org(st2)))
	                    st1=lprev(st1);
			else break;
		    }
		    goto next2;
		}
	    }
	    while(dest(st1)!= WALL){
	        if (ccw(org(st1),dest(st1),org(st2))){
	           st1=lnext(st1);
	           ch1=1;
	           latch1=1;
		}
		else break;
	    }
next2:;
	}while(ch1!=0 || ch2!=0);
	*pst1=st1; *pst2=st2;
}


diredge* Thiessen::f1stl(diredge *first1,diredge *first2)
{
	int ch2,latch2;
	 diredge *st1,*st2;
	
	st1=first1; st2=first2;
	latch2=0;
	do{
	    /*
	        move S's side end
	     */
	    ch2=0;
	    if (latch2==0 && dest(oprev(st2))!= WALL){
	        if (ccw(org(st1),dest(oprev(st2)),org(st2))){
	            st2=rnext(st2);
	            ch2=1;
	            while(dest(oprev(st2))!= WALL){
	                if (ccw(org(st1),dest(oprev(st2)),org(st2)))
	                    st2=rnext(st2);
			else break;
		    }
		    goto next1;
		}
	    }
	    while(dest(st2)!= WALL){
	        if (ccw(org(st1),dest(st2),org(st2))){
	            st2=rprev(st2);
	            ch2=1;
	            latch2=1;
	        }
		else break;
	    }
next1:;
	}while(ch2!=0);
	return(st2);
}


diredge* Thiessen::s1stl(diredge *first1, diredge *first2)
{
	int ch1,latch1;
	 diredge *st1,*st2;
	
	st1=first1; st2=first2;
	latch1=0;
	do{
	    /*
	        move F's side end
	     */
	    ch1=0;
	    if (latch1==0 && dest(onext(st1))!= WALL){
	        if (ccw(org(st1),dest(onext(st1)),org(st2))){
	            st1=lprev(st1);
	            ch1=1;
	            while(dest(onext(st1))!= WALL){
	                if (ccw(org(st1),dest(onext(st1)),org(st2)))
	                    st1=lprev(st1);
			else break;
		    }
		    goto next1;
		}
	    }
	    while(dest(st1)!= WALL){
	        if (ccw(org(st1),dest(st1),org(st2))){
	            st1=lnext(st1);
	            ch1=1;
	            latch1=1;
		}
		else break;
	    }
next1:;
	}while(ch1!=0);
	return(st1);
}


void Thiessen::parallel(diredge *basel,diredge *cand1,diredge *cand2,int *ppal1, int *ppal2)
{
	 diredge *tmp;

	*ppal1= *ppal2=0;
	if (dest(cand1)>=0) {
		if (gen[dest(cand1)].note == PEND){
			for(tmp=dprev(cand1);tmp!=cand1 && org(tmp)!=org(basel);
	    							  tmp=dprev(tmp));
			if (org(tmp)==org(basel)) *ppal1=1;
		}
	}
	else error++;
	if (dest(cand2)>=0) {
		if (*ppal1!=1 && gen[dest(cand2)].note == PEND){
			for(tmp=dnext(cand2);tmp!=cand2 && org(tmp)!=dest(basel);
	    							  tmp=dnext(tmp));
			if (org(tmp)==dest(basel)) *ppal2=1;
		}
	}
	else error++;
	if (error>1) {
		// Error, Array out of bound.
		return;
	}
}


/*
    low level merge routine
 */
diredge* Thiessen::normmerge(diredge *basel,diredge *tm1,diredge *tm2)
{
	diredge *cand1,*cand2,*tmp;
	int flag1,flag2,winner;
	int pal1,pal2;
	
	for(;;){
	    cand1=onext(sym(basel));
	    cand2=oprev(basel);
	    
	    do{
		flag1=0;
		if(cand1 != tm1){
		    flag1=1;
		    if(ccw(dest(basel),org(basel),dest(cand1))){
			flag1=2;
			if (cand1->shell != SHELL){
			    flag1=3;
			    if (incircle
		(dest(basel),org(basel),dest(cand1),dest(onext(cand1)))){

				flag1=4;
		    		(sym(onext(cand1)))->shell = SHELL;
		    		shlpush(sym(onext(cand1)));
				(oprev(sym(cand1)))->shell = SHELL;
				shlpush(oprev(sym(cand1)));
		    		tmp=onext(cand1);
		    		deleteedge(cand1);
		    		cand1=tmp;
			    }
		        }
		    }
		}
	    }while(flag1==4);


	    do{
		flag2=0;
		if(cand2 != tm2){
		    flag2=1;
		    if(ccw(dest(basel),org(basel),dest(cand2))){
			flag2=2;
			if (cand2->shell != SHELL){
			    flag2=3;
			    if (incircle
		(dest(basel),org(basel),dest(cand2),dest(oprev(cand2)))){

				flag2=4;
		    		(sym(oprev(cand2)))->shell = SHELL;
		    		shlpush(sym(oprev(cand2)));
				(onext(sym(cand2)))->shell = SHELL;
				shlpush(onext(sym(cand2)));
		    		tmp=oprev(cand2);
		    		deleteedge(cand2);
		    		cand2=tmp;
			    }
		        }
		    }
		}
	    }while(flag2==4);
	    
	    
	    if(cand1==tm1 && cand2==tm2) return(basel);

	    if (cand1!=tm1 && cand2!=tm2) {
		if (incircle(dest(cand1),dest(basel),org(basel),dest(cand2)))
		    winner=2;
	        else winner=1;
	    }

	    if (cand1==tm1) winner=2;
	    if (cand2==tm2) winner=1;

	    parallel(basel,cand1,cand2,&pal1,&pal2);

	    if ((winner==1 && pal1==1) || (winner==2 && pal2==1)){
		if ((cand1==tm1 && winner==2) || 
		    (cand2==tm2 && winner==1)) return(basel);
		else{
		    if (winner==1) winner=2;
		    if (winner==2) winner=1;
		}
	    }

	    if (winner==2){
	     /*   sym(cand2)->shell = SHELL; shlpush(sym(cand2));
*/
	        gen[org(basel)].note = PEND; genpush(org(basel));
	        basel=connect(cand2,sym(basel));
	    }
	    else {
	/*	sym(cand1)->shell = SHELL; shlpush(sym(cand1));
*/
		gen[dest(basel)].note = PEND; genpush(dest(basel));
	        basel=connect(sym(basel),sym(cand1));
	    }
	}
}


diredge* Thiessen::f1merge(diredge *basel,diredge *tm2)
{
	int g;
	diredge *cand2,*tmp;

	genpush(org(basel));
	tmp=oprev(basel);
	while(tmp!=tm2){
	    gen[dest(tmp)].note = FENDSM;
	    genpush(dest(tmp));
	    tmp=lnext(tmp);
	}
	for(;;){
	    cand2=oprev(basel);
	    if (cand2==tm2) break;
	    if (gen[dest(cand2)].note== PENDSM) break;
	    while(cand2->shell!= SHELL){
	        g=dest(oprev(cand2));
		if (g<0 || g>=numgen) 
	 printf("\nerror : illegal generator call in fn. F1MERGE.\n");
	        if (gen[g].note== FENDSM || gen[g].note== PENDSM) break;
		if (incircle(dest(basel),org(basel),dest(cand2),
	                 dest(oprev(cand2)))){
	            (rprev(cand2))->shell = SHELL;
	            shlpush(rprev(cand2));
	            tmp=oprev(cand2);
	            deleteedge(cand2);
	            cand2=tmp;
		    gen[dest(cand2)].note = FENDSM;
		    genpush(dest(cand2));
		}
		else break;
	    }
	    gen[org(basel)].note = PENDSM;
	    (sym(cand2))->shell = SHELL;
	    shlpush(sym(cand2));
	    basel=connect(cand2,sym(basel));
	}
	return(basel);
}


diredge* Thiessen::s1merge(diredge *basel,diredge *tm1)
{
	int g;
	diredge *cand1,*tmp;

	genpush(dest(basel));
	tmp=rprev(basel);
	while(tmp!=tm1){
	    gen[dest(tmp)].note = FENDSM;
	    genpush(dest(tmp));
	    tmp=rprev(tmp);
	}
	for(;;){
	    cand1=onext(sym(basel));
	    if (cand1==tm1) break;
	    if (gen[dest(cand1)].note== PENDSM) break;
	    while(cand1->shell!= SHELL){
		g=dest(onext(cand1));
		if (g<0 || g>=numgen)
	 printf("\nerror : illegal generator call in fn. S1MERGE.\n");
	        if (gen[g].note== FENDSM || gen[g].note== PENDSM) break;
		if (incircle(dest(basel),org(basel),dest(cand1),
	                 dest(onext(cand1)))){
	            (lnext(cand1))->shell = SHELL;
	            shlpush(lnext(cand1));
	            tmp=onext(cand1);
	            deleteedge(cand1);
	            cand1=tmp;
		    gen[dest(cand1)].note = FENDSM;
		    genpush(dest(cand1));
		}
		else break;
	    }
	    gen[dest(basel)].note = PENDSM;
	    (sym(cand1))->shell = SHELL;
	    shlpush(sym(cand1));
	    basel=connect(sym(basel),sym(cand1));
	}
	return(basel);
}

/*
     comparison tools
	for coordinates of generators
 */
short Thiessen::higher(int gen1, int gen2)
{
	if (gen[gen1].y_co>gen[gen2].y_co) return(1);
	else{
	    if (gen[gen1].y_co == gen[gen2].y_co &&
		  gen[gen1].x_co > gen[gen2].x_co) 
		return(1);
	    else return(0);
	}
}


short Thiessen::righter(int gen1, int gen2)
{
	if (gen[gen1].x_co>gen[gen2].x_co) return(1);
	else{
	    if (gen[gen1].x_co == gen[gen2].x_co){
		if (gen[gen1].y_co > gen[gen2].y_co)
		    return(1);
		else{
		    if (gen[gen1].y_co == gen[gen2].y_co){
			return(SAME);
		    }
		    else return(0);
		}
	    }
	    else return(0);
	}
}


short Thiessen::scrighter(int gen1, int gen2)
{
	if (gen[gen1].x_co>gen[gen2].x_co) return(1);
	else {
	    if (gen[gen1].x_co == gen[gen2].x_co){
	        if (gen[gen1].y_co > gen[gen2].y_co)
		    return(1);
		else{
		    if (gen[gen1].y_co == gen[gen2].y_co){
		        gen[gen1].note= SAME;
		        gen[gen2].note= SAME;
			return(SAME);
		    }
		    else return(0);
		}
	    }
	    else return(0);
	}
}



/*
    high level merge routine
 */
void Thiessen::horizontalmerge(cell *tri1, cell *tri2, cell *tri0)
{
	int g;
	int ng1,ng2;
	int tinf;
	int e1,e2,e3,e4,r1,r2,r3,r4;
	short locked1,locked2,moved1,moved2;
	 diredge *be1,*be2,*re1,*re2,*te1,*te2,*le1,*le2;
	 diredge *be,*te,*bed,*ted;
	 diredge *st1,*st2,*tm1,*tm2;
	 diredge *startF,*startS,*endF,*endS;
	 diredge *basel,*tmp;
	
	ng1=(tri1->ng); ng2=(tri2->ng);

    // case0 : ng1==0 or ng2==0
	if (ng1==0){
	    tri0->ng=tri2->ng;
	    tri0->be_e=tri2->be_e; tri0->be_r=tri2->be_r;
	    tri0->re_e=tri2->re_e; tri0->re_r=tri2->re_r;
	    tri0->te_e=tri2->te_e; tri0->te_r=tri2->te_r;
	    tri0->le_e=tri2->le_e; tri0->le_r=tri2->le_r;
	    return;
	}
	if (ng2==0){
	    tri0->ng=tri1->ng;
	    tri0->be_e=tri1->be_e; tri0->be_r=tri1->be_r;
	    tri0->re_e=tri1->re_e; tri0->re_r=tri1->re_r;
	    tri0->te_e=tri1->te_e; tri0->te_r=tri1->te_r;
	    tri0->le_e=tri1->le_e; tri0->le_r=tri1->le_r;
	    return;
	}

	
    // case1 : ng1==1 and ng2==1
	if (ng1==1 && ng2==1){
            be1=n2ptr(tri1->be_e, tri1->be_r);
            be2=n2ptr(tri2->be_e, tri2->be_r);

            basel=connect(sym(be2),be1);
	    deleteedge(be1); deleteedge(be2);

	    ptr2n(basel,&e1,&r1); ptr2n(sym(basel),&e2,&r2);
	    tri0->le_e=e2; tri0->le_r=r2;
	    tri0->re_e=e1; tri0->re_r=r1;
	    if (higher(dest(basel),org(basel))){
		tri0->be_e=e1; tri0->be_r=r1;
		tri0->te_e=e2; tri0->te_r=r2;
	    }
	    else{
		tri0->be_e=e2; tri0->be_r=r2;
		tri0->te_e=e1; tri0->te_r=r1;
	    }
	    tri0->ng=2;
	    return;
	}

	
    // case2 : ng1==1 and ng2>=2
        if (ng1==1 && ng2>=2){
            be2=n2ptr(tri2->be_e, tri2->be_r);
	    te2=n2ptr(tri2->te_e, tri2->te_r);
            le2=n2ptr(tri2->le_e, tri2->le_r);
	    re2=n2ptr(tri2->re_e, tri2->re_r);
            be1=n2ptr(tri1->be_e, tri1->be_r);
	    re1=makeedge(); org(re1)=org(be1); splice(be1,re1);
	    te1=makeedge(); org(te1)=org(be1); splice(re1,te1);
	    le1=makeedge(); org(le1)=org(be1); splice(te1,le1);
	    
	    be2->shell = SHELL; shlpush(be2);
            for(tmp=rprev(be2);tmp!=be2;tmp=rprev(tmp)){
                tmp->shell = SHELL; shlpush(tmp);
            }

	    tmp=makeedge(); org(tmp)=org(be2);
	        dest(tmp)= WALL; splice(tmp,oprev(be2)); be2=tmp;
	    tmp=makeedge(); org(tmp)=org(re2);
	        dest(tmp)= WALL; splice(tmp,oprev(re2)); re2=tmp;
	    tmp=makeedge(); org(tmp)=org(te2);
	        dest(tmp)= WALL; splice(tmp,oprev(te2)); te2=tmp;
	    tmp=makeedge(); org(tmp)=org(le2);
	        dest(tmp)= WALL; splice(tmp,oprev(le2)); le2=tmp;
	    if (oprev(le2)==be2){ tmp=le2; le2=be2; be2=tmp;}

	    if (higher(org(te1),org(te2))){
	        te=te1; ted=te2; te2=oprev(te2);
		tinf=1;
	    }
	    else{
	        te=te2; ted=te1;
		tinf=0;
	    }
	    if (higher(org(be1),org(be2))){
	        be=be2; bed=be1;
	    }
	    else{
	        be=be1; bed=be2; be2=onext(be2);
	    }

	    if (onext(te)==le2){
	        tmp=makeedge();
	        dest(tmp)= WALL;
	        splice(  sym(onext(le2)),tmp);
	        st2=f1stl(be1,be2);
	        deleteedge(tmp);
	    }
	    else st2=f1stl(be1,be2);
	    
	    basel=connect(sym(st2),be1);
	    deleteedge(bed);
	    if (tinf==1){
	        if (oprev(basel)==le2 && oprev(le2)==ted){
	            te2=lnext(te2);
		    splice(ted,oprev(ted));
		    splice(ted,te2);
		}
	    }
	    else {
	        if (oprev(basel)==le2){
	            splice(le2,oprev(le2));
	            splice(le2,lnext(oprev(basel)));
	        }
	    }
            tm2=s1stl(te2,te1);
	    deleteedge(re1); deleteedge(le2);
	    deleteedge(ted);
	    basel=f1merge(basel,tm2);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }


	    if (tinf==1){
		tmp=re2; re2=onext(re2); deleteedge(tmp);
	        tmp=be;   be=onext( be); deleteedge(tmp);
	        tmp=le1; le1=onext(le1); deleteedge(tmp);
	        tmp=te;   te=onext( te); deleteedge(tmp);
	    }
	    else {
	        tmp=te;   te=onext( te); deleteedge(tmp);
	        tmp=re2; re2=onext(re2); deleteedge(tmp);
		tmp=be;   be=onext( be); deleteedge(tmp);
		tmp=le1; le1=onext(le1); deleteedge(tmp);
	    }
	    
	    ptr2n( be,&e1,&r1);
	    ptr2n(re2,&e2,&r2);
	    ptr2n( te,&e3,&r3);
	    ptr2n(le1,&e4,&r4);

	    tri0->be_e=e1; tri0->be_r=r1;
	    tri0->re_e=e2; tri0->re_r=r2;
	    tri0->te_e=e3; tri0->te_r=r3;
	    tri0->le_e=e4; tri0->le_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}


            
    // case3 : ng1>=2 and ng2==1
        if (ng1>=2 && ng2==1){
            be1=n2ptr(tri1->be_e, tri1->be_r);
	    te1=n2ptr(tri1->te_e, tri1->te_r);
            le1=n2ptr(tri1->le_e, tri1->le_r);
	    re1=n2ptr(tri1->re_e, tri1->re_r);
            be2=n2ptr(tri2->be_e, tri2->be_r);
	    re2=makeedge(); org(re2)=org(be2); splice(be2,re2);
	    te2=makeedge(); org(te2)=org(be2); splice(re2,te2);
	    le2=makeedge(); org(le2)=org(be2); splice(te2,le2);

	    (sym(be1))->shell = SHELL; shlpush(sym(be1));
	    for(tmp=oprev(be1);sym(tmp)!=be1;tmp=lnext(tmp)){
	        tmp->shell = SHELL; shlpush(tmp);
	    }
	    
	    tmp=makeedge(); org(tmp)=org(be1);
	        dest(tmp)= WALL; splice(tmp,oprev(be1)); be1=tmp;
	    tmp=makeedge(); org(tmp)=org(re1);
	        dest(tmp)= WALL; splice(tmp,oprev(re1)); re1=tmp;
	    tmp=makeedge(); org(tmp)=org(te1);
	        dest(tmp)= WALL; splice(tmp,oprev(te1)); te1=tmp;
	    tmp=makeedge(); org(tmp)=org(le1);
	        dest(tmp)= WALL; splice(tmp,oprev(le1)); le1=tmp;
	    if (oprev(le1)==be1){ tmp=le1; le1=be1; be1=tmp;}

	    if (higher(org(te1),org(te2))){
	        te=te1; ted=te2;
	        tinf=1;
	    }
	    else{
	        te=te2; ted=te1; te1=onext(te1);
	        tinf=0;
	    }
	    if (higher(org(be1),org(be2))){
	        be=be2; bed=be1; be1=oprev(be1);
	    }
	    else{
	        be=be1; bed=be2;
	    }

	    if (oprev(te)==re1){
	        tmp=makeedge();
	        dest(tmp)= WALL;
	        splice(lnext(oprev(re1)),tmp);
	        st1=s1stl(be1,be2);
	        deleteedge(tmp);
	    }
	    else st1=s1stl(be1,be2);

	    basel=connect(sym(be2),st1);
	    deleteedge(bed);
	    if (tinf==1){
	        if (rprev(basel)==re1){
	            splice(re1,oprev(re1));
	            splice(re1,dnext(basel));
	        }
	    }
	    else {
	        if (rprev(basel)==re1 && onext(re1)==ted){
	            te1=rprev(te1);
		    splice(ted,oprev(ted));
		    splice(ted,oprev(te1));
		}
	    }
            tm1=f1stl(te2,te1);
	    deleteedge(re1); deleteedge(le2);
	    deleteedge(ted);
	    basel=s1merge(basel,tm1);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }

	    if (tinf==1){
		tmp=re2; re2=onext(re2); deleteedge(tmp);
	        tmp=be;   be=onext( be); deleteedge(tmp);
	        tmp=le1; le1=onext(le1); deleteedge(tmp);
	        tmp=te;   te=onext( te); deleteedge(tmp);
	    }
	    else {
	        tmp=te;   te=onext( te); deleteedge(tmp);
	        tmp=re2; re2=onext(re2); deleteedge(tmp);
		tmp=be;   be=onext( be); deleteedge(tmp);
		tmp=le1; le1=onext(le1); deleteedge(tmp);
	    }
	    
	    ptr2n( be,&e1,&r1);
	    ptr2n(re2,&e2,&r2);
	    ptr2n( te,&e3,&r3);
	    ptr2n(le1,&e4,&r4);

	    tri0->be_e=e1; tri0->be_r=r1;
	    tri0->re_e=e2; tri0->re_r=r2;
	    tri0->te_e=e3; tri0->te_r=r3;
	    tri0->le_e=e4; tri0->le_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}


    // case4 : ng1>=2 and ng2>=2
        if (ng1>=2 && ng2>=2){
            be1=n2ptr(tri1->be_e, tri1->be_r);
            be2=n2ptr(tri2->be_e, tri2->be_r);
            te1=n2ptr(tri1->te_e, tri1->te_r);
	    te2=n2ptr(tri2->te_e, tri2->te_r);
            le1=n2ptr(tri1->le_e, tri1->le_r);
            le2=n2ptr(tri2->le_e, tri2->le_r);
            re1=n2ptr(tri1->re_e, tri1->re_r);
	    re2=n2ptr(tri2->re_e, tri2->re_r);

	    tmp=sym(be1);
	    do{
		tmp->shell = SHELL; shlpush(tmp);
		tmp=lnext(tmp);
	    }while(tmp!=sym(be1));
	    tmp=be2;
	    do{
		tmp->shell = SHELL; shlpush(tmp);
		tmp=rprev(tmp);
	    }while(tmp!=be2);

	    tmp=makeedge(); org(tmp)=org(be1);
	        dest(tmp)= WALL; splice(tmp,oprev(be1)); be1=tmp;
	    tmp=makeedge(); org(tmp)=org(re1);
	        dest(tmp)= WALL; splice(tmp,oprev(re1)); re1=tmp;
	    tmp=makeedge(); org(tmp)=org(te1);
	        dest(tmp)= WALL; splice(tmp,oprev(te1)); te1=tmp;
	    tmp=makeedge(); org(tmp)=org(le1);
	        dest(tmp)= WALL; splice(tmp,oprev(le1)); le1=tmp;
	    if (oprev(le1)==be1){ tmp=le1; le1=be1; be1=tmp;}

	    tmp=makeedge(); org(tmp)=org(be2);
	        dest(tmp)= WALL; splice(tmp,oprev(be2)); be2=tmp;
	    tmp=makeedge(); org(tmp)=org(re2);
	        dest(tmp)= WALL; splice(tmp,oprev(re2)); re2=tmp;
	    tmp=makeedge(); org(tmp)=org(te2);
	        dest(tmp)= WALL; splice(tmp,oprev(te2)); te2=tmp;
	    tmp=makeedge(); org(tmp)=org(le2);
	        dest(tmp)= WALL; splice(tmp,oprev(le2)); le2=tmp;
	    if (oprev(le2)==be2){ tmp=le2; le2=be2; be2=tmp;}

	    if (higher(org(te1),org(te2))){
	        te=te1; ted=te2;
	        tinf=1;
	    }
	    else{
	        te=te2; ted=te1;
	        tinf=0;
	    }
	    if (higher(org(be1),org(be2))){
	        be=be2; bed=be1;
	    }
	    else{
	        be=be1; bed=be2;
	    }

	/* H_StartingLine */
	    if (higher(org(be1), org(be2))){
		startF=oprev(be1);
		endF=le1;
		startS=onext(le2);
		endS=be2;
		st1=oprev(be1);
		st2=be2;
	    }
	    else{
		startF=oprev(re1);
		endF=be1;
		startS=onext(be2);
		endS=re2;
		st1=be1;
		st2=onext(be2);
	    }
	    locked1=locked2=0;
	    do{
		moved1=0;
		if (locked1==0) {
		    while (st1!=startF)
			if (ccw(org(st1), org(lprev(st1)), org(st2))) {
			    st1=lprev(st1); moved1=1;
			}
			else break;
		}
		if (moved1==0) {
		    while (st1!=endF)
			if (ccw(org(st1), dest(st1), org(st2))) {
			    st1=lnext(st1); moved1=1; locked1=1;
			}
			else break;
		}
		moved2=0;
		if (locked2==0) {
		    while (st2!=startS)
			if (ccw(org(rnext(st2)), org(st2), org(st1))) {
			    st2=rnext(st2); moved2=1;
			}
			else break;
		}
		if (moved2==0) {
		    while (st2!=endS)
			if (ccw(dest(st2), org(st2), org(st1))) {
			    st2=rprev(st2); moved2=1; locked2=1;
			}
			else break;
		}
	    }while (moved1==1 || moved2==1);

	/* H_TerminalLine */
	    if (higher(org(te2), org(te1))){
		startF=onext(te1);
		endF=le1;
		startS=oprev(le2);
		endS=te2;
		tm1=onext(te1);
		tm2=te2;
	    }
	    else{
		startF=onext(re1);
		endF=te1;
		startS=oprev(te2);
		endS=re2;
		tm1=te1;
		tm2=oprev(te2);
	    }
	    locked1=locked2=0;
	    do{
		moved1=0;
		if (locked1==0) {
		    while (tm1!=startF)
			if (ccw(org(tm1), org(tm2), org(rnext(tm1)))) {
			    tm1=rnext(tm1); moved1=1;
			}
			else break;
		}
		if (moved1==0) {
		    while (tm1!=endF)
			if (ccw(org(tm1), org(tm2), dest(tm1))) {
			    tm1=rprev(tm1); moved1=1; locked1=1;
			}
			else break;
		}
		moved2=0;
		if (locked2==0) {
		    while (tm2!=startS)
			if (ccw(org(tm2), org(lprev(tm2)), org(tm1))) {
			    tm2=lprev(tm2); moved2=1;
			}
			else break;
		}
		if (moved2==0) {
		    while (tm2!=endS)
			if (ccw(org(tm2), dest(tm2), org(tm1))) {
			    tm2=lnext(tm2); moved2=1; locked2=1;
			}
			else break;
		}
	    }while (moved1==1 || moved2==1);

	    deleteedge(re1); deleteedge(le2);
	    deleteedge(bed); deleteedge(ted);

	    basel=connect(sym(st2),st1);

	    basel=normmerge(basel,tm1,tm2);
	    if (oprev(basel)!=tm2) basel=f1merge(basel,tm2);
	    else if (rprev(basel)!=tm1) basel=s1merge(basel,tm1);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }


	    if (tinf==1){
		tmp=re2; re2=onext(re2); deleteedge(tmp);
	        tmp=be;   be=onext( be); deleteedge(tmp);
	        tmp=le1; le1=onext(le1); deleteedge(tmp);
	        tmp=te;   te=onext( te); deleteedge(tmp);
	    }
	    else {
	        tmp=te;   te=onext( te); deleteedge(tmp);
	        tmp=re2; re2=onext(re2); deleteedge(tmp);
		tmp=be;   be=onext( be); deleteedge(tmp);
		tmp=le1; le1=onext(le1); deleteedge(tmp);
	    }
	    
	    ptr2n( be,&e1,&r1);
	    ptr2n(re2,&e2,&r2);
	    ptr2n( te,&e3,&r3);
	    ptr2n(le1,&e4,&r4);

	    tri0->be_e=e1; tri0->be_r=r1;
	    tri0->re_e=e2; tri0->re_r=r2;
	    tri0->te_e=e3; tri0->te_r=r3;
	    tri0->le_e=e4; tri0->le_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}
}



/*
    high level merge routine
        (continued)
 */
void Thiessen::verticalmerge(cell *tri1,cell *tri2,cell *tri0)
{
	int g;
	int ng1,ng2;
	int rinf;
	int e1,e2,e3,e4,r1,r2,r3,r4;
	 diredge *le1,*le2,*be1,*be2,*re1,*re2,*te1,*te2;
	 diredge *le,*re,*led,*red;
	 diredge *st1,*st2,*tm1,*tm2;
	 diredge *basel,*tmp;
	
	ng1=(tri1->ng); ng2=(tri2->ng);

    // case0 : ng1==0 or ng2==0
	if (ng1==0){
	    tri0->ng=tri2->ng;
	    tri0->le_e=tri2->le_e; tri0->le_r=tri2->le_r;
	    tri0->be_e=tri2->be_e; tri0->be_r=tri2->be_r;
	    tri0->re_e=tri2->re_e; tri0->re_r=tri2->re_r;
	    tri0->te_e=tri2->te_e; tri0->te_r=tri2->te_r;
	    return;
	}
	if (ng2==0){
	    tri0->ng=tri1->ng;
	    tri0->le_e=tri1->le_e; tri0->le_r=tri1->le_r;
	    tri0->be_e=tri1->be_e; tri0->be_r=tri1->be_r;
	    tri0->re_e=tri1->re_e; tri0->re_r=tri1->re_r;
	    tri0->te_e=tri1->te_e; tri0->te_r=tri1->te_r;
	    return;
	}

	
    // case1 : ng1==1 and ng2==1
	if (ng1==1 && ng2==1){
            le1=n2ptr(tri1->le_e, tri1->le_r);
            le2=n2ptr(tri2->le_e, tri2->le_r);

            basel=connect(sym(le2),le1);
	    deleteedge(le1); deleteedge(le2);

	    ptr2n(basel,&e1,&r1); ptr2n(sym(basel),&e2,&r2);
	    tri0->te_e=e2; tri0->te_r=r2;
	    tri0->be_e=e1; tri0->be_r=r1;
	    if (righter(dest(basel),org(basel))){
		tri0->le_e=e1; tri0->le_r=r1;
		tri0->re_e=e2; tri0->re_r=r2;
	    }
	    else{
		tri0->le_e=e2; tri0->le_r=r2;
		tri0->re_e=e1; tri0->re_r=r1;
	    }
	    tri0->ng=2;
	    return;
	}

	
    // case2 : ng1==1 and ng2>=2
        if (ng1==1 && ng2>=2){
            le2=n2ptr(tri2->le_e, tri2->le_r);
	    re2=n2ptr(tri2->re_e, tri2->re_r);
            te2=n2ptr(tri2->te_e, tri2->te_r);
	    be2=n2ptr(tri2->be_e, tri2->be_r);
            le1=n2ptr(tri1->le_e, tri1->le_r);
	    be1=makeedge(); org(be1)=org(le1); splice(le1,be1);
	    re1=makeedge(); org(re1)=org(le1); splice(be1,re1);
	    te1=makeedge(); org(te1)=org(le1); splice(re1,te1);
	    
	    le2->shell = SHELL; shlpush(le2);
            for(tmp=rprev(le2);tmp!=le2;tmp=rprev(tmp)){
                tmp->shell = SHELL; shlpush(tmp);
            }

	    tmp=makeedge(); org(tmp)=org(le2);
	        dest(tmp)= WALL; splice(tmp,oprev(le2)); le2=tmp;
	    tmp=makeedge(); org(tmp)=org(be2);
	        dest(tmp)= WALL; splice(tmp,oprev(be2)); be2=tmp;
	    tmp=makeedge(); org(tmp)=org(re2);
	        dest(tmp)= WALL; splice(tmp,oprev(re2)); re2=tmp;
	    tmp=makeedge(); org(tmp)=org(te2);
	        dest(tmp)= WALL; splice(tmp,oprev(te2)); te2=tmp;
	    if (oprev(te2)==le2){ tmp=te2; te2=le2; le2=tmp;}

	    if (righter(org(re1),org(re2))){
	        re=re1; red=re2; re2=oprev(re2);
		rinf=1;
	    }
	    else{
	        re=re2; red=re1;
		rinf=0;
	    }
	    if (righter(org(le1),org(le2))){
	        le=le2; led=le1;
	    }
	    else{
	        le=le1; led=le2; le2=onext(le2);
	    }

	    if (onext(re)==te2){
	        tmp=makeedge();
	        dest(tmp)= WALL;
	        splice(  sym(onext(te2)),tmp);
	        st2=f1stl(le1,le2);
	        deleteedge(tmp);
	    }
	    else st2=f1stl(le1,le2);
	    
	    basel=connect(sym(st2),le1);
	    deleteedge(led);
	    if (rinf==1){
	        if (oprev(basel)==te2 && oprev(te2)==red){
	            re2=lnext(re2);
		    splice(red,oprev(red));
		    splice(red,re2);
		}
	    }
	    else {
	        if (oprev(basel)==te2){
	            splice(te2,oprev(te2));
	            splice(te2,lnext(oprev(basel)));
	        }
	    }
            tm2=s1stl(re2,re1);
	    deleteedge(be1); deleteedge(te2);
	    deleteedge(red);
	    basel=f1merge(basel,tm2);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }


	    if (rinf==1){
		tmp=be2; be2=onext(be2); deleteedge(tmp);
	        tmp=le;   le=onext( le); deleteedge(tmp);
	        tmp=te1; te1=onext(te1); deleteedge(tmp);
	        tmp=re;   re=onext( re); deleteedge(tmp);
	    }
	    else {
	        tmp=re;   re=onext( re); deleteedge(tmp);
	        tmp=be2; be2=onext(be2); deleteedge(tmp);
		tmp=le;   le=onext( le); deleteedge(tmp);
		tmp=te1; te1=onext(te1); deleteedge(tmp);
	    }
	    
	    ptr2n( le,&e1,&r1);
	    ptr2n(be2,&e2,&r2);
	    ptr2n( re,&e3,&r3);
	    ptr2n(te1,&e4,&r4);

	    tri0->le_e=e1; tri0->le_r=r1;
	    tri0->be_e=e2; tri0->be_r=r2;
	    tri0->re_e=e3; tri0->re_r=r3;
	    tri0->te_e=e4; tri0->te_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}


            
    // case3 : ng1>=2 and ng2==1
        if (ng1>=2 && ng2==1){
            le1=n2ptr(tri1->le_e, tri1->le_r);
	    re1=n2ptr(tri1->re_e, tri1->re_r);
            te1=n2ptr(tri1->te_e, tri1->te_r);
	    be1=n2ptr(tri1->be_e, tri1->be_r);
            le2=n2ptr(tri2->le_e, tri2->le_r);
	    be2=makeedge(); org(be2)=org(le2); splice(le2,be2);
	    re2=makeedge(); org(re2)=org(le2); splice(be2,re2);
	    te2=makeedge(); org(te2)=org(le2); splice(re2,te2);

	    (sym(le1))->shell = SHELL; shlpush(sym(le1));
	    for(tmp=oprev(le1);sym(tmp)!=le1;tmp=lnext(tmp)){
	        tmp->shell = SHELL; shlpush(tmp);
	    }
	    
	    tmp=makeedge(); org(tmp)=org(le1);
	        dest(tmp)= WALL; splice(tmp,oprev(le1)); le1=tmp;
	    tmp=makeedge(); org(tmp)=org(re1);
	        dest(tmp)= WALL; splice(tmp,oprev(be1)); be1=tmp;
	    tmp=makeedge(); org(tmp)=org(re1);
	        dest(tmp)= WALL; splice(tmp,oprev(re1)); re1=tmp;
	    tmp=makeedge(); org(tmp)=org(te1);
	        dest(tmp)= WALL; splice(tmp,oprev(te1)); te1=tmp;
	    if (oprev(te1)==le1){ tmp=te1; te1=le1; le1=tmp;}

	    if (righter(org(re1),org(re2))){
	        re=re1; red=re2;
	        rinf=1;
	    }
	    else{
	        re=re2; red=re1; re1=onext(re1);
	        rinf=0;
	    }
	    if (righter(org(le1),org(le2))){
	        le=le2; led=le1; le1=oprev(le1);
	    }
	    else{
	        le=le1; led=le2;
	    }

	    if (oprev(re)==be1){
	        tmp=makeedge();
	        dest(tmp)= WALL;
	        splice(lnext(oprev(be1)),tmp);
	        st1=s1stl(le1,le2);
	        deleteedge(tmp);
	    }
	    else st1=s1stl(le1,le2);

	    basel=connect(sym(le2),st1);
	    deleteedge(led);
	    if (rinf==1){
	        if (rprev(basel)==be1){
	            splice(be1,oprev(be1));
	            splice(be1,dnext(basel));
	        }
	    }
	    else {
	        if (rprev(basel)==be1 && onext(be1)==red){
	            re1=rprev(re1);
		    splice(red,oprev(red));
		    splice(red,oprev(re1));
		}
	    }
            tm1=f1stl(re2,re1);
	    deleteedge(be1); deleteedge(te2);
	    deleteedge(red);
	    basel=s1merge(basel,tm1);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }

	    if (rinf==1){
		tmp=be2; be2=onext(be2); deleteedge(tmp);
	        tmp=le;   le=onext( le); deleteedge(tmp);
	        tmp=te1; te1=onext(te1); deleteedge(tmp);
	        tmp=re;   re=onext( re); deleteedge(tmp);
	    }
	    else {
	        tmp=re;   re=onext( re); deleteedge(tmp);
	        tmp=be2; be2=onext(be2); deleteedge(tmp);
		tmp=le;   le=onext( le); deleteedge(tmp);
		tmp=te1; te1=onext(te1); deleteedge(tmp);
	    }
	    
	    ptr2n( le,&e1,&r1);
	    ptr2n(be2,&e2,&r2);
	    ptr2n( re,&e3,&r3);
	    ptr2n(te1,&e4,&r4);

	    tri0->le_e=e1; tri0->le_r=r1;
	    tri0->be_e=e2; tri0->be_r=r2;
	    tri0->re_e=e3; tri0->re_r=r3;
	    tri0->te_e=e4; tri0->te_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}


    // case4 : ng1>=2 and ng2>=2
        if (ng1>=2 && ng2>=2){
            le1=n2ptr(tri1->le_e, tri1->le_r);
            le2=n2ptr(tri2->le_e, tri2->le_r);
            re1=n2ptr(tri1->re_e, tri1->re_r);
	    re2=n2ptr(tri2->re_e, tri2->re_r);
            te1=n2ptr(tri1->te_e, tri1->te_r);
            te2=n2ptr(tri2->te_e, tri2->te_r);
            be1=n2ptr(tri1->be_e, tri1->be_r);
	    be2=n2ptr(tri2->be_e, tri2->be_r);

	    (sym(le1))->shell = SHELL; shlpush(sym(le1));
            for(tmp=oprev(le1);sym(tmp)!=le1;tmp=lnext(tmp)){
                tmp->shell = SHELL; shlpush(tmp);
            }
	    le2->shell = SHELL; shlpush(le2);
            for(tmp=rprev(le2);tmp!=le2;tmp=rprev(tmp)){
                tmp->shell = SHELL; shlpush(tmp);
            }


	    tmp=makeedge(); org(tmp)=org(le1);
	        dest(tmp)= WALL; splice(tmp,oprev(le1)); le1=tmp;
	    tmp=makeedge(); org(tmp)=org(be1);
	        dest(tmp)= WALL; splice(tmp,oprev(be1)); be1=tmp;
	    tmp=makeedge(); org(tmp)=org(re1);
	        dest(tmp)= WALL; splice(tmp,oprev(re1)); re1=tmp;
	    tmp=makeedge(); org(tmp)=org(te1);
	        dest(tmp)= WALL; splice(tmp,oprev(te1)); te1=tmp;
	    if (oprev(te1)==le1){ tmp=te1; te1=le1; le1=tmp;}

	    tmp=makeedge(); org(tmp)=org(le2);
	        dest(tmp)= WALL; splice(tmp,oprev(le2)); le2=tmp;
	    tmp=makeedge(); org(tmp)=org(be2);
	        dest(tmp)= WALL; splice(tmp,oprev(be2)); be2=tmp;
	    tmp=makeedge(); org(tmp)=org(re2);
	        dest(tmp)= WALL; splice(tmp,oprev(re2)); re2=tmp;
	    tmp=makeedge(); org(tmp)=org(te2);
	        dest(tmp)= WALL; splice(tmp,oprev(te2)); te2=tmp;
	    if (oprev(te2)==le2){ tmp=te2; te2=le2; le2=tmp;}

	    if (righter(org(re1),org(re2))){
	        re=re1; red=re2; re2=oprev(re2);
	        rinf=1;
	    }
	    else{
	        re=re2; red=re1; re1=onext(re1);
	        rinf=0;
	    }
	    if (righter(org(le1),org(le2))){
	        le=le2; led=le1; le1=oprev(le1);
	    }
	    else{
	        le=le1; led=le2; le2=onext(le2);
	    }

	    if (oprev(re)==be1){
	        tmp=makeedge();
	        dest(tmp)= WALL;
	        splice(lnext(oprev(be1)),tmp);
	        normstl(le1,le2,&st1,&st2);
	        deleteedge(tmp);
	    }
	    else{
	        if (onext(re)==te2){
	            tmp=makeedge();
	            dest(tmp)= WALL;
	            splice(  sym(onext(te2)),tmp);
	            normstl(le1,le2,&st1,&st2);
	            deleteedge(tmp);
	        }
	        else normstl(le1,le2,&st1,&st2);
	    }
	    basel=connect(sym(st2),st1);
	    deleteedge(led);
	    if (rinf==1){
	        if (oprev(basel)==te2 && oprev(te2)==red){
	            re2=lnext(re2);
		    splice(red,oprev(red));
		    splice(red,re2);
		}
	        if (rprev(basel)==be1){
	            splice(be1,oprev(be1));
	            splice(be1,dnext(basel));
	        }
	    }
	    else{
	        if (rprev(basel)==be1 && onext(be1)==red){
	            re1=rprev(re1);
		    splice(red,oprev(red));
		    splice(red,oprev(re1));
		}
		if (oprev(basel)==te2){
		    splice(te2,oprev(te2));
		    splice(te2,lnext(oprev(basel)));
		}
	    }
            normstl(re2,re1,&tm2,&tm1);
	    deleteedge(be1); deleteedge(te2);
	    deleteedge(red);
	    basel=normmerge(basel,tm1,tm2);
	    if (oprev(basel)!=tm2) basel=f1merge(basel,tm2);
	    else if (rprev(basel)!=tm1) basel=s1merge(basel,tm1);
	    if (ng1+ng2!=numgen-numsame){
	        while(shlsp>0){ tmp=shlpop(); tmp->shell = EMP;}
	        while(gensp>0){ g=genpop(); gen[g].note = EMP;}
	    }


	    if (rinf==1){
		tmp=be2; be2=onext(be2); deleteedge(tmp);
	        tmp=le;   le=onext( le); deleteedge(tmp);
	        tmp=te1; te1=onext(te1); deleteedge(tmp);
	        tmp=re;   re=onext( re); deleteedge(tmp);
	    }
	    else {
	        tmp=re;   re=onext( re); deleteedge(tmp);
	        tmp=be2; be2=onext(be2); deleteedge(tmp);
		tmp=le;   le=onext( le); deleteedge(tmp);
		tmp=te1; te1=onext(te1); deleteedge(tmp);
	    }
	    
	    ptr2n( le,&e1,&r1);
	    ptr2n(be2,&e2,&r2);
	    ptr2n( re,&e3,&r3);
	    ptr2n(te1,&e4,&r4);

	    tri0->le_e=e1; tri0->le_r=r1;
	    tri0->be_e=e2; tri0->be_r=r2;
	    tri0->re_e=e3; tri0->re_r=r3;
	    tri0->te_e=e4; tri0->te_r=r4;
	    
	    tri0->ng=ng1+ng2;
	    return;
	}
}



/*
    heap sort
 */
void Thiessen::heapify(int i, int j)
{
	int k;
	int t;

	if (2*i+1<=j){
	    if (2*i+2<=j){
	        if (scrighter(gennote[2*i+1],gennote[2*i+2]))
		    k=2*i+1;
		else k=2*i+2;
		if (scrighter(gennote[k],gennote[i])){
		    t=gennote[k]; gennote[k]=gennote[i]; gennote[i]=t;
		    heapify(k,j);
		}
	    }
	    else{
	        if (scrighter(gennote[2*i+1],gennote[i])){
	            t=gennote[2*i+1];
	            gennote[2*i+1]=gennote[i];
	            gennote[i]=t;
	            heapify(2*i+1,j);
	        }
	    }
	}
}


void Thiessen::heapsort()
{
	int i;
	int ng;
	int t;

	ng=gensp;
	for(i=ng-1;i>=0;i--) heapify(i,ng-1);
	
	for(i=ng-1;i>=1;i--){
	    t=gennote[0]; gennote[0]=gennote[i]; gennote[i]=t;
	    heapify(0,i-1);
	}
}


/*
     make triangulation
	for a small number of generators
 */
void Thiessen::maketri(cell *tri0)
{
	int ng;
	int gen1,gen2,gen3;
	int CCW;
	int h,l;
	int e1,e2,e3,r1,r2,r3;
	 diredge *a,*b,*c;

	ng=tri0->ng;
	
	if (ng==1){
	    gen1=tri0->be_e;
	    a=makeedge();
	    org(a)=gen1;
	    ptr2n(a,&e1,&r1);
	    tri0->be_e=e1; tri0->be_r=r1;
	    tri0->te_e=e1; tri0->te_r=r1;
	    tri0->le_e=e1; tri0->le_r=r1;
	    tri0->re_e=e1; tri0->re_r=r1;
	    return;
	}
	
	if (ng==2){
	    gen1=tri0->be_e; gen2=tri0->be_r;
	    a=makeedge();
	    org(a)=gen1; dest(a)=gen2;
	    ptr2n(a,&e1,&r1);
	    ptr2n(sym(a),&e2,&r2);
	    if (higher(gen1,gen2)){
	        tri0->be_e=e2; tri0->be_r=r2;
	        tri0->te_e=e1; tri0->te_r=r1;
	    }
	    else{
	        tri0->be_e=e1; tri0->be_r=r1;
	        tri0->te_e=e2; tri0->te_r=r2;
	    }
	    tri0->le_e=e2; tri0->le_r=r2;
	    tri0->re_e=e1; tri0->re_r=r1;
	    gen[gen1].note= EMP;
	    return;
	}
	
	if (ng==3){
	    gen1=tri0->be_e;
	    gen2=gen[gen1].note;
	    gen3=gen[gen2].note;
	    a=makeedge(); org(a)=gen1; dest(a)=gen2;
	    b=makeedge(); org(b)=gen2; dest(b)=gen3;
	    splice(sym(a),b);
	    c=connect(b,a);

	    if (higher(gen1,gen2) && higher(gen1,gen3)){
	        h=1;
	        if (higher(gen2,gen3)){ l=3; CCW=0;}
	        else{ l=2; CCW= -1;}
	    }
	    if (higher(gen2,gen1) && higher(gen2,gen3)){
	        h=2;
	        if (higher(gen1,gen3)){ l=3; CCW=1;}
	        else{ l=1; CCW=1;}
	    }
	    if (higher(gen3,gen1) && higher(gen3,gen2)){
	        h=3;
	        if (higher(gen1,gen2)){ l=2; CCW= -1;}
	        else{ l=1; CCW=0;}
	    }
	    
	    if (CCW==1 || (CCW==0 && ccw(gen1,gen2,gen3))){
	        ptr2n(a,&e1,&r1);
	        ptr2n(b,&e2,&r2);
	        ptr2n(c,&e3,&r3);
	    }
	    else{
	        ptr2n(sym(c),&e1,&r1);
	        ptr2n(sym(a),&e2,&r2);
	        ptr2n(sym(b),&e3,&r3);
	    }
	    
	    tri0->le_e=e3; tri0->le_r=r3;
	    tri0->re_e=e1; tri0->re_r=r1;
	    if (h==1){ tri0->te_e=e1; tri0->te_r=r1;}
	    if (h==2){ tri0->te_e=e2; tri0->te_r=r2;}
	    if (h==3){ tri0->te_e=e3; tri0->te_r=r3;}
	    if (l==1){ tri0->be_e=e1; tri0->be_r=r1;}
	    if (l==2){ tri0->be_e=e2; tri0->be_r=r2;}
	    if (l==3){ tri0->be_e=e3; tri0->be_r=r3;}

	    gen[gen1].note= EMP;
	    gen[gen2].note= EMP;
	    return;
	}
}


/*
    make triangulation
	in one bucket
 */
void Thiessen::triangulate(cell *tri0)
{
	int i,j;
	int ng;
	int gen1,gen2,gen3;
	int h,numslab;
	int pw,div,mod;
	int t,g;
	int res1,res2,res3,ns;

	ng=tri0->ng;

	if (ng==0) return;

	if (ng==1){ maketri(tri0); return;}

	if (ng==2){
	    gen1=tri0->be_e; gen2=tri0->be_r;
	    res1=scrighter(gen1,gen2);
	    if (res1== SAME){
		tri0->ng=1;
		tri0->be_r=gen1;
		gen[gen1].note= EMP;
		numsame++;
		maketri(tri0);
		return;
	    }
	    if (res1){
	        maketri(tri0);
	        return;
	    }
	    else {
	        tri0->be_e=gen2;
	        tri0->be_r=gen1;
	        gen[gen2].note=gen1;
	        gen[gen1].note= EMP;
		maketri(tri0);
		return;
	    }
	}

	if (ng==3){
	    gen1=tri0->be_e;
	    gen2=gen[gen1].note;
	    gen3=gen[gen2].note;
	    if (res1=scrighter(gen3,gen2)){ t=gen3; gen3=gen2; gen2=t;}
	    if (res2=scrighter(gen2,gen1)){ t=gen2; gen2=gen1; gen1=t;}
	    if (res3=scrighter(gen3,gen2)){ t=gen3; gen3=gen2; gen2=t;}
	    if (res1== SAME || res2== SAME || res3== SAME){
		if (res1== SAME && res2== SAME && res3== SAME){
		    tri0->ng=1;
		    tri0->be_e=gen1;
		    tri0->be_r=gen1;
		    gen[gen1].note= EMP;
		    numsame+=2;
		    maketri(tri0);
		    return;
		}
		else {
		    tri0->ng=2;
		    tri0->be_e=gen1;
		    tri0->be_r=gen3;
		    gen[gen1].note=gen3;
		    gen[gen3].note= EMP;
		    numsame++;
		    maketri(tri0);
		    return;
		}
	    }
	    tri0->be_e=gen1;
	    tri0->be_r=gen3;
	    gen[gen1].note=gen2;
	    gen[gen2].note=gen3;
	    gen[gen3].note= EMP;
	    maketri(tri0);
	    return;
	}
	    
	if (ng>=4){
	    genclear();
	    gen1=tri0->be_e;
	    genpush(gen1);
	    for(i=1;i<ng;i++){
		gen1=gen[gen1].note;
	        genpush(gen1);
	    }
	    if (gen[gen1].note != EMP) 
		printf("\nerror : something is wrong at inserting sites to stackGENNOTE\n");
	    heapsort();

	    ns=numsame;
	    if (gen[gennote[0]].note== SAME)
	        gen[gennote[0]].note= EMP;
	    for(i=1;i<ng;i++){
	        if (gen[gennote[i]].note== SAME){
	            if (righter(gennote[i],gennote[i-1])!= SAME)
	                gen[gennote[i]].note= EMP;
	            else numsame++;
	        }
	    }

	    
	    if (numsame==ns){
	        for(i=0,pw=1; 3*pw<ng; i++,pw*=2);
	        h=i; numslab=pw;
	        div=ng/numslab; mod=ng%numslab;
	    
	        for(i=0;i<mod;i++){
	            g=genpop();
	            slab[i].be_e=g; slab[i].be_r=g; slab[i].ng=1;
	            for(j=1;j<div+1;j++){
	                g=genpop();
	                gen[(slab[i].be_r)].note=g;
	                slab[i].be_r=g;
	                (slab[i].ng)+=1;
	            }
	            gen[(slab[i].be_r)].note= EMP;
	        }
	        for(;i<numslab;i++){
	            g=genpop();
	            slab[i].be_e=g; slab[i].be_r=g; slab[i].ng=1;
	            for(j=1;j<div;j++){
	                g=genpop();
	                gen[(slab[i].be_r)].note=g;
	                slab[i].be_r=g;
	                (slab[i].ng)+=1;
	            }
	            gen[(slab[i].be_r)].note= EMP;
	        }
		if (gensp!=0) 
			printf("\nerror : something is wrong at inserting sites to slabs\n");
	    }
	    else {
		ng=ng-(numsame-ns);
	        for(i=0,pw=1; 3*pw<ng; i++,pw*=2);
	        h=i; numslab=pw;
	        div=ng/numslab; mod=ng%numslab;
	    
	        for(i=0;i<mod;i++){
		    do{
	                g=genpop();
	            }while(gen[g].note== SAME);
	            slab[i].be_e=g; slab[i].be_r=g; slab[i].ng=1;
	            for(j=1;j<div+1;j++){
			do{
	                    g=genpop();
			}while(gen[g].note== SAME);
	                gen[(slab[i].be_r)].note=g;
	                slab[i].be_r=g;
	                (slab[i].ng)+=1;
	            }
	            gen[(slab[i].be_r)].note= EMP;
	        }
	        for(;i<numslab;i++){
		    do{
	                g=genpop();
		    }while(gen[g].note== SAME);
	            slab[i].be_e=g; slab[i].be_r=g; slab[i].ng=1;
	            for(j=1;j<div;j++){
			do{
	                    g=genpop();
	                }while(gen[g].note== SAME);
	                gen[(slab[i].be_r)].note=g;
	                slab[i].be_r=g;
	                (slab[i].ng)+=1;
	            }
	            gen[(slab[i].be_r)].note= EMP;
	        }
		while(gensp>0){
		    if (gen[gennote[gensp-1]].note== SAME) g=genpop();
		    else break;
		}
		if (gensp!=0) printf
  ("\nerror : something is wrong at inserting sites in 'SAME' mode\n");
	    }


	    for(i=0;i<numslab;i++)
	        maketri(&slab[i]);
	    if (numslab==1){
		tri0->ng=slab[0].ng;
		tri0->be_e=slab[0].be_e; tri0->be_r=slab[0].be_r;
		tri0->te_e=slab[0].te_e; tri0->te_r=slab[0].te_r;
		tri0->le_e=slab[0].le_e; tri0->le_r=slab[0].le_r;
		tri0->re_e=slab[0].re_e; tri0->re_r=slab[0].re_r;
		return;
	    }
	    for(i=h,pw=numslab;i>1;i--,pw/=2){
	        for(j=0;j<pw;j+=2){
	            horizontalmerge(&slab[j+1],&slab[j],&slab[j/2]);
	        }
	    }
	    horizontalmerge(&slab[1],&slab[0],tri0);
	    return;
	}
}



/*
    calculate coordinates of
	triangle's circumcenter
 */
void Thiessen::circumctr(diredge* ded)
{//in
	int ga,gb,gc;
	double xa,xb,xc;
	double ya,yb,yc;
	double xi,xj,xk;
	double yi,yj,yk;
	double la,lb,lc;
	double dxi,dxj,dyi,dyj,sqi,sqj;
	double j2,j3,j4;
	double length;
	double epsilon;
	 diredge *ded2,*ded3;

	epsilon=1.0e-50;
	
	ga=org(ded);
	ded2=lnext(ded);  gb=org(ded2);
	ded3=lnext(ded2); gc=org(ded3);
	if (dest(ded3) != ga)
	    printf("\nerror : triangulation is not complete\n");
	if (ga<0 || gb<0 || gc<0 ||
	      ga>=numgen || gb>=numgen || gc>=numgen){
	    printf
	      ("\nerror : irregular generator call in fn. CIRCUMCTR");
	    printf("\n  %d %d %d\n",ga,gb,gc);
	    return;
	}
	if (vpt_ptr>=(m_GENSIZE+3)*2) {
	    printf("\nerror : array v_pt is full\n");
	    return;
	}
	xa=gen[ga].x_co; ya=gen[ga].y_co;
	xb=gen[gb].x_co; yb=gen[gb].y_co;
	xc=gen[gc].x_co; yc=gen[gc].y_co;

	la=(xb-xc)*(xb-xc)+(yb-yc)*(yb-yc);
	lb=(xc-xa)*(xc-xa)+(yc-ya)*(yc-ya);
	lc=(xa-xb)*(xa-xb)+(ya-yb)*(ya-yb);
	if (la>=lb){
	    if (la>=lc){ xk=xa; xi=xb; xj=xc; yk=ya; yi=yb; yj=yc;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}
	else{
	    if (lb>=lc){ xk=xb; xi=xc; xj=xa; yk=yb; yi=yc; yj=ya;}
	    else       { xk=xc; xi=xa; xj=xb; yk=yc; yi=ya; yj=yb;}
	}

	dxi=xi-xk; dxj=xj-xk;
	dyi=yi-yk; dyj=yj-yk;
	sqi=(dxi*dxi+dyi*dyi)/2.0;
	sqj=(dxj*dxj+dyj*dyj)/2.0;
	j2=dyi*sqj-dyj*sqi; j3=dxi*sqj-dxj*sqi; j4=dxi*dyj-dxj*dyi;
	if ((-1.0)*epsilon<j4 && j4<epsilon){
	    length=sqrt(j2*j2+j3*j3);
	    v_pt[vpt_ptr].o_co=0;
	    v_pt[vpt_ptr].x_co=(-2.0)*j2*areasize/length;
	    v_pt[vpt_ptr].y_co=  2.0 *j3*areasize/length;
	}
	else{
	    v_pt[vpt_ptr].o_co=1;
	    v_pt[vpt_ptr].x_co=(-1.0)*j2/j4+xk;
	    v_pt[vpt_ptr].y_co=j3/j4+yk;
	}
	org(rotinv(ded ))=vpt_ptr;
	org(rotinv(ded2))=vpt_ptr;
	org(rotinv(ded3))=vpt_ptr;
	vpt_ptr++;
}


/*
    processing for
	infinite Voronoi edges
 */
void Thiessen::infvored(diredge *ded)
{
	int ga,gb;
	double xa,xb,ya,yb;
	double dx,dy,length;
	ga=org(ded);
	gb=dest(ded);
	if (ga<0 || gb<0 || ga>=numgen || gb>=numgen){
	    printf
	       ("\nerror : irregular generator call in fn. INFVORED");
	    printf("\n  %d %d",ga,gb);
	    return;
	}
	if (vpt_ptr>=(m_GENSIZE+3)*2){
	    printf("\nerror : array V_PT is full\n");
	    return;
	}
	xa=gen[ga].x_co; ya=gen[ga].y_co;
	xb=gen[gb].x_co; yb=gen[gb].y_co;
	dx=xb-xa; dy=yb-ya;
	length=sqrt(dx*dx+dy*dy);
	v_pt[vpt_ptr].o_co=0;
	v_pt[vpt_ptr].x_co=(2.0*areasize*  dy )/length;
	v_pt[vpt_ptr].y_co=(2.0*areasize*(-dx))/length;
	org(rot(ded))=vpt_ptr;
	vpt_ptr++;
}


/*
    make Voronoi diagram
        from Delaunay triangulation
 */
void Thiessen::makevor(diredge *ded)
{
	int te,tr;
	int e,r;
	int i,counter;
	ptr2n(ded,&te,&tr);
	infvored(ded);
	ded=rprev(ded);
	for(;;){
	    ptr2n(ded,&e,&r);
	    if (te==e && tr==r) break;
	    infvored(ded);
	    ded=rprev(ded);
	}
	
	for(i=0,counter=0;i<numedge;i++){
	    ded=n2ptr(i,0);
	    if (ded->shell != NO_USED){
		counter++;
	        if (org(rotinv(ded))== EMP) circumctr(ded);
	        ded=sym(ded);
	        if (org(rotinv(ded))== EMP) circumctr(ded);
	    }
	}
	if (i==counter)
	    printf("\nwarning : don't you forget to do USECHECK?\n");
}


/*
     read data of generators' locations
 */
void Thiessen::PushData(int n, std::vector<double>& x, std::vector<double>& y)
{
	int i;
    numgen = n;

	if (n <= m_GENSIZE) {
		x_max_gen=x_min_gen=x.at(0);
		y_max_gen=y_min_gen=y.at(0);

		for(i=0;i < numgen;i++){
			if (x_max_gen<x.at(i)) x_max_gen=x.at(i);
			if (x_min_gen>x.at(i)) x_min_gen=x.at(i);
			if (y_max_gen<y.at(i)) y_max_gen=y.at(i);
			if (y_min_gen>y.at(i)) y_min_gen=y.at(i);
			gen[i].x_co=x.at(i);
			gen[i].y_co=y.at(i);
		}
		xwidth=x_max_gen - x_min_gen;
		ywidth=y_max_gen - y_min_gen;
		if (xwidth>ywidth) areasize=xwidth;
		else areasize=ywidth;

	}
	else exit(1);
}

/*
    check whether use or no-use
 */
void Thiessen::usecheck()
{
	int i;
	int t;
	 diredge *ded;
	for(i=0;i<gbgsp;i++){
	    t=garbage[i];
	    ded=n2ptr(t,0);
	    ded->shell= NO_USED;
	}
}



/*
    output coordinates of vertices
	incident to Voronoi edges and Delaunay edges
	without control codes
	(usually, this must be all that people want to get...)
 */
void Thiessen::outdata(int nPoints, int *nEdges, LineSegment *V, LineSegment *D, myBox B, myPolygon* PolyIx)
{


	int i,j, Ix=0;
	int cc;
	int orgn[4],shl;
	int o1,o2;
	double x1,x2,y1,y2,mdx,mdy;
	EndPointCode clip = OUTSIDE;

	diredge *ded;

	typedef int* doubleint;

	doubleint* Polyx;
	int* nE;

	double *Left, *Right, *Bott, *Top;
	int	nLeft=0, nRight=0, nBott=0, nTop=0;

	Left  = new double [nPoints/2];
	Right = new double [nPoints/2];
	Top   = new double [nPoints/2];
	Bott  = new double [nPoints/2];

	Polyx = new doubleint [nPoints];
	nE = new int [nPoints];

	for (i=0; i < nPoints; i++) {
		Polyx[i] = new int[20];
		nE[i] = 0;
	}
	

	int nV=0;
	
	
	for(i=0,cc=0;i<numedge;i++){
	    ded=n2ptr(i,0);
	    if (ded->shell != NO_USED){
			cc++;
			for(j=0;j<4;j++) orgn[j]=(ded+j)->origin;
			if (ded->shell == SHELL || (ded+2)->shell == SHELL)
				shl=1;
			else shl=0;
			(D+cc)->f.x = gen[orgn[0]].x_co;
			(D+cc)->f.y = gen[orgn[0]].y_co;
			(D+cc)->t.x = gen[orgn[2]].x_co;
			(D+cc)->t.y = gen[orgn[2]].y_co;
			
			o1=v_pt[orgn[1]].o_co; o2=v_pt[orgn[3]].o_co;
			x1=v_pt[orgn[1]].x_co; x2=v_pt[orgn[3]].x_co;
			y1=v_pt[orgn[1]].y_co; y2=v_pt[orgn[3]].y_co;
			mdx=(gen[orgn[0]].x_co+gen[orgn[2]].x_co)/2.0;
			mdy=(gen[orgn[0]].y_co+gen[orgn[2]].y_co)/2.0;
			if (o1==0){ x1+=mdx; y1+=mdy; }
			if (o2==0){ x2+=mdx; y2+=mdy; }
			if ((x1==x2) && (y1==y2)) cc--;
			else {

				clip = ClippLine(B, &x1, &y1, &x2, &y2);
				while (clip == CLIP) 
				{
					clip = ClippLine(B, &x1, &y1, &x2, &y2);

				} 
				if (clip != OUTSIDE) 
				{

					(V+nV)->f.x = x1; (V+nV)->f.y = y1; 
					(V+nV)->t.x = x2; (V+nV)->t.y = y2; 
					(V+nV)->p1  = -1;  (V+nV)->p2 = -1; 
					(V+nV)->twin = 2;
					(V+nV)->poly1 = orgn[0]; // this edge belong to poly1
					(V+nV)->poly2 = orgn[2]; // this edge belong also to poly2

					Polyx[orgn[0]][nE[orgn[0]]] = nV;//putting nV-th edge as boundary for polygons-orgn[0];
					nE[orgn[0]] += 1;
					Polyx[orgn[2]][nE[orgn[2]]] = nV;
					nE[orgn[2]] += 1;
					nV++;

				}

			}

		}
	}

	// Creating Polygon index: List of polygon index to edges;
	for (i=0; i<nPoints; i++) {
		PolyIx[i].n = nE[i];
		PolyIx[i].p = new int [nE[i]+2];
		for (j=0; j<nE[i]; j++) {
			PolyIx[i].p[j] = Polyx[i][j];

		}
	}

	*nEdges = nV;
	if (i==cc)
	     wxMessageBox("\nWarning : Did you forget to use USECHECK?\n");


	delete [] Polyx;
	Polyx = NULL;
}

/*
    initialization
 */
void Thiessen::initdata()
{
	diredge *ded;
	int e,r;
	/* initilize global variables */
	gbgsp = 0;
	vpt_ptr = 0;
	numsame = 0;
	genclear();
	shlclear();

	numedge=numgen*3+10;
	for(e=0;e<numedge;e++){
	    edptr[e] = &edge1[e*4];
	}
	for(e=0;e<numedge;e++){
	    for(r=0;r<4;r++){
		ded=n2ptr(e,r);
		ded->e=e;
		ded->origin= EMP;
		ded->shell = EMP;
	    }
	    gbgpush(e);
	}
}




void Thiessen::MakeQHull(int nObs, std::vector<double>& raw_X, std::vector<double>& raw_Y, 
		  int *nEdges, LineSegment *V, LineSegment *D, myBox B, myPolygon* PolyIx)
{
	int alpha;
	int K,logK,sqrtK;
	int lg,pw;
	int h,i,j;
	int bucketn,bucketx,buckety;
	int n;
	double x,y;
	double unitsize;
	cell T1,T2;
	
	if (nObs < 1) {
		exit(0);
	}

	PushData(nObs, raw_X, raw_Y);
	initdata();

	alpha=0;

	for (lg=0, pw=1; pw<=numgen; lg++, pw*=4);
	lg--; pw/=4;
	for(i=0;i<alpha;i++,pw/=4);
	if (lg-alpha>0){ K=pw; logK=lg-alpha;}
	else { K=1; logK=0;}
	for(i=0;i*i<=K;i++);
	i--; sqrtK=i;
	for (i=0;i<K;i++) bucket[i].ng=0;
	
	unitsize=areasize/(double)sqrtK;
	for(i=0;i<numgen;i++){
	    x=gen[i].x_co; y=gen[i].y_co;
	    bucketx=(int)((x-x_min_gen)/unitsize);
	    if (bucketx>=sqrtK) bucketx=sqrtK-1;
	    buckety=(int)((y-y_min_gen)/unitsize);
	    if (buckety>=sqrtK) buckety=sqrtK-1;
	    bucketn=bucketx+sqrtK*buckety;
	    if (bucket[bucketn].ng==0){
	        bucket[bucketn].be_e=i;
	        bucket[bucketn].be_r=i;
	        bucket[bucketn].ng=1;
	    }
	    else{
	        gen[(bucket[bucketn].be_r)].note=i;
	        bucket[bucketn].be_r=i;
	        bucket[bucketn].ng+=1;
	    }
	}
	for(i=0;i<K;i++){
	   if (bucket[i].ng!=0) gen[(bucket[i].be_r)].note= EMP;
	}
	    
	for(i=0;i<K;i++) triangulate(&bucket[i]);
	
    // STEP 2
	for(h=logK,pw=sqrtK;h>=1;h--,pw/=2){
	    for(i=0;i<pw;i+=2){
	        for(j=0;j<pw;j+=2){
	            n=i+sqrtK*j;

				horizontalmerge(&bucket[n],&bucket[n+1],&T1);
	            horizontalmerge(&bucket[n+sqrtK],&bucket[n+sqrtK+1],&T2);
	            n=i/2+sqrtK*j/2;
	            verticalmerge(&T2,&T1,&bucket[n]); 

	        }

	    }
	}
	//Computation of Voronoi	
	usecheck();
	makevor(n2ptr(bucket[0].be_e, bucket[0].be_r));

	outdata(nObs, nEdges, V, D, B, PolyIx);
}


void Thiessen::CollectPoints(int nEdges, LineSegment *V, myPoint* P, int* nP) 
{
	// collect all points from V and put them in P
	// 
	int i = 0;
	for (i=0; i<nEdges * 2; i++) {
		P[i].n = 0;
		P[i].e = new int [7];

	}

	for (i=0; i<nEdges; i++) {
		P[i*2].x = V[i].f.x;
		P[i*2].y = V[i].f.y;
		P[i*2].e[0] = i;
		P[i*2].n += 1;
		P[i*2+1].x = V[i].t.x;
		P[i*2+1].y = V[i].t.y;
		P[i*2+1].e[0] = i;
		P[i*2+1].n += 1;
	}

	int (*compare) (const void*, const void*);
	compare = XYSort;
	qsort((void*) P, nEdges*2, sizeof(myPoint), compare);


   // SQUEIZING
	myPoint t; t.e = new int [5];

	i = 0; 
	int k=0;
	while (i<nEdges*2) {
		t.x = P[i].x;
		t.y = P[i].y;
		t.n = 1;
		t.e[0] = P[i].e[0];
		i++; 
		while (t.x ==P[i].x && t.y == P[i].y && i<nEdges*2) 
		{
			t.n += 1;
			t.e[t.n -1] = P[i].e[0];
			i++;
		}
		P[k].x = t.x;
		P[k].y = t.y;
		P[k].n = t.n;
		for (int j=0; j< t.n; j++) P[k].e[j] = t.e[j];
		k++;

	}	
	
	*nP = k;
	//CreateEdgeList
	for (i=0; i< k ; i++) {
		for (int j=0; j<P[i].n; j++) {
			if (V[P[i].e[j]].f.x == P[i].x && V[P[i].e[j]].f.y == P[i].y)
				 V[P[i].e[j]].p1 = i;
			else V[P[i].e[j]].p2 = i;
		}		
	}
}


myPolygon* Thiessen::GeneratePolygons(int nGen, LineSegment *V, int nP, myPoint* P, myPolygon* PolyIx, myBox B )
{
	// GENERATING POLYGONS from
	myPolygon* gPoly;
	gPoly = new myPolygon [nGen];

	int nn,jj, I1[100], I2[100], I3[100]; int last=-1, first=-1;
	double xx, yy;
	int i = 0;
	int j = 0;
	for (i=0; i<nGen; i++) 
	{
		xx = gen[i].x_co; yy = gen[i].y_co;

		for (j=0;j < 20;j++) I3[j]=-1;
		for (j=0; j<PolyIx[i].n; j++) 
		{
			I1[j] = V[PolyIx[i].p[j]].p1;
			I2[j] = V[PolyIx[i].p[j]].p2;
		}

		if (!IsCCW(xx,yy,P[I1[0]].x,P[I1[0]].y,P[I2[0]].x,P[I2[0]].y)) 
		{
			int temp= I1[0];
			I1[0] = I2[0];
			I2[0] = temp;
		}

		I3[0] = I1[0];
		I3[1] = I2[0];
		first = I3[0];
		last  = I3[1];
		I2[0]=-1;
		nn = PolyIx[i].n;
		jj=2;  bool found; 
		while (first != last) 
		{
			found = false;
			for  (int j=0; !found && j < nn; j++) 
			{
				if (I1[j] >= 0) 
				{
					if (I1[j] == last) 
					{
						last = I2[j];
						found = true;
						I2[j] = -1;
						I1[j] = -1;
					}
					else if(I2[j] == last) 
					{
						last = I1[j];
						found = true;
						I2[j] = -1;
						I1[j] = -1;
					}
				}
			}
			if (found) 
			{

				if (last >=0 && last != I3[jj-1])
				{
					I3[jj] = last;
					jj++;
				}
			}
			else 
			{
				double xL=P[last].x, yL=P[last].y; 
				int kk; short axis = 0;

				if (xL == B.p1.x) {axis = 1;}
				else if (xL == B.p2.x) {axis = 2;}

				if (yL == B.p1.y) {axis += 5;}
				else if (yL == B.p2.y) {axis += 8;}

				found = false;
				for  (int j=0; !found && j < nn; j++) 
				{
					if (I1[j] >= 0) 
					{

						if (axis == 1 || axis == 9) 
						{
							if (P[I1[j]].x == xL && P[I1[j]].y <= yL) 
							{
								found = true;
								kk = I1[j];
							}
						}
						else if (axis == 2 || axis == 7) 
						{
							if (P[I1[j]].x == xL && P[I1[j]].y >= yL) 
							{
								found = true;
								kk = I1[j];
							}
						}
						else if (axis == 5 || axis == 6) 
						{
							if (P[I1[j]].y == yL && P[I1[j]].x >= xL) 
							{
								found = true;
								kk = I1[j];
							}
						}
						else if (axis == 8 || axis == 10) 
						{
							if (P[I1[j]].y == yL && P[I1[j]].x <= xL) 
							{
								found = true;
								kk = I1[j];
							}
						}

					}	

					if (!found && I2[j] >=0)
					{
						if (axis == 1 || axis == 9) 
						{
							if	(P[I2[j]].x == xL && P[I2[j]].y <= yL) 
							{
								found = true;
								kk = I2[j];
							}
						}
						else if (axis == 2 || axis == 7) 
						{
							if	(P[I2[j]].x == xL && P[I2[j]].y >= yL) 
							{
								found = true;
								kk = I2[j];
							}
							
						}
						else if (axis == 5 || axis == 6) 
						{
							if	(P[I2[j]].y == yL) // && P[I2[j]].x >= xL) 
							{
								found = true;
								kk = I2[j];
							}
						}
						else if (axis == 8 || axis == 10) 
						{
							if	(P[I2[j]].y == yL && P[I2[j]].x <= xL) 
							{
								found = true;
								kk = I2[j];
							}
						}
					} // I2 > 0
				}

				if (found) 
				{
					if (kk >=0 && kk != I3[jj-1])
					{
						last = kk;
						I3[jj] = kk;
						jj++;
					}
				}
				else 
				{ // New point

					if (axis==1 || axis == 9) 
					{ //BL
						last = nP;
					}
					else if (axis == 2 ||axis == 7 ) 
					{ // TR
						last = nP+2;
					}
					else if (axis == 5 || axis == 6) 
					{ // BR
						last = nP+1;
					}
					else if (axis == 8 || axis == 10) 
					{// TL
						last = nP+3;
					}
					else 
					{
						last = nP+3;
						continue;
					}

					I3[jj] = last;
					jj++;
				} // New Point
			 } // 
			 
		}
		gPoly[i].ID = i;
		gPoly[i].n = jj;
		gPoly[i].p = new int[jj];
		gPoly[i].co = new DPOINT [jj];
				
		int k = 0;

		for ( j=0; j<jj; j++) 
		{

			gPoly[i].p[j] = I3[j];
			gPoly[i].co[j].x = P[I3[j]].x;
			gPoly[i].co[j].y = P[I3[j]].y;
		}
		
	} // loop i, each generator
	return gPoly;
	
}

