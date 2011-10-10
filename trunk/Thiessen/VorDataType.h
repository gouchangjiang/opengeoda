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

#ifndef __GEODA_CENTER_VOR_DATA_TYPE_H__
#define __GEODA_CENTER_VOR_DATA_TYPE_H__

#define EMP      -1
#define NO_USED  -2
#define SAME     -3
#define SHELL    -10
#define PEND     -20
#define PENDSM   -21
#define FENDSM   -31
#define WALL     -100

#define n2ptr(e,r) (edptr[(e)]+(r))
#define org(ded)  ((ded)->origin)
#define dest(ded) (sym((ded))->origin)

typedef struct dpoint_stru {
	double x,y;
} DPOINT;

typedef struct generator_stru{
	double x_co,y_co;
	long    note;
} generator;


typedef struct voronoi_pt_stru{
	int  o_co;
	double x_co,y_co;
} voronoi_pt;

typedef struct diredge_stru{
	long    e;
	long   origin;
	long    next_e;  long  next_r;
	long  shell;
} diredge;

typedef diredge* p_diredge;

/*
typedef struct cell_stru{
	int    ng;
	int    be_e;    int    be_r;
	int    te_e;    short  te_r;
	int    le_e;    short  le_r;
	int    re_e;    short  re_r;
} cell;
*/

typedef struct cell_stru{
	long    ng;
	long   be_e;    long  be_r;
	long   te_e;    long  te_r;
	long   le_e;    long  le_r;
	long   re_e;    long  re_r;
} cell;

typedef struct point_stru{
  double        x;
  double		y;
  int			n; // # of edges shared that point
  int*			e; // list of edge indexes
} myPoint;

typedef struct Box_stru {
		DPOINT p1;
		DPOINT p2;
} myBox;

typedef struct Line_stru {
	DPOINT f;
	DPOINT t;
	int		p1; // index for f pointing to Point table
	int		p2; // index for t
	short	twin; // indicator for line sharing by polygons (1 or 2)
	int		poly1;
	int		poly2;
} LineSegment;

typedef struct polygon_stru {
	int		ID; // Polygon ID 
	short	n; // # of points
	myBox	B; // Box
	int*	p; // list of point index
	DPOINT*  co;
} myPolygon;

typedef myPolygon* PPolygon;




typedef unsigned int EndPointCode;
enum {	OUTSIDE = 0xF, 
		TOP		= 0x1, 
        BOTTOM	= 0x2, 
		RIGHT	= 0x4, 
		LEFT	= 0x8,
		TOPLEFT = 0x9,
		TOPRIGHT= 0x5,
		BOTLEFT = 0xA,
		BOTRIGHT= 0x6,
		CLIP	= 0xFF
};

extern int XYSort(const void* , const void*);
extern bool IsCCW(double, double, double, double, double, double);
extern EndPointCode ComputeEndPointCode (myBox , double , double );
extern EndPointCode ClippLine(myBox, double*, double*, double*, double*);
extern double dp(double);



#endif

