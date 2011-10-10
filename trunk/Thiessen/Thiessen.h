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

#ifndef __GEODA_CENTER_THIESSEN_H__
#define __GEODA_CENTER_THIESSEN_H__

#include <vector>

class Thiessen {
protected:
		generator	*gen;
		voronoi_pt	*v_pt;
		diredge		*edge1;
		diredge		**edptr;
		cell		*bucket;
		cell		*slab;
		int			gbgsp;
		int			*garbage;
		int			gensp;
		int			*gennote;
		int			shlsp;
		diredge		**shelledge;
		int			PREC;
		int			m_GENSIZE;
		int			m_EDGESIZE;
		int			numgen;
		int			numedge;
		double		x_max_gen,x_min_gen;
		double		y_max_gen,y_min_gen;
		double		xwidth,ywidth,areasize;
		int			vpt_ptr;
		int			numsame;

	private:
		int ccw(int ga, int gb, int gc);
		int incircle(int ga, int gb, int gc, int gd);

		void circumctr(diredge* ded);
		void CollectPoints(int nEdges, LineSegment *V, myPoint* P, int* nP); 
		diredge *connect(diredge *d_a, diredge *d_b);
		void deleteedge(diredge *ded);
		diredge *dnext(diredge *ded);
		diredge *dprev(diredge *ded);
		diredge *f1merge(diredge *basel,diredge *tm2);
		diredge *f1stl(diredge *first1,diredge *first2);
		void gbgclear();
		int gbgpop();
		int gbgpush(int e);
		void genclear();
		myPolygon* GeneratePolygons(int nGen, LineSegment *V, int nP, myPoint* P, myPolygon* PolyIx, myBox B );
		int genpop();
		int genpush(int gen);
		void heapify(int i, int j);
		void heapsort();
		short higher(int gen1, int gen2);
		void horizontalmerge(cell *tri1, cell *tri2, cell *tri0);
		void verticalmerge(cell *tri1,cell *tri2,cell *tri0);
		void infvored(diredge *ded);
		void initdata();
		diredge *lnext(diredge *ded);
		diredge *lprev(diredge *ded);
		diredge *makeedge();
	void MakeQHull(int nObs, std::vector<double>& raw_X, std::vector<double>& raw_Y, int *nEdges, LineSegment *V, LineSegment *D, myBox B, myPolygon* PolyIx);
		void maketri(cell *tri0);
		void makevor(diredge *ded);
		diredge *normmerge(diredge *basel,diredge *tm1,diredge *tm2);
		void normstl(diredge *first1,diredge *first2, diredge *(*pst1), diredge *(*pst2));
		diredge *onext(diredge *ded);
		diredge *oprev(diredge *ded);
		void outdata(int nPoints, int *nEdges, LineSegment *V, LineSegment *D, myBox B, myPolygon* PolyIx);
		void parallel(diredge *basel,diredge *cand1,diredge *cand2,int *ppal1, int *ppal2);
		void ptr2n(diredge *ded, int *pe, int *pr);
	void PushData(int n, std::vector<double>& x, std::vector<double>& y);
		short righter(int gen1, int gen2);
		diredge *rnext(diredge *ded);
		diredge *rot(diredge *ded);
		diredge *rotinv(diredge *ded);
		diredge *rprev(diredge *ded);
		diredge *s1merge(diredge *basel,diredge *tm1);
		diredge *s1stl(diredge *first1, diredge *first2);
		short scrighter(int gen1, int gen2);
		void shlclear();
		diredge *shlpop();
		int shlpush(diredge *ded);
		int splice(diredge *a, diredge *b);
		diredge *sym(diredge *ded);
		void triangulate(cell *tri0);
		void usecheck();
		myPolygon* Polygons;
		long	   nPolygon;
	public:
		int		error;
	Thiessen(int ndata, std::vector<double>& x, std::vector<double>& y, myBox B);
		virtual ~Thiessen();
		myPolygon* GetPolygons() {return Polygons; };
		long GetPolygonSize() { return nPolygon; };

};

#endif

