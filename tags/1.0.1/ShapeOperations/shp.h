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

/* Interface for Classes and types necessarily to handle shapefiles
	(binary form) and associated Boundary files (ASCII format).
*/

#ifndef __GEODA_CENTER_GEODA_SHP_H__
#define __GEODA_CENTER_GEODA_SHP_H__

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "AbstractShape.h"
#include "BasePoint.h"
#include "Box.h"
#include "DBF.h"
#include "ShapeFile.h"
#include "ShapeFileHdr.h"
#include "ShapeFileTypes.h"
#include "../GeoDaConst.h"
#include "../GenGeomAlgs.h"
#include "../GenUtils.h"
using namespace std;

extern long ReadBig(ifstream &input);
extern void ggcvt(double d, int n, char* str);

#define SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
//void SWAP(char& x, char& y, char& t) { t = x; x = y; y = t; }
//void SWAP(int& x, int& y, int& t) { t = x; x = y; y = t; }
//void SWAP(int& x, int& y, double& t) { t = x; x = y; y = t; }
//void SWAP(double& x, double& y, double& t) { t = x; x = y; y = t; }
//void SWAP(size_t& x, size_t& y, int& t) { t = x; x = y; y = t; }
//#define	HUGE_NUMBER		99999999
const long HUGE_NUMBER = 99999999;


typedef struct Ref  {
    int next, prev;
    Ref(const int nxt= -1, const int prv= -1) : next(nxt), prev(prv) {};
} RefStruct;

typedef RefStruct* RefPtr;
typedef char*      CharPtr;
typedef double*	   DoublePtr;
typedef int*       IntPtr;

/**
 Ppoint
 Corresponds to BasePoint shapes in the shapefile.
 */
class Ppoint :  public virtual AbstractShape  
{
	private :
	BasePoint p;
	public :
	Ppoint()  {};
	Ppoint(char *name) : AbstractShape(name)  {};
	virtual ~Ppoint()  {};
	virtual Box ShapeBox() const  {  return Box(p);  };
	virtual Box SetData(int nParts, int* Part, int nPoints,
						const std::vector<BasePoint>& P) 
	{ Box b; return b;}
	virtual BasePoint Centroid() const { return p; }
	virtual BasePoint MeanCenter() const { return p; }
	virtual BasePoint* GetPoints() const { return NULL; }
	virtual long ContentsLength() const  { return 20; }
	virtual ostream& WriteShape(ostream &s) const
    {  WriteID(s);  return s << p << endl; }
	virtual istream& ReadShape(istream &s);
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual void AssignPointData(double x, double y) { p.setXY(x,y); }
};

/**
 MultiPoint - Multipoint Shape
 Class for shapes with multiple points.
 Corresponds to multipoint shape in the shapefile.
  */
class MultiPoint : public virtual AbstractShape  {
public:
	MultiPoint() : NumPoints(0), Points(NULL) {};
	MultiPoint(char *nme, const long &points) :  AbstractShape(nme),
		Points(new BasePoint[ points ]), NumPoints(points) {};
	virtual ~MultiPoint()  {  if (Points) delete [] Points; return;  };
	virtual Box ShapeBox() const  {  return bBox;  };
	virtual BasePoint MeanCenter() const;
	virtual BasePoint Centroid() const;
	virtual long ContentsLength() const  {  return 20 + 8 * NumPoints;  };
	virtual ostream& WriteShape(ostream &s) const;
	virtual istream& ReadShape(istream &s);
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual Box SetData(int nParts, int* Part, int nPoints,
						const std::vector<BasePoint>& P) {
		Box b; return b;
	}
	virtual void AssignPointData(double x, double y) {};
	protected :
	long          NumPoints;
	BasePoint         *Points;
	Box           bBox;
	void ComputeBox();
};

/**
 Shape
 Class for shapes with multiple parts.
  */
class Shape : public virtual MultiPoint  {
public :
	virtual long ContentsLength() const {
		return 22 + 2 * NumParts + 8 * NumPoints;  }
	virtual ostream& WriteShape(ostream &s) const;
	virtual istream& ReadShape(istream &s) {
		MultiPoint::ReadShape(s);  NumParts= 1;
		Parts= new long int[1];  Parts[0]= 0;
		return s;
    };
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual Box SetData(int nParts, int* Part, int nPoints,
						const std::vector<BasePoint>& P);
	protected :
	long int      NumParts;
	long int      *Parts;
	Box			oBox; // FileBox: from data
	Shape() : Parts(NULL), NumParts(0)  {};
	Shape(char *nme, const long int &parts, const long int &points) :
	MultiPoint(nme, points), NumParts(parts), Parts(new long[ parts ]) {
		Parts[0]= 0;
		return;
	}
	virtual ~Shape()  {  if (Parts) delete [] Parts;  return; };
	virtual void AssignPointData(double x, double y) {};
};

