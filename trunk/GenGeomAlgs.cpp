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
#include "GenGeomAlgs.h"

double GenGeomAlgs::ComputeEucDist(double x1, double y1, double x2, double y2) 
{
	return sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

/*
 Notes on ComputeArcDist:
 In the equation below, the 69.11 factor is actually the distance,
 in miles, between each degree of latitude for the WGS
 84 sphere. Remember that this equation is just an approximation
 because the earth is actually an ellipsoid. Because of
 this, the distance between latitudes will increase as the latitude
 increases. The distance at 0∞ on the WGS 84 ellipsoid is
 actually 68.71 miles while
 it is 69.40 miles at 90∞.
 */

double GenGeomAlgs::ComputeArcDist(double long1, double lat1,
								   double long2, double lat2)
{
	//const double pi = 3.141592653589;
	const double rad = 0.017453292519938; // rad = pi/180.0
	double rlat1 = (90.0 - lat1) *rad;
	double rlat2 = (90.0 - lat2) *rad;
	double rlong = (long2-long1) *rad;
	double drad = cos(rlong)*sin(rlat1)*sin(rlat2) + cos(rlat1)*cos(rlat2);
	double dist = acos(drad)*3959.0;  // approx radius of earth in miles
	return dist;  // in miles
}

/*
 * Fhe following four functions: findArea, ComputeArea2D,
 *     and ComputePerimeter2D are borrowed from FastArea.c++
 *
 * From the paper:
 *
 *      Daniel Sunday
 *      "Fast Polygon Area and Newell Normal Computation"
 *      journal of graphics tools, 7(2):9-13, 2002
 *
 */

// assume vertex coordinates are in arrays x[], y[], and z[]
// with room to duplicate the first two vertices at the end

// return the signed area of a 2D polygon
double GenGeomAlgs::findArea(int n, double *x, double *y)             // 2D polygon
{
	// guarantee the first two vertices are also at array end
	x[n] = x[0];
	y[n] = y[0];
	x[n+1] = x[1];
	y[n+1] = y[1];
	
	double sum = 0.0;
	double *xptr = x+1, *ylow = y, *yhigh = y+2;
	for (int i=1; i <= n; i++) {
		sum += (*xptr++) * ( (*yhigh++) - (*ylow++) );
	}
	return (sum / 2.0);
}

double GenGeomAlgs::ComputeArea2D(int n, double *x, double *y) // output unit normal
{
	// get the Newell normal
	double *z = new double [n+2];
	for (int i=0;i<n+2; i++)
		z[i] = 0.0;
	double nwx = GenGeomAlgs::findArea(n, y, z);
	double nwy = GenGeomAlgs::findArea(n, z, x);
	double nwz = GenGeomAlgs::findArea(n, x, y);
	
	// get length of the Newell normal
	double nlen = sqrt( nwx*nwx + nwy*nwy + nwz*nwz );
	return nlen;    // area of polygon = length of Newell normal
}

double GenGeomAlgs::ComputePerimeter2D(int n, double *x, double *y) 
{
	double Peri = GenGeomAlgs::ComputeEucDist(x[0],y[0],x[n-1],y[n-1]);
	for (int i=0; i < n-1; i++)
		Peri += GenGeomAlgs::ComputeEucDist(x[i],y[i],x[i+1],y[i+1]);
	return Peri;
}

