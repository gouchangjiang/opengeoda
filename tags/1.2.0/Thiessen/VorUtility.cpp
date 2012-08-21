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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "VorDataType.h"

/*
    Guibas & Stolfi's
	geometrical predicates
 */

double dp(double a)
{
	double b,c,n,f,pw,r;

	b=a;
	if (a<0.0) b= -a;
	else if (a==0.0) return(0.0);
	n=floor(log(b)/log(2.0));

	// PREC = 52
	pw=pow(2.0,n- 52.0);
	c=b/pw;
	f=floor(c);
	r=c-f;
	if (r>=0.5) f+=1.0;
	if (a<0.0) return(-pw*f);
	else return(pw*f);
}
//*****************************************************//

EndPointCode ComputeEndPointCode (myBox B, double x, double y)
{
	EndPointCode c = 0;
	if (y > B.p2.y)
		c |= TOP;
	else if (y < B.p1.y)
		c |= BOTTOM;
	if (x > B.p2.x)
		c |= RIGHT;
	else if (x < B.p1.x)
		c |= LEFT;
	return c;
}



EndPointCode ClippLine(myBox B, double* x0, double* y0, double* x1, double* y1)
{
	EndPointCode C0, C1, C, CC;
	double x, y;
	
	C0 = ComputeEndPointCode (B, *x0, *y0);
	C1 = ComputeEndPointCode (B, *x1, *y1);
	CC = C0 ? C0 : C1;

	for (;;) {
		/* trivial accept: both ends in rectangle */
		if ((C0 | C1) == 0) {
//			MidPointLineReal (x0, y0, x1, y1);
			return(CC);
		}
		
		/* trivial reject: both ends on the external side of the rectangle */
		if ((C0 & C1) != 0)
			return(OUTSIDE);
		
		/* normal case: clip end outside rectangle */
		C = C0 ? C0 : C1;
		if (C & TOP) {
			x = *x0 + (*x1 - *x0) * (B.p2.y - *y0) / (*y1 - *y0);
			y = B.p2.y;
			CC = TOP;
		} else if (C & BOTTOM) {
			x = *x0 + (*x1 - *x0) * (B.p1.y - *y0) / (*y1 - *y0);
			y = B.p1.y;
			CC = BOTTOM;
		} else if (C & RIGHT) {
			x = B.p2.x;
			y = *y0 + (*y1 - *y0) * (B.p2.x - *x0) / (*x1 - *x0);
			CC = RIGHT;
		} else {
			x = B.p1.x;
			y = *y0 + (*y1 - *y0) * (B.p1.x - *x0) / (*x1 - *x0);
			CC = LEFT;
		}
		
		/* set new end point and iterate */
		if (C == C0) {
			*x0 = x; *y0 = y;
			C0 = ComputeEndPointCode (B, *x0, *y0);
		} else {
			*x1 = x; *y1 = y;
			C1 = ComputeEndPointCode (B, *x1, *y1);
		}
	}
	
	/* notreached */


	// ClippLine(B, x0, y0, x1, y1);
	return(CLIP);

}

bool IsCCW(double xc, double yc, double xa, double ya, double xb, double yb)
{
	double la,lb,lc;
	double xi,xj,xk,yi,yj,yk;
	double dxi,dxj;
	double dyi,dyj;
	double p1,p2;
	double res;

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
	if (res>0) return true;
	else return false;
}


int XYSort(const void* fp1, const void* fp2)
{
	myPoint f1 = *(myPoint*) fp1;
	myPoint f2 = *(myPoint*) fp2;


    int c=0;
	double p1,p2;
	p1 = f1.x * 100000000 + f1.y;
	p2 = f2.x * 100000000 + f2.y;

	if (p1 < p2) return -1;
	else if (p1 == p2) return 0;
	else return 1;
}