/**
 PolygonShape
 Class for Polygon shapes - corresponds to the Polygon shape
 in the shapefile.
 */
class PolygonShape : public virtual Shape {
 public:
  PolygonShape &  operator+=(const PolygonShape &a);
  virtual void    SeparateParts();
};

/**
 BasePartition
 */
class BasePartition  {
	protected :
    int         elements, cells;
    int *       cell;
    int *       next;
    double      step;
	public :
    BasePartition(const int els= 0, const int cls= 0, const double range= 0);
    virtual ~BasePartition();
    void virtual alloc(const int els, const int cls, const double range);
    int         Cells() const  {  return cells;  };
    double      Step() const  {  return step;  };
    virtual void include(const int incl, const double range)  {
        int where = (int) floor(range/step);
        // if (where < -1 || where > cells || incl < 0 || incl >= elements)
        //     cout << " BasePartition: incl= " << incl << " location= "
		//          << where << " els= " << elements << " cells= "
		//          << cells << endl;
        if (where < 0) where= 0;
		else if (where >= cells) where= cells-1;
        next [ incl ] = cell [ where ];
        cell [ where ] = incl;
        return;
    };
	
    int first(const int cl) const  {  return cell [ cl ];  };
    int tail(const int elt) const  {  return next [ elt ];  };
};

/**
 PartitionP
 */
class PartitionP : public BasePartition  {
	private :
    int *       cellIndex;
    int *       previous;
	public :
    PartitionP(const int els= 0, const int cls= 0, const double range= 0);
    ~PartitionP();
    void alloc(const int els, const int cls, const double range);
	
    void include(const int incl);
    void initIx(const int incl, const double range)  {
        int cl= (int) floor(range / step);
		// if (cl < -1 || cl > cells || incl < 0 || incl >= elements)
		//     cout << "PartitionP: incl= " << incl << " at " << cl << endl;
        if (cl < 0) cl= 0;
		else if (cl >= cells) cl= cells-1;
        cellIndex[ incl ] = cl;
        return;
    };
    int inTheRange(const double range) const  
	{
        if (range < 0 || range/step > cells) return -1;
        int where= (int) floor(range / step);
        if (where < 0) where= 0;
        else if (where >= cells) --where;
        return where;
    }
    void remove(const int del);
    void cleanup(const BasePartition &p, const int cl)  {
		for (int cnt= p.first(cl); cnt != GeoDaConst::EMPTY; cnt= p.tail(cnt))
			remove(cnt);
    }
};


/**
 PolygonPartition
 */
class PolygonPartition : public PolygonShape
{
	private :	
	BasePartition       pX;
    PartitionP          pY;
    int *               nbrPoints;
    int prev(const int pt) const  
	{
        int ix= nbrPoints[pt];
        return (ix >= 0) ? pt-1 : -ix;
    }
    int succ(const int pt) const  
	{
        int ix= nbrPoints[pt];
        return (ix >= 0) ? ix : pt+1;
    }
	
	public :	
    PolygonPartition() : PolygonShape(), pX(), pY(), nbrPoints(NULL) {}
    ~PolygonPartition();
    int MakePartition(int mX= 0, int mY= 0);
    void MakeSmallPartition(const int mX, const double Start,
							const double Stop);
    void MakeNeighbors();
    bool edge(const PolygonPartition &p, const int host, const int guest);
    int sweep(PolygonPartition & guest, const int criteria= 0);
};

/** BoundaryShape */
class BoundaryShape : public virtual PolygonPartition 
{
public:
	long int GetNumPoints() {return NumPoints;}
    BasePoint* GetPoints() {return Points;}
	long int GetNumParts() {return NumParts;}
	long int* GetParts() {return Parts;}
};




extern bool operator ==(const BasePoint &a, const BasePoint &b);
extern bool operator != (const BasePoint &a, const BasePoint &b);
extern BasePoint pmin (const BasePoint &a, const BasePoint &b);
extern BasePoint pmax (const BasePoint &a, const BasePoint &b);
extern BasePoint& operator += (BasePoint &a, const BasePoint &b);
extern istream& operator >>(istream &s, BasePoint &p);
extern ostream& operator <<(ostream &s, const BasePoint &p);
extern iShapeFile& operator >>(iShapeFile &s, BasePoint &p);
extern oShapeFile& operator <<(oShapeFile &s, const BasePoint &p);
extern istream& operator >>(istream &s, Box &b);
extern ostream& operator <<(ostream &s, const Box &b);
extern iShapeFile& operator >>(iShapeFile &s, Box &b);
extern oShapeFile& operator <<(oShapeFile &s, const Box &b);



#endif
