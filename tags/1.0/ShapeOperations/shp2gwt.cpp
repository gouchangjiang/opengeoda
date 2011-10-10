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

/*
 Functions to create spatial weights (.gwt) from any shapefile.
 */

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "shp.h"
#include "shp2gwt.h"
#include "shp2cnt.h"
#include <math.h>
#include <stdio.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <time.h>
#include "../GenUtils.h"
#include "../GeoDaConst.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "ShapeFileTriplet.h"
#include "ShapeFileTypes.h"
#include "DbfFile.h"

class PartitionGWT
{
	protected :
    int         elements, cells;
    int *       cell;
    int *       next;
    double      step;
	public :
    PartitionGWT(const int els= 0, const int cls= 0, const double range= 0);
	virtual ~PartitionGWT();
    void  alloc(const int els, const int cls, const double range);
    int         Cells() const  {  return cells;  };
    double      Step()  const  {  return step;  };
    int         Where(const double range)  {
        int where= (int)floor(range/step);
        if (where < 0) where= 0;
		else if (where >= cells) where= cells-1;
        return where;
    };
	
    void include(const int incl, const double range)  {
        int     where= Where( range );
        next [ incl ] = cell [ where ];
        cell [ where ] = incl;
        return;
    };
	
    int first(const int cl) const  {  return cell [ cl ];  };
    int tail(const int elt) const  {  return next [ elt ];  };
    void reset()  {
		
        for (int cnt= 0; cnt < cells; ++cnt)
			cell [ cnt ] = GeoDaConst::EMPTY;
        return;
    };
    void reset(const double range)  {
        cell [ Where(range) ] = GeoDaConst::EMPTY;
        return;
    };
};


typedef PartitionGWT * PartitionPtr;

void PartitionGWT::alloc(const int els, const int cls, const double range)  {
	
	elements= els;
	cells= cls;
	step= range / cls;
	cell= new int [ cells ];
	next= new int [ elements ];
	if (cell && next)
		reset();
    else elements= cells= 0;
	return;
	
}


PartitionGWT::PartitionGWT(const int els, const int cls, const double range)
: elements(els), cells(cls),
cell(NULL), next(NULL)  {
	if (elements > 0) alloc(els, cls, range);
	return;
}


PartitionGWT::~PartitionGWT()  
{
	if (cell != NULL) delete [] cell;
	cell = NULL;
	if (next != NULL) delete [] next;
	next = NULL;
	elements = cells = 0;
	return;
}


class Location  
{
	private :
	long    points;
	BasePoint * centroid;
	double threshold, distanceSquare;
	Box    BoundingBox;
	long   current;
	BasePoint  currPoint;
	int		method;
	public :
	double currDistance;
	Location(BasePoint * pts, const double distance, const long num, int mt);
	virtual ~Location();
	bool good()  const  {  return points > 0;  };
	long   x_cells()  const  {  return  (long)floor( BoundingBox.range_x(method)/threshold ) + 1;  };
	long   y_cells()  const  {  return (long)floor( BoundingBox.range_y(method)/threshold ) + 1;  };
	double x_range(int method)  const  {  return  BoundingBox.range_x(method);  };
	double y_range(int method)  const  {  return  BoundingBox.range_y(method);  };
	double x_range(const long elt, int method)  const {
		if (method==1) // 1:Eucledian Distance 2:Arc Distance
			return centroid[elt].x - BoundingBox._min().x;
		else
			return GenGeomAlgs::ComputeArcDist(BoundingBox._min().x,BoundingBox._min().y,centroid[elt].x,BoundingBox._min().y);
	};
	double y_range(const long elt, int method)  const {
		if (method==1)
			return centroid[elt].y - BoundingBox._min().y;
		else
			return GenGeomAlgs::ComputeArcDist(BoundingBox._min().x,BoundingBox._min().y,BoundingBox._min().x,centroid[elt].y);
	};
	void setCurrent(const long cp) {
        current= cp;
        currPoint= centroid[cp];
        return;
	};
	bool InTheSemiCircle(const long pt);
	bool InTheCircle(const long pt);
	void CheckParticle(PartitionGWT * Y, const long cell, long &BufferSize,
					   GwtNeighbor * buffer);
};

Location::Location(BasePoint * pts, 
				   const double distance, 
				   const long num, int mt) : 
centroid(pts),
threshold(distance), 
points(num),
method(mt)
{
    if (threshold < 0) 
		threshold = - threshold;
	
    distanceSquare= threshold * threshold;
    if (centroid == NULL)  
	{ 
		points= 0; 
		return;  
	};
    BoundingBox= Box(centroid[0]);
    for(long cnt= 1; cnt < points; ++cnt) BoundingBox += centroid[ cnt ];
}



Location::~Location()  {
	delete [] centroid;
	centroid = NULL;
	points = 0;
}



inline bool Location::InTheCircle(const long pt)  
{
    BasePoint   work= centroid[pt];
	double d2;
	if (method == 2)
	{
		d2 = GenGeomAlgs::ComputeArcDist(currPoint.x,currPoint.y,work.x,work.y);
		d2 = d2 * d2;
	}
	else {
		double dx = work.x-currPoint.x;
		double dy = work.y-currPoint.y;
		d2= dx*dx + dy*dy;
	}
	
    bool test= (d2 <= distanceSquare);
    if (test) currDistance= sqrt( d2 );
    return test;
}



inline bool Location::InTheSemiCircle(const long pt)  
{
    BasePoint   work= centroid[pt];
    if (work.x > currPoint.x) return false;
    if (work.x == currPoint.x)
		if (work.y > currPoint.y) return false;
		else if (work.y == currPoint.y)  // identically located points
		{
			currDistance= threshold * 0.000001;  // set 'small' distance
			return (pt < current);               // use id to introduce assymetry
		}
	
    double d2=-1;
	if (method == 2) 
	{
		d2 = GenGeomAlgs::ComputeArcDist(currPoint.x,currPoint.y,work.x,work.y);
		d2 = d2 * d2;
	}
	else {
		double dx = work.x-currPoint.x;
		double dy = work.y-currPoint.y;
		d2= dx*dx + dy*dy;
	}	
	
    bool test= (d2 <= distanceSquare);
    if (test) currDistance= sqrt( d2 );
    return test;
}



void Location::CheckParticle(PartitionGWT * Y, 
							 const long cell, 
							 long &BufferSize,
							 GwtNeighbor * buffer)  
{
    for ( long cnt= Y->first(cell); cnt != GeoDaConst::EMPTY; cnt= Y->tail(cnt)) {
		if (InTheSemiCircle( cnt)) {
			buffer[ BufferSize++ ] = GwtNeighbor(cnt, currDistance);
		}
	}
}



extern long GetShpFileSize(const wxString& fname);

bool ReadId(const wxString& fname, const wxString& field, long* id )  
{
    if (id == NULL) {
        wxMessageBox("Error: id array of longs points to NULL.");
        return false;
    }
	
    DbfFileReader dbf_r(fname);
    if (!dbf_r.isDbfReadSuccess()) {
        wxMessageBox("Error: There was a problem reading the DBF file.");
		return false;
    }
	
    if (!dbf_r.isFieldExists(field)) {
        wxString msg="Error: ID Variable " + field + " not found ";
        msg+="in DBF file \"" + fname + "\".";
        wxMessageBox(msg);
		return false;
    }
	
    DbfFieldDesc f_desc = dbf_r.getFieldDesc(field);
    if (f_desc.type == 'C' || f_desc.decimals != 0) {
        wxString msg="Error: ID Variable is not of type integer.";
        wxMessageBox(msg);
        return false;
    }
	
    std::vector<wxInt64> id_vals(dbf_r.getNumRecords());
    if (! dbf_r.getFieldValsLong(field, id_vals)) {
        wxMessageBox("Error: could not read in ID Variable values.");
        return false;
    }
	
    for (int i=0, iend=id_vals.size(); i<iend; i++) {
        id[i] = (long) id_vals[i];
    }
	
    return true;
}

// read id from dbf file
bool ReadData(const wxString& fname, char* varname, std::vector<double>& dt)
{
	long gObs = GetShpFileSize(fname);	
	iDBF dbf( fname );
	if (!dbf.IsConnectedToFile()) {
		wxMessageBox("Error in opening the .dbf file!");
		return false;
	}
	
	const int column=dbf.FindField(wxString(varname));
	if (column == GeoDaConst::EMPTY) return false;
	
	for (int cnt= 0; cnt < gObs; ++cnt)  {
		while (dbf.Pos() != column) dbf.Read();
		dbf.Read(dt.at(cnt));
	}
	return true;
}

// read id from dbf file, but with wxString for varname
bool ReadData(const wxString& fname, const wxString& varname,
			  std::vector<double>& dt)
{
	long gObs = GetShpFileSize(fname);	
	iDBF dbf(fname);
	if (!dbf.IsConnectedToFile()) {
		wxMessageBox("Error in opening the .dbf file!");
		return false;
	}
	
	const int column = dbf.FindField(varname);
	if (column == GeoDaConst::EMPTY) return false;
	
	for (int cnt= 0; cnt < gObs; ++cnt)  {
		while (dbf.Pos() != column) dbf.Read();
		dbf.Read(dt.at(cnt));
	}
	return true;
}

double ComputeMaxDistance(int Records, std::vector<double>& x,
						  std::vector<double>& y, int methods) 
{
	
	BasePoint minmax[2];
	
	BasePoint *centroid= new BasePoint [ Records ];
	int rec = 0;
	for (rec =0; rec < Records; rec++)
		centroid[rec].setXY(x.at(rec),y.at(rec));
	
	minmax[0].setXY(centroid[0].x, centroid[0].y);
	minmax[1].setXY(centroid[0].x, centroid[0].y);
	
	for (rec = 1; rec < Records; ++rec) {
		if (centroid[rec].x > minmax[1].x) 
			minmax[1].setXY(centroid[rec].x, minmax[1].y);
		else if (centroid[rec].x < minmax[0].x) 
			minmax[0].setXY(centroid[rec].x, minmax[0].y);
		if (centroid[rec].y > minmax[1].y) 
			minmax[1].setXY(minmax[1].x, centroid[rec].y);
		else if (centroid[rec].y < minmax[0].y) 
			minmax[0].setXY(minmax[0].x, centroid[rec].y);
	}
	
	double dist = sqrt(geoda_sqr(minmax[1].x - minmax[0].x) + geoda_sqr(minmax[1].y - minmax[0].y));
	if (methods == 2) {
		// Arc Distance
		dist = GenGeomAlgs::ComputeArcDist(minmax[0].x, minmax[0].y,
										   minmax[1].x, minmax[1].y);
	}
	return dist;
}




DBF_descr* CreateDBFDesc4Thiessen(char* infl, int* numCol, bool* willbeIn,
								  int* numDesc)
{
	
	iDBF tb(infl);
	
	int m_row = tb.GetNumOfRecord();
	int m_col = tb.GetNumOfField();
	
	*numCol = m_col;
	
	DBF_descr *dbfdesc;
	dbfdesc			= new DBF_descr [m_col+3];
	dbfdesc[0]	= new DBF_field("POLYID", 'N',5,0);
	dbfdesc[1]	= new DBF_field("AREA", 'N',16,6);
	dbfdesc[2]	= new DBF_field("PERIMETER", 'N',16,6);
	
	
	int k = 3;
	for (int col = 0; col < m_col; col++)	
	{
		willbeIn[col] = true;
		wxString m_ColNm	= wxString::Format("%s",tb.GetFieldName(col));
		m_ColNm.Trim(false);
		m_ColNm.Trim(true);
		if (m_ColNm == "POLYID" || 
			m_ColNm == "PERIMETER" ||
			m_ColNm == "AREA")
		{
			willbeIn[col] = false;
		}
		else
		{
			int		width						= tb.GetFieldSize(col);
			//			if (width == 32) width = 33; //it is really funny! why?
			const char	ty				= tb.GetFieldType(col);
			const int		prec			= tb.GetFieldPrecision(col);
			dbfdesc[k] = new DBF_field(m_ColNm.wx_str(), ty,width,prec);
			k++;
		}
	}
	
	*numDesc = k;
	
	return dbfdesc;
	
}

DBF_descr* CreateDBFDesc4Grid(wxString ID)
{
	
	DBF_descr *dbfdesc;
	if (ID==wxEmptyString)
	{
		dbfdesc			= new DBF_descr [3];
		dbfdesc[0]	= new DBF_field("POLYID", 'N',8,0);
		dbfdesc[1]	= new DBF_field("AREA", 'N',16,6);
		dbfdesc[2]	= new DBF_field("PERIMETER", 'N',16,6);
	}
	else
	{
		dbfdesc			= new DBF_descr [4];
		dbfdesc[0]	= new DBF_field(ID.wx_str(), 'N',8,0);
		dbfdesc[1]	= new DBF_field("AREA", 'N',16,6);
		dbfdesc[2]	= new DBF_field("PERIMETER", 'N',16,6);
		dbfdesc[3]	= new DBF_field("RECORD_ID", 'N',8,0);
	}
	
	return dbfdesc;
	
}

bool CreateSHPfromBoundary(wxString ifl, wxString otfl)
{
	// Open the Boundary file
	ifstream ias;
	ias.open(ifl.wx_str());
	int nRows;
	char name[1000];
	ias.getline(name,100);
	//wxString tok = wxString::Format("%100s",name);
	wxString tok = wxString(name, wxConvUTF8);
	wxString ID_name=wxEmptyString;
	
	int pos = tok.Find(',');
	if( pos >= 0)
	{
		//nRows = wxAtoi(tok.Left(pos));
		long temp;
		tok.Left(pos).ToCLong(&temp);
		nRows = (int) temp;
		ID_name = tok.Right(tok.Length()-pos-1);
	}
	else
	{
		wxMessageBox("Wrong format!");
		return false;
	}
	
	ID_name.Trim(false);
	ID_name.Trim(true);
	
	if (nRows < 1 || ID_name == wxEmptyString)
	{
		wxMessageBox("Wrong format!");
		return false;
	}
	
	double const EPS = 1e-10;
	BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
	BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
	Box fBox(a,b);
	
	
	// Define the output file
	DBF_descr *dbfdesc = CreateDBFDesc4Grid(ID_name);
	oShapeFileTriplet Triple(otfl,fBox, dbfdesc, 4,ShapeFileTypes::POLYGON);
	AbstractShape* shape = new PolygonShape;
	if (shape == NULL )
		return false;
	int nParts=1;
	int* Parts;
	Parts = new int[nParts];
	Parts[0]=0;
	
	// Assuming max # of points in a polygon is 1000
	int const MAX_POINT = 1000;
	std::vector<BasePoint> Points(MAX_POINT);
	double* x = new double[MAX_POINT];
	double* y = new double[MAX_POINT];
	
	long polyid = 0, ID; int nPoint;
	for (long row = nRows; row >= 1; row--) 
	{
		ias.getline(name,100);
		sscanf(name,"%d, %d",&ID, &nPoint);
		if (nPoint < 1)
		{
			wxString xx= wxString::Format("at polygon-%d",ID);
			wxMessageBox(wxT("Fail in reading the Boundary file: "+xx));
			delete [] x;
			x = NULL;
			delete [] y;
			y = NULL;
			delete [] Parts;
			Parts = NULL;
			delete shape;
			shape = NULL;
			return false;
		}
		
		polyid += 1;
		
		
		long pt = 0;
		for (pt=0; pt < nPoint; pt++) 
		{
			double xt,yt;
			
			ias.getline(name, 100);
			tok = wxString(name,wxConvUTF8);
			//tok = wxString::Format("%100s",name);
			int pos = tok.Find(',');
			if( pos >= 0)
			{
				//xt = wxAtof(tok.Left(pos));
				tok.Left(pos).ToCDouble(&xt);
				tok = tok.Right(tok.Length()-pos-1);
				//yt = wxAtof(tok);
				tok.ToCDouble(&yt);
			}
			else
			{
				wxMessageBox("Wrong format!");
				return false;
			}
			x[pt] = xt; y[pt]= yt;
			Points.at(pt).setXY(x[pt],y[pt]);
		}
		Points.at(pt).setXY(x[0],y[0]);
		
		const double Area  = GenGeomAlgs::ComputeArea2D(nPoint,x,y); 
		const double Perim = GenGeomAlgs::ComputePerimeter2D(nPoint,x,y); 
		
		Box rBox(a,b);
		rBox = shape->SetData(nParts, Parts, nPoint+1, Points);
		fBox += rBox;
		Triple << *shape;
		Triple << ID;
		Triple << Area;
		Triple << Perim;
		Triple << polyid;
		
	}
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;
	delete [] Parts;
	Parts = NULL;
	delete shape;
	shape = NULL;
	return true;
}

bool CreateGridShapeFile(wxString otfl, int nRows, int nCols, double *xg, double *yg, myBox myfBox) 
{
	
	BasePoint p1(myfBox.p1.x,myfBox.p1.y);
	BasePoint p2(myfBox.p2.x,myfBox.p2.y);
	Box xoBox(p1,p2);
	double const EPS = 1e-10;
	
	int const nPolygon = nRows * nCols;
	
	DBF_descr *dbfdesc = CreateDBFDesc4Grid(wxEmptyString);
	oShapeFileTriplet Triple(otfl,xoBox, dbfdesc, 3,ShapeFileTypes::POLYGON);
	
	AbstractShape* shape = new PolygonShape;
	
	int nParts=1;
	int* Parts;
	Parts = new int[nParts];
	Parts[0]=0;
	
	BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
	BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
	Box fBox(a,b);
	
	std::vector<BasePoint> Points(6);
	
	if (shape == NULL)
		return false;
	
	long const nPoint = 4;
	double *x = new double[nPoint+2];
	double *y = new double[nPoint+2];
	
	long polyid = 0;
	for (long row = nRows; row >= 1; row--) 
	{
		
		for (long col=1; col <= nCols; col++) 
		{
			polyid += 1;
			
			x[0] = xg[col-1];
			y[0] = yg[row];
			x[1] = xg[col];
			y[1] = yg[row];
			x[2] = xg[col];
			y[2] = yg[row-1];
			x[3] = xg[col-1];
			y[3] = yg[row-1];
			Points[0].setXY(x[0],y[0]);
			Points[1].setXY(x[1],y[1]);
			Points[2].setXY(x[2],y[2]);
			Points[3].setXY(x[3],y[3]);
			Points[4].setXY(x[0],y[0]);
			
			const double Area  = GenGeomAlgs::ComputeArea2D(nPoint,x,y); 
			const double Perim = GenGeomAlgs::ComputePerimeter2D(nPoint,x,y); 
			
			Box rBox(a,b);
			rBox = shape->SetData(nParts, Parts, nPoint+1, Points);
			fBox += rBox;
			Triple << *shape;
			Triple << polyid;
			Triple << Area;
			Triple << Perim;
		}
	}
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;
	delete [] Parts;
	Parts = NULL;
	delete shape;
	shape = NULL;
	return true;
}

bool CreatePolygonShapeFile(char* otfl, char* infl, const int nPolygon,
							myPolygon* Polygons, myBox myfBox) 
{
	
	BasePoint p1(myfBox.p1.x,myfBox.p1.y);
	BasePoint p2(myfBox.p2.x,myfBox.p2.y);
	Box xoBox(p1,p2);
	
	
	bool* whatIn = new bool[256];
	int numCol=0, numDesc=0;
	DBF_descr *dbfdesc = CreateDBFDesc4Thiessen(infl, &numCol ,whatIn, &numDesc);
	
	oShapeFileTriplet Triple(otfl,xoBox, dbfdesc, numDesc,
							 ShapeFileTypes::POLYGON);
	
	AbstractShape* shape = new PolygonShape;
	
	int nParts=1; int* Parts;
	Parts = new int[nParts]; Parts[0]=0;
	
	BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
	BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
	Box fBox(a,b);
	
	std::vector<BasePoint> Points(100);
	
	if (shape == NULL)
		return false;
	
	
	iDBF tb(infl);
	
	for (long rec=0; rec < nPolygon; rec++) 
	{
		
		long nPoint = Polygons[rec].n, p = 0;
		double *x = new double[nPoint+2];
		double *y = new double[nPoint+2];
		
		for (p=0; p < nPoint; p++) 
		{
			x[p] = Polygons[rec].co[p].x;
			y[p] = Polygons[rec].co[p].y;
			Points.at(p).setXY(Polygons[rec].co[p].x,Polygons[rec].co[p].y);
		}
		
		const double Area = GenGeomAlgs::ComputeArea2D(nPoint,x,y);
		const double Perim = GenGeomAlgs::ComputePerimeter2D(nPoint,x,y);
		
		Box rBox(a,b);
		rBox = shape->SetData(nParts, Parts, nPoint, Points);
		fBox += rBox;
		const long polyid = rec+1;
		Triple << *shape;
		Triple << polyid;
		Triple << Area;
		Triple << Perim;
		
		char* m_dt;
		for (p =0; p < numCol; p++)
		{
			char ty = tb.GetFieldType(p); 
			if (ty == 'N' || ty == 'F' || ty == 'D') 
			{
				if(tb.GetFieldPrecision(p) > 0)
				{
					double dt;
					
					tb.Read(dt);
					wxString strText = wxString::Format("%f",dt);
					strText = strText.Left(tb.GetFieldSize(p));			
					
					if (whatIn[p]) 	Triple << strText;
					
					//						if (whatIn[p]) 	Triple << dt;
					
				}
				else
				{
					if (tb.GetFieldSize(p) < 10)
					{
						
						long dt;
						tb.Read(dt);
						if (whatIn[p]) 	Triple << dt;
					}
					else
					{
						double dt;
						tb.Read(dt);
						long const ldt = (long)dt;
						
						if (whatIn[p]) 	Triple << ldt;
					}
				}
			}
			else 
			{
				int len = tb.GetFieldSize(p);
				m_dt = new char[len+1];
				m_dt[len]='\x0';
				tb.Read(m_dt, len);
				if (whatIn[p]) 	Triple << m_dt;
				delete [] m_dt;
				m_dt = NULL;
			}
		}
		
	}
	
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	return true;
}


DBF_descr* CreateDBFDesc4Point(char* infl, int* numCol, bool* willbeIn,
							   int* numDesc)
{
	
	iDBF tb(infl);
	
	int m_row = tb.GetNumOfRecord();
	int m_col = tb.GetNumOfField();
	
	*numCol = m_col;
	
	DBF_descr *dbfdesc;
	dbfdesc			= new DBF_descr [m_col+3];
	dbfdesc[0]	= new DBF_field("POLYID", 'N',9,0);
	dbfdesc[1]	= new DBF_field("X_COORD_", 'N',16,5);
	dbfdesc[2]	= new DBF_field("Y_COORD_", 'N',16,5);
	
	
	int k = 3;
	for (int col = 0; col < m_col; col++)	
	{
		willbeIn[col] = true;
		wxString m_ColNm	= wxString::Format("%s",tb.GetFieldName(col));
		m_ColNm.Trim(false);
		m_ColNm.Trim(true);
		if (m_ColNm == "POLYID" || 
			m_ColNm == "X_COORD_" ||
			m_ColNm == "Y_COORD_")
		{
			willbeIn[col] = false;
		}
		else
		{
			int width = tb.GetFieldSize(col);
			
			
			const char ty = tb.GetFieldType(col);
			const int prec = tb.GetFieldPrecision(col);
			if (ty == 'N' && width - prec <= 1) width = width + 1;
			dbfdesc[k] = new DBF_field(m_ColNm, ty, width, prec);
			k++;
		}
	}
	
	*numDesc = k;
	
	return dbfdesc;
	
}



bool CreatePointShapeFile(char* otfl, char* infl, const int nPoint, 
						  std::vector<double>& x, std::vector<double>& y,
						  myBox myfBox) 
{
	
	BasePoint p1(myfBox.p1.x,myfBox.p1.y);
	BasePoint p2(myfBox.p2.x,myfBox.p2.y);
	Box xoBox(p1,p2);
	
	bool* whatIn = new bool[256];
	int numCol=0, numDesc=0;
	
	DBF_descr *dbfdesc = CreateDBFDesc4Point(infl, &numCol ,whatIn, &numDesc);
	
	oShapeFileTriplet Triple(otfl,xoBox, dbfdesc, numDesc, ShapeFileTypes::SPOINT);
	
	
	AbstractShape* shape = new Ppoint;
	
	double max_x=x.at(0), max_y=y.at(0), min_x=x.at(0), min_y=y.at(0);
	
	iDBF tb(infl);
	for (long rec=0; rec < nPoint; rec++) 
	{		
		shape->AssignPointData(x.at(rec),y.at(rec));
		Triple << *shape;
		const long polyid = rec+1;
		
		Triple << polyid;
		Triple << x.at(rec);
		Triple << y.at(rec);
		
		char* m_dt;
		for (int p =0; p < numCol; p++)
		{
			char ty = tb.GetFieldType(p); 
			if (ty == 'N' || ty == 'F' || ty == 'D') 
			{
				if(tb.GetFieldPrecision(p) > 0)
				{
					double dt;
					
					tb.Read(dt);
					wxString strText = wxString::Format("%f",dt);
					strText = strText.Left(tb.GetFieldSize(p));
					if (whatIn[p]) 	Triple << strText;
					
				}
				else
				{
					if (tb.GetFieldSize(p) < 10)
					{
						
						long dt;
						tb.Read(dt);
						if (whatIn[p]) 	Triple << dt;
					}
					else
					{
						double dt;
						tb.Read(dt);
						long const ldt = (long)dt;
						
						if (whatIn[p]) 	Triple << ldt;
					}
				}
			}
			else 
			{
				
				int len = tb.GetFieldSize(p);
				m_dt = new char[len+1];
				m_dt[len]='\x0';
				tb.Read(m_dt, len);
				if (whatIn[p]) 	Triple << m_dt;
				delete [] m_dt;
				m_dt = NULL;
			}
		}
		if (max_x < x.at(rec)) max_x = x.at(rec);
		if (min_x > x.at(rec)) min_x = x.at(rec);
		if (max_y < y.at(rec)) max_y = y.at(rec);
		if (min_y > y.at(rec)) min_y = y.at(rec);	
		
	}
	
	tb.close();
	
	double delta_x = (max_x - min_x)/20;
	double delta_y = (max_y - min_y)/20;
	
	min_x = min_x - delta_x;
	min_y = min_y - delta_y;
	max_x = max_x + delta_x;
	max_y = max_y + delta_y;
	
	
	BasePoint a(min_x,min_y);
	BasePoint b(max_x,max_y);
	Box fBox(a,b);
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	
	delete [] whatIn;
	whatIn = NULL;
	delete shape;
	
	return true;
}

bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* &B, bool mean_center,
			   const wxString& boundingFileName)
{
	LOG_MSG("Entering ComputeXY ver 1");
	// input:  an ESRI point/polygon shape file
	// output: - nPoints, # of points
	//         - vector of x, and y
	//         - B, the bounding box
	
	if (fname.IsEmpty() ||
		boundingFileName.IsEmpty() ||
		x.empty() || y.empty() ) {
		return false;
	}
	
	iShapeFile    shx(fname, "shx");
	char          hsx[ 2*GeoDaConst::ShpHeaderSize ];
	shx.read((char *) &hsx[0], 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hdx(hsx);
	long          offset, contents, *OffsetIx;
	long n = (hdx.Length() - GeoDaConst::ShpHeaderSize) / 4;
	OffsetIx= new long [ n ];
	
	if (n < 1 || OffsetIx == NULL)
		return false;
	
	for (long rec= 0; rec < n; ++rec)  {
		offset= ReadBig(shx);
		contents= ReadBig(shx);
		offset *= 2;
		OffsetIx[rec]= offset;
	}
	shx.close();
	
	myBox B1;
	
	iShapeFile    shp(fname, "shp");
	char          hs[ 2*GeoDaConst::ShpHeaderSize ];
	shp.read(hs, 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hd(hs);
	Box m_BigBox1 = hd.BoundingBox();
	
	//BEGIN code not in ver 2
	iShapeFile     shp_ref(boundingFileName, "shp");
	char           hs_ref[ 2*GeoDaConst::ShpHeaderSize ];
	shp_ref.read(hs_ref, 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr         hd_ref(hs_ref);
	Box m_BigBox2 = hd_ref.BoundingBox();
	//END code not in ver 2
	
	if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::POLYGON)  
	{
		
		Box bBox= hd.BoundingBox(); // The extent of the file 
		BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
		BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
		
		Box BBox(a,b), pBox;
		BasePoint * centroid= new BasePoint [ n];
		if (centroid == NULL)
			return false;
		
		for (long rec= 0; rec < n; ++rec)  
		{
			shp.seekg(OffsetIx[rec]+12, ios::beg);
#ifdef WORDS_BIGENDIAN
			char r[32], p;
			double m1, m2, n1, n2;
			shp.read((char *)r, sizeof(double) * 4);
			SWAP(r[0], r[7], p);
			SWAP(r[1], r[6], p);
			SWAP(r[2], r[5], p);
			SWAP(r[3], r[4], p);
			memcpy(&m1, &r[0], sizeof(double));
			SWAP(r[8], r[15], p);
			SWAP(r[9], r[14], p);
			SWAP(r[10], r[13], p);
			SWAP(r[11], r[12], p);
			memcpy(&m2, &r[8], sizeof(double));
			SWAP(r[16], r[23], p);
			SWAP(r[17], r[22], p);
			SWAP(r[18], r[21], p);
			SWAP(r[19], r[20], p);
			memcpy(&n1, &r[16], sizeof(double));
			SWAP(r[24], r[31], p);
			SWAP(r[25], r[30], p);
			SWAP(r[26], r[29], p);
			SWAP(r[27], r[28], p);
			memcpy(&n2, &r[24], sizeof(double));
			BasePoint p1 = BasePoint(m1, m2);
			BasePoint p2 = BasePoint(n1, n2);
			pBox = Box(p1, p2);
#else
			shp >> pBox;
#endif
			BBox += pBox;
			shp.seekg(OffsetIx[rec]+12, ios::beg);
			
			BoundaryShape t;
			t.ReadShape(shp);
			
			if (mean_center || t.GetNumParts() > 1) {
				if (mean_center && t.GetNumParts() > 1) {	
					LOG_MSG("Warning: MeanCenter being returned rather than Centroid due to multi-part polygon");
				}
				centroid[rec] = t.MeanCenter();
			}
			else
				centroid[rec] = t.Centroid();
			
			x.at(rec) = centroid[rec].x;
			y.at(rec) = centroid[rec].y;  
		} 
		
		B->p1.x = BBox._min().x;
		B->p1.y = BBox._min().y;
		B->p2.x = BBox._max().x;
		B->p2.y = BBox._max().y;
		
		//BEGIN code not in ver 2		
		B->p1.x = m_BigBox1._min().x;
		if (m_BigBox2._min().x < m_BigBox1._min().x)
			B->p1.x = m_BigBox2._min().x;
		if (BBox._min().x < m_BigBox2._min().x)
			B->p1.x = BBox._min().x;
		
		B->p2.x = m_BigBox1._max().x;
		if (m_BigBox2._max().x > m_BigBox1._max().x)
			B->p2.x = m_BigBox2._max().x;
		if (BBox._max().x > m_BigBox2._max().x)
			B->p2.x = BBox._max().x;
		
		B->p1.y = m_BigBox1._min().y;
		if (m_BigBox2._min().y < m_BigBox1._min().y)
			B->p1.y = m_BigBox2._min().y;
		if (BBox._min().y < m_BigBox2._min().y)
			B->p1.y = BBox._min().y;
		
		B->p2.y = m_BigBox1._max().y;
		if (m_BigBox2._max().y > m_BigBox1._max().y)
			B->p2.y = m_BigBox2._max().y;
		if (BBox._max().y > m_BigBox2._max().y)
			B->p2.y = BBox._max().y;
		//BEGIN code not in ver 2
		
		//		delete [] centroid;
		//		centroid = NULL;
		
	}
	else if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::SPOINT)
	{// POINT
		
		
		Box bBox= hd.BoundingBox();
		long rec;		  
		BasePoint *centroid= new BasePoint [ n];
		
		if (centroid == NULL)
			return false;
		
		AbstractShape *shape;
		shp.Recl(hd.FileShape());
		shape = new Ppoint;
		shape->ReadShape(shp);
		centroid[ 0 ] = shape->Centroid();
		//		delete shape;
		
		x.at(0) = B1.p1.x = B1.p2.x = centroid[0].x;
		y.at(0) = B1.p1.y = B1.p2.y = centroid[0].y;
		
		
		for (rec= 1; rec < n; ++rec)  
		{
			shp.Recl(hd.FileShape());
			shape = new Ppoint;
			shape->ReadShape(shp);
			centroid[ rec ] = shape->Centroid();
			
			x.at(rec) = centroid[rec].x;
			y.at(rec) = centroid[rec].y;
			//			delete shape;
			
			if (centroid[rec].x > B1.p2.x) B1.p2.x = centroid[rec].x;
			else if (centroid[rec].x < B1.p1.x) B1.p1.x = centroid[rec].x;
			if (centroid[rec].y > B1.p2.y) B1.p2.y = centroid[rec].y;
			else if (centroid[rec].y < B1.p1.y) B1.p1.y = centroid[rec].y;
		}
		
		double rx = (B1.p2.x - B1.p1.x)/100.0;
		double ry = (B1.p2.y - B1.p1.y)/100.0;
		B1.p1.x = B1.p1.x - rx;
		B1.p1.y = B1.p1.y - ry;
		B1.p2.x = B1.p2.x + rx;
		B1.p2.y = B1.p2.y + ry;
		
		//BEGIN code not in ver 2
		B->p1.x = m_BigBox1._min().x;
		if (m_BigBox2._min().x < m_BigBox1._min().x)
			B->p1.x = m_BigBox2._min().x;
		
		B->p2.x = m_BigBox1._max().x;
		if (m_BigBox2._max().x > m_BigBox1._max().x)
			B->p2.x = m_BigBox2._max().x;
		
		B->p1.y = m_BigBox1._min().y;
		if (m_BigBox2._min().y < m_BigBox1._min().y)
			B->p1.y = m_BigBox2._min().y;
		
		B->p2.y = m_BigBox1._max().y;
		if (m_BigBox2._max().y > m_BigBox1._max().y)
			B->p2.y = m_BigBox2._max().y;
		
		// checking if the box is less then the actual area (B1)
		if (B->p1.x > B1.p1.x)
			B->p1.x = B1.p1.x;
		if (B->p1.y > B1.p1.y)
			B->p1.y = B1.p1.y;
		if (B->p2.x < B1.p2.x)
			B->p2.x = B1.p2.x;
		if (B->p2.y < B1.p2.y)
			B->p2.y = B1.p2.y;
		//END code not in ver 2
		
		//		delete [] centroid;
		//		centroid = NULL;
		
	}
	nPoints = &n;
	shp.close();
	
	//	delete [] OffsetIx;
	//	OffsetIx = NULL;
	
	LOG_MSG("Exiting ComputeXY ver 1");
	return true;
}

bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* B, bool mean_center)
{
	LOG_MSG("Entering ComputeXY ver 2");
	// input:  an ESRI point/polygon shape file
	// output: - nPoints, # of points
	//         - vector of x, and y
	//         - B, the bounding box
	
	if (fname.IsEmpty() ||
		x.empty() || y.empty() )
		return false;
	
	iShapeFile    shx(fname, "shx");
	char          hsx[ 2*GeoDaConst::ShpHeaderSize ];
	shx.read((char *) &hsx[0], 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hdx(hsx);
	long          offset, contents, *OffsetIx;
	long n = (hdx.Length() - GeoDaConst::ShpHeaderSize) / 4;
	OffsetIx= new long [ n ];
	
	if (n < 1 || OffsetIx == NULL)
		return false;
	
	for (long rec= 0; rec < n; ++rec)  {
		offset= ReadBig(shx);
		contents= ReadBig(shx);
		offset *= 2;
		OffsetIx[rec]= offset;
	}
	shx.close();
	
	myBox B1;
	
	iShapeFile    shp(fname, "shp");
	char          hs[ 2*GeoDaConst::ShpHeaderSize ];
	shp.read(hs, 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hd(hs);
	Box m_BigBox = hd.BoundingBox();
	
	if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::POLYGON)  
	{
		
		Box bBox= hd.BoundingBox();
		BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
		BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
		
		Box BBox(a,b), pBox;
		BasePoint * centroid= new BasePoint [ n];
		if (centroid == NULL)
			return false;
		
		for (long rec= 0; rec < n; ++rec)  
		{
			shp.seekg(OffsetIx[rec]+12, ios::beg);
#ifdef WORDS_BIGENDIAN
			char r[32], p;
			double m1, m2, n1, n2;
			shp.read((char *)r, sizeof(double) * 4);
			SWAP(r[0], r[7], p);
			SWAP(r[1], r[6], p);
			SWAP(r[2], r[5], p);
			SWAP(r[3], r[4], p);
			memcpy(&m1, &r[0], sizeof(double));
			SWAP(r[8], r[15], p);
			SWAP(r[9], r[14], p);
			SWAP(r[10], r[13], p);
			SWAP(r[11], r[12], p);
			memcpy(&m2, &r[8], sizeof(double));
			SWAP(r[16], r[23], p);
			SWAP(r[17], r[22], p);
			SWAP(r[18], r[21], p);
			SWAP(r[19], r[20], p);
			memcpy(&n1, &r[16], sizeof(double));
			SWAP(r[24], r[31], p);
			SWAP(r[25], r[30], p);
			SWAP(r[26], r[29], p);
			SWAP(r[27], r[28], p);
			memcpy(&n2, &r[24], sizeof(double));
			BasePoint p1 = BasePoint(m1, m2);
			BasePoint p2 = BasePoint(n1, n2);
			pBox = Box(p1, p2);
#else
			shp >> pBox;
#endif
			BBox += pBox;
			shp.seekg(OffsetIx[rec]+12, ios::beg);
			
			BoundaryShape t;
			t.ReadShape(shp);
			
			if (mean_center || t.GetNumParts() > 1) {
				if (mean_center && t.GetNumParts() > 1) {	
					LOG_MSG("Warning: MeanCenter being returned rather than Centroid due to multi-part polygon");
				}
				centroid[rec] = t.MeanCenter();
			}
			else
				centroid[rec] = t.Centroid();
			
			x.at(rec) = centroid[rec].x;
			y.at(rec) = centroid[rec].y;  
		}
		
		B->p1.x = BBox._min().x;
		B->p1.y = BBox._min().y;
		B->p2.x = BBox._max().x;
		B->p2.y = BBox._max().y;
		
		//		delete [] centroid;
		//		centroid = NULL;
		
	}
	else if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::SPOINT)
	{// POINT
		
		
		Box bBox= hd.BoundingBox();
		long rec;		   
		BasePoint *centroid= new BasePoint [ n];
		
		if (centroid == NULL)
			return false;
		
		AbstractShape        *shape;
		shp.Recl(hd.FileShape());
		shape = new Ppoint;
		shape->ReadShape(shp);
		centroid[ 0 ] = shape->Centroid();
		//		delete shape;
		
		x.at(0) = B1.p1.x = B1.p2.x = centroid[0].x;
		y.at(0) = B1.p1.y = B1.p2.y = centroid[0].y;
		
		
		for (rec= 1; rec < n; ++rec)  
		{
			shp.Recl(hd.FileShape());
			shape = new Ppoint;
			shape->ReadShape(shp);
			centroid[ rec ] = shape->Centroid();
			
			x.at(rec) = centroid[rec].x;
			y.at(rec) = centroid[rec].y;
			//			delete shape;
			
			if (centroid[rec].x > B1.p2.x) B1.p2.x = centroid[rec].x;
			else if (centroid[rec].x < B1.p1.x) B1.p1.x = centroid[rec].x;
			if (centroid[rec].y > B1.p2.y) B1.p2.y = centroid[rec].y;
			else if (centroid[rec].y < B1.p1.y) B1.p1.y = centroid[rec].y;
		}
		
		double rx = (B1.p2.x - B1.p1.x)/100.0;
		double ry = (B1.p2.y - B1.p1.y)/100.0;
		B->p1.x = B1.p1.x - rx;
		B->p1.y = B1.p1.y - ry;
		B->p2.x = B1.p2.x + rx;
		B->p2.y = B1.p2.y + ry;
		
		//		delete [] centroid;
		//		centroid = NULL;
		
	}
	nPoints = &n;
	shp.close();
	
	//	delete [] OffsetIx;
	//	OffsetIx = NULL;
	
	LOG_MSG("Exiting ComputeXY ver 2");
	return true;
}


BasePoint * MakeCentroids(char *fname, long Records)  
{
	LOG_MSG("Entering MakeCentroids");
	iShapeFile   shp(wxString(fname), "shp");
	char         hs[ 2*GeoDaConst::ShpHeaderSize ];
	
	shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
	ShapeFileHdr       head(hs);
	long rec;
	
	BasePoint *      centroid= new BasePoint [ Records ];
	if (centroid == NULL) return NULL;
	AbstractShape        *shape;
	
	for (rec= 0; rec < Records; ++rec)  {
		shp.Recl(head.FileShape());
		switch (head.FileShape())  {
			case ShapeFileTypes::SPOINT :
				LOG_MSG("SPOINT shape type read.");
				shape= new Ppoint;
				break;
			case ShapeFileTypes::MULTIPOINT :
				LOG_MSG("MULTIPOINT shape type read.");
				shape= new MultiPoint;
				break;
			case ShapeFileTypes::ARC :
				LOG_MSG("ARC shape type read.  Warning: unsupported!");
				wxMessageBox("ERROR: ARC shape type detected but not supported!");
				//shape= new Shape;
				break;
			case ShapeFileTypes::POLYGON :
				LOG_MSG("POLYGON shape type read.");
				shape= new PolygonShape;
				break;
			default :
				LOG_MSG("unknown shape type read!");
				wxMessageBox("ERROR: unknown shape type detected!");
				break;
		}
		
		shape->ReadShape(shp);
		centroid[ rec ] = shape->Centroid();
		
		delete shape;
	}
	
	LOG_MSG("Exiting MakeCentroids");
	
	return centroid;
}

bool WriteGwt(
			  const GwtElement *g, 
			  const wxString& ifname, 
			  const wxString& ofname, 
			  const wxString& idd, 
			  const long Obs,
			  const int degree_flag,
			  bool geodaL)  
{
    if (g == NULL || ifname.IsEmpty() || ofname.IsEmpty() || idd.IsEmpty())
        return false;
	
    wxFileName galfn(ofname);
    galfn.SetExt("gwt");
    wxString gal_ofn(galfn.GetFullPath());
    std::ofstream out(gal_ofn.mb_str(wxConvUTF8), std::ios::out);
    if (!(out.is_open() && out.good())) {
        return false;
    }
	
    long *ids = new long[Obs];
    wxFileName dbf_name(ifname);
    dbf_name.SetExt("dbf");
    if (!ReadId(dbf_name.GetFullPath(), idd, ids)) {
        if (ids) delete [] ids; ids = NULL;
        return false;
    }
	
    // IS; 24/8/08
    int degree_fl = geodaL ? 0 : degree_flag ;
    wxFileName local(ifname);
    std::string local_name(local.GetName().mb_str(wxConvUTF8));
    out << degree_fl << " " << Obs << " " << local_name;
    out << " " << idd.mb_str() << endl;
    
    for (int i=0; i < Obs; i++) {
        for (long nbr= 0; nbr < g[i].Size(); ++nbr) {
            GwtNeighbor  current= g[i].elt(nbr);
            double w = current.weight;
            out << ids[i] << ' ' << ids[current.nbx]
			<< ' ' << setprecision(9) << setw(18) << w << '\n';
        }
    }
	
    if (ids) delete [] ids; ids = NULL;
    return true;
}

GwtElement * MakeFullGwt(GwtElement * half, const long dim, int degree,
						 bool standardize)  
{
	long * Count = new long [ dim ], cnt, nbr, Nz= 0;
	for (cnt= 0; cnt < dim; ++cnt)
		Count[cnt] = half[cnt].Size();
	for (cnt= 0; cnt < dim; ++cnt)
		for (nbr= 0; nbr < half[cnt].Size(); ++nbr)
			++Count[ half[cnt].elt(nbr).nbx ];
	GwtElement *tmp = new GwtElement [dim], * full= new GwtElement [ dim ];
	bool good= true;
	double min = 1e10, *sum = new double [dim];
	
	for (cnt= 0; cnt < dim; ++cnt)  {
		if (Count[cnt]) {
			good &= full[cnt].alloc( Count[cnt] );
			good &= tmp[cnt].alloc(Count[cnt]);
		}
		Nz += Count[cnt];
		for (nbr = 0; nbr < half[cnt].Size(); nbr++)
		{
			if (min > half[cnt].elt(nbr).weight)
			{
				min = half[cnt].elt(nbr).weight;
			}
		}
	}
	
	if (good)  {
		for (cnt= 0; cnt < dim; ++cnt)
			for (nbr= 0; nbr < half[cnt].Size(); ++nbr)  {
				GwtNeighbor cx= half[cnt].elt(nbr);
				if (degree < 0) cx.weight = pow(cx.weight / min, degree);
				else cx.weight = pow(cx.weight, degree);
				tmp[cnt].Push( cx );
				tmp[cx.nbx].Push( GwtNeighbor(cnt, cx.weight) );
			}
		
		if (standardize) {
			for (cnt = 0; cnt < dim; cnt++)
			{
				sum[cnt] = 0;
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					sum[cnt] += cx.weight * cx.weight;
				}
			}
			
			for (cnt = 0; cnt < dim; cnt++)
			{
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					cx.weight /= sqrt(sum[cnt]);
					full[cnt].Push(cx);
				}
			}
			
		}
		else
		{
			for (cnt = 0; cnt < dim; cnt++)
			{
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					full[cnt].Push(cx);
				}
			}
		}
	}  else {
		delete [] full;
		full = NULL;
	}
	delete [] tmp;
	tmp = NULL;
	delete [] Count;
	Count = NULL;
	// cout << " nonzero elements: " << Nz << endl;
	return full;
}



long Isolated(const GwtElement *gwt, const long dim)  
{
	long iso= 0;
	for (long cnt= 0; cnt < dim; ++cnt)
		if (gwt[cnt].Size() == 0) ++iso;
	return iso;
}


inline void swap(PartitionPtr &x, PartitionPtr &y)  {
	PartitionPtr z= x;
	x= y;
	y= z;
	return;
}

GwtElement* shp2gwt(int Obs, 
					std::vector<double>& x,
					std::vector<double>& y,
					const double threshold, 
					const int degree,
					int	method) // 0: Euclidean dist, 1:Arc
{
	long Records = Obs, cnt;
	
	BasePoint *m_pt = new BasePoint[Obs];
	if (m_pt == NULL) return NULL;
	
	for (int i=0;i < Obs; i++)
		m_pt[i].setXY(x.at(i),y.at(i));
	
	Location Center(m_pt, threshold, Records, method);
	
	if (!Center.good()) return NULL;  
	
	long gx, gy, part, curr;
	gx = Center.x_cells();
	gy = Center.y_cells();
	
	if (gx > 4*Records || gy > 4*Records) return NULL;
	
	PartitionGWT gX(Records, gx, Center.x_range(method));
	for (cnt= 0; cnt < Records; ++cnt)       // fill in x-partition
		gX.include ( cnt, Center.x_range(cnt, method) );
	
	PartitionGWT *A	= new PartitionGWT(Records, gy, Center.y_range(method)),
	*B	= new PartitionGWT(Records, gy, Center.y_range(method));
	
	GwtNeighbor * buffer	= new GwtNeighbor[ Records];
	GwtElement * GwtHalf	= new GwtElement[ Records];
	long BufferSize= 0, included;
	
	for (part= 0; part < gx; ++part)  
	{      // processing all elements along (part, y)
		included= 0;
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr), ++included)
			A->include( curr, Center.y_range(curr, method) );
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr))  
		{
			long cell= A->Where( Center.y_range(curr, method) );
			Center.setCurrent(curr);
			if (cell > 0)  
			{
				Center.CheckParticle(B, cell-1, BufferSize, buffer);
				Center.CheckParticle(A, cell-1, BufferSize, buffer);
			};
			Center.CheckParticle(B, cell, BufferSize, buffer);
			Center.CheckParticle(A, cell, BufferSize, buffer);
			if (++cell < gy)  
			{
				Center.CheckParticle(A, cell, BufferSize, buffer);
				Center.CheckParticle(B, cell, BufferSize, buffer);
			};
			GwtHalf[curr].alloc(BufferSize);
			while (BufferSize)
				GwtHalf[curr].Push(buffer[--BufferSize]);
		};
		if (part > 0 && included > 0)  
		{
			if (4*included > gy)         // it's less expensive to reset all cells in the partition
				B->reset();
			else                         // it's less expensive to reset only used cells
				for(curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr))
					B->reset(Center.y_range(curr, method));
		};
		swap(A, B); // swap A and B -- sweep by; now A is empty, B contains current line
	};
	delete  A;
	delete  B;
	GwtElement * GwtFull= MakeFullGwt(GwtHalf, Records, degree, false);
	delete [] GwtHalf;
	GwtHalf = NULL;
	delete [] buffer;
	buffer = NULL;
	
	return GwtFull;
}

GwtElement* inv2gwt(int Obs, 
					std::vector<double>& x,
					std::vector<double>& y,
					const double threshold, 
					const int degree,
					int	method, // 0: Euclidean dist, 1:Arc
					bool standardize)
{
	long Records = Obs, cnt;
	
	BasePoint *m_pt = new BasePoint[Obs];
	if (m_pt == NULL)
		return NULL;
	
	for (int i=0;i < Obs; i++)
		m_pt[i].setXY(x.at(i),y.at(i));
	
	
	Location Center(m_pt, threshold, Records, method);
	
	if (!Center.good())  
		return NULL;  
	
	long gx, gy, part, curr;
	gx = Center.x_cells();
	gy = Center.y_cells();
	
	
	if (gx > 4*Records || gy > 4*Records)  
		return NULL;
	
	PartitionGWT gX(Records, gx, Center.x_range(method));
	for (cnt= 0; cnt < Records; ++cnt)       // fill in x-partition
		gX.include ( cnt, Center.x_range(cnt, method) );
	
	PartitionGWT *A	= new PartitionGWT(Records, gy, Center.y_range(method)),
	*B	= new PartitionGWT(Records, gy, Center.y_range(method));
	
	GwtNeighbor * buffer	= new GwtNeighbor[ Records];
	GwtElement * GwtHalf	= new GwtElement[ Records];
	long BufferSize= 0, included;
	
	for (part= 0; part < gx; ++part)  
	{      // processing all elements along (part, y)
		included= 0;
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr), ++included)
			A->include( curr, Center.y_range(curr, method) );
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr))  
		{
			long cell= A->Where( Center.y_range(curr, method) );
			Center.setCurrent(curr);
			if (cell > 0)  
			{
				Center.CheckParticle(B, cell-1, BufferSize, buffer);
				Center.CheckParticle(A, cell-1, BufferSize, buffer);
			};
			Center.CheckParticle(B, cell, BufferSize, buffer);
			Center.CheckParticle(A, cell, BufferSize, buffer);
			if (++cell < gy)  
			{
				Center.CheckParticle(A, cell, BufferSize, buffer);
				Center.CheckParticle(B, cell, BufferSize, buffer);
			};
			GwtHalf[curr].alloc(BufferSize);
			while (BufferSize)
				GwtHalf[curr].Push(buffer[--BufferSize]);
		};
		if (part > 0 && included > 0)  
		{
			if (4*included > gy)         // it's less expensive to reset all cells in the partition
				B->reset();
			else                         // it's less expensive to reset only used cells
				for(curr= gX.first(part); curr != GeoDaConst::EMPTY; curr= gX.tail(curr))
					B->reset(Center.y_range(curr, method));
		};
		swap(A, B); // swap A and B -- sweep by; now A is empty, B contains current line
	};
	delete  A;
	delete  B;
	GwtElement * GwtFull= MakeFullGwt(GwtHalf, Records, -degree, standardize);
	delete [] GwtHalf;
	GwtHalf = NULL;
	delete [] buffer;
	buffer = NULL;
	
	return GwtFull;
}

#include "../kNN/ANN.h"			// ANN declarations
GwtElement *DynKNN(const wxString& infl, int k, long obs,
				   int method, char *v1, char *v2, bool mean_center)
{
	if (obs	< 3	|| infl.IsEmpty() || k < 1 || k > obs)
		return NULL;
	
	long nPoints; myBox* B=new myBox;
	nPoints = obs;
	
	std::vector<double> x(nPoints);
	std::vector<double> y(nPoints);
	
	if (v1 != NULL && v2 != NULL) 
	{
		if (!ReadData(infl, v1, x)) return NULL;
		if (!ReadData(infl, v2, y)) return NULL;
	}
	else if (v1 != NULL)
	{
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center))
			return NULL;
		if (!ReadData(infl, v1, x)) return NULL;
	}
	else if (v2 != NULL)
	{
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center))
			return NULL;
		if (!ReadData(infl, v2, y)) return NULL;
	}
	else
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center))
			return NULL;
	
	int		dim		= 2;		// dimension
	ANNpointArray	data_pts;		// data points
	ANNpoint			query_pt;		// query point
	ANNidxArray		nn_idx;			// near neighbor indices
	ANNdistArray	dists;			// near neighbor distances
	ANNkd_tree		*the_tree;		// search structure
	
	GwtElement *gwt = new GwtElement[obs];
	if (gwt == NULL)
		return NULL;
	
	query_pt	= annAllocPt(dim);			// allocate query point
	data_pts	= annAllocPts(obs, dim);	// allocate data points
	nn_idx		= new ANNidx[k];			// allocate near neigh indices
	dists			= new ANNdist[k];			// allocate near neighbor dists
	int i = 0;
	for (i=0;i<obs; i++) {
		data_pts[i][0] = x.at(i);
		data_pts[i][1] = y.at(i);
	}
	
	the_tree = new ANNkd_tree(data_pts,obs,dim);
	
	for (i=0; i<obs; i++) {
		the_tree->annkSearch(data_pts[i], k, nn_idx, dists,0.0, method);
		gwt[i].alloc(k-1);
		for (int j=1; j<k; j++) {
			GwtNeighbor e;
			e.nbx = nn_idx[j];
			e.weight = sqrt(dists[j]);  // annkSearch returns each distance
			// squared, so take sqrt.
			gwt[i].Push(e);
		}
	}
	return gwt;
}

double ComputeCutOffPoint(const wxString& infl, 
						  long obs, 
						  int method, 
						  const wxString& t_v1, 
						  const wxString& t_v2, 
						  std::vector<double>& x, 
						  std::vector<double>& y,
						  bool mean_center)
{
	if (obs < 3 || infl.IsEmpty()) return 0.0;
	
	long nPoints; myBox* B=new myBox;
	nPoints = obs;
	int i = 0;
	
	if (!t_v1.IsEmpty() && !t_v2.IsEmpty()) {
		if (!ReadData(infl, t_v1, x)) return 0;
		if (!ReadData(infl, t_v2, y)) return 0;
	} else if (!t_v1.IsEmpty()) {
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center)) return 0;
		if (!ReadData(infl, t_v1, x)) return 0;
	} else if (!t_v2.IsEmpty()) {
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center)) return 0;
		if (!ReadData(infl, t_v2, y)) return 0;
	} else {
		if (!ComputeXY(infl, &nPoints, x, y, B, mean_center)) return 0;
	}
	
	int	dim	= 2;		// dimension
	ANNpointArray	data_pts;		// data points
	ANNpoint		query_pt;		// query point
	ANNidxArray		nn_idx;			// near neighbor indices
	ANNdistArray	dists;			// near neighbor distances
	ANNkd_tree		*the_tree;		// search structure
	
	int k = 2;
	query_pt = annAllocPt(dim);			// allocate query point
	data_pts = annAllocPts(obs, dim);	// allocate data points
	nn_idx	 = new ANNidx[k];			// allocate near neigh indices
	dists	 = new ANNdist[k];			// allocate near neighbor dists
	
	for (i=0; i<obs; i++) {
		data_pts[i][0] = x[i];
		data_pts[i][1] = y[i];
	}
	
	the_tree = new ANNkd_tree(data_pts,obs,dim);
	
	the_tree->annkSearch(data_pts[0], k, nn_idx, dists,0.0, method);
	double minDist = sqrt(dists[1]);
	
	int p1 = 0, p2 = 1;
	for (i=1; i<obs; i++) {
		the_tree->annkSearch(data_pts[i], k, nn_idx, dists,0.0, method);
		if (minDist <  sqrt(dists[1])) {
			minDist = sqrt(dists[1]);
			p1 = i;
			p2 = nn_idx[1];
		}
	}
	if (method==1) {
		minDist = GenGeomAlgs::ComputeEucDist(data_pts[p1][0], data_pts[p1][1],
											  data_pts[p2][0], data_pts[p2][1]);
	} else {
		minDist = GenGeomAlgs::ComputeArcDist(data_pts[p1][0], data_pts[p1][1],
											  data_pts[p2][0],data_pts[p2][1]);
	}
	
	delete the_tree;
	
	return minDist;
}

#include <map>

GwtElement* ReadTxtGwt(const char *fname, long gObs,
					   const wxString& full_dbf_name)  
{
	char    fn[256];
	strcpy(fn, fname);
	const int   lengthName= strlen(fn);
	fn[ lengthName ] = '\x0';         // cut out the extension. "shp"
	
	ifstream    ifl(fname, ios::in);
	if (ifl.fail()) {
		wxMessageBox("Error: Could not open file");
		return NULL;
	}
	
	long obs;
	int file_type;
	ifl >> file_type;
	
	wxString unique_id_msg;
	unique_id_msg = "Warning, your weights file relies on record order ";
	unique_id_msg += "rather than an ID Variable.  Please consider ";
	unique_id_msg += "generating a new weights file that uses an ID ";
	unique_id_msg += "Variable.";	
	// YT
	// Note that the type flags of gwt files can not be values except 0, 1, 2 and -2.
	
	// Reading direct Indices
	if ((file_type != 0) && (file_type != 1) &&
		(file_type != 2) && (file_type != -2)) 
	{  // type is # of observation
		wxMessageBox(unique_id_msg);
		if (file_type == gObs) 
		{
			GwtElement *mx = new GwtElement [ gObs ];
			long id1, id2, temp; double dist; int size;
			GwtNeighbor *elt = new GwtNeighbor[gObs];
			int cnt = 0;
			ifl >> id1 >> id2 >> dist;
			if (id1 > gObs || id2 > gObs)
			{
				wxMessageBox("Error: Some of indices are \n"
							 "larger than # of records");
				return NULL;
			}
			
			while (!ifl.eof()) 
			{
				temp = id1;
				size = 0;
				while (temp==id1 && !ifl.eof()) 
				{
					elt[size].nbx = id2-1;
					elt[size].weight = dist;
					ifl >> id1 >> id2 >> dist;
					size++;
					if (id1 > gObs || id2 > gObs)
					{
						wxMessageBox("Error: Some of indices are \n"
									 "larger than # of records!");
						return NULL;
					}
				}
				
				for (;cnt < temp-1;cnt++) 
				{
					mx[cnt].alloc(0);
					//					mx[cnt].nbrs = -1;
					mx[cnt].nbrs = 0;
				}
				
				mx[temp-1].alloc(size);
				for (int i=0; i<size;i++) mx[temp-1].Push(elt[i]);
				mx[temp-1].nbrs = size;
				cnt = temp;
			}
			delete [] elt;
			elt = NULL;
			return mx;
		}
		else
		{
			wxMessageBox("Error: Could not open file");
			return NULL;
		}
	}
	
	char flname[GeoDaConst::FileNameLen], varname[40];
	ifl >> obs >> flname >> varname;
	
	if (obs != gObs) 
	{
		wxMessageBox("Error: Number of observations in weights file doesn't "
					 "match currently chosen DBF file.");
		return NULL; 
	}
	
	// Reading direct Indices, with var = rec_num
	wxString s = wxString::Format("%s",varname);
	if (s == "rec_num") 
	{
		wxMessageBox(unique_id_msg);
		
		GwtElement *mx = new GwtElement [ gObs ];
		long id1, id2, temp; double dist; int size;
		GwtNeighbor *elt = new GwtNeighbor[gObs];
		int cnt=0;
		ifl >> id1 >> id2 >> dist;
		if (id1 > gObs || id2 > gObs)
		{
			wxMessageBox("Error: Some of indices are \n"
						 "larger than # of records!");
			return NULL;
		}
		
		while (!ifl.eof()) 
		{
			temp = id1;
			size = 0;
			while (temp==id1 && !ifl.eof()) 
			{
				elt[size].nbx = id2-1;
				elt[size].weight = dist;
				ifl >> id1 >> id2 >> dist;
				size++;
				if (id1 > gObs || id2 > gObs)
				{
					wxMessageBox("Error: Some of indices are \n"
								 "larger than # of records");
					return NULL;
				}
			}
			for (;cnt < temp-1;cnt++) {
				mx[cnt].alloc(0);
				//					mx[cnt].nbrs = -1;
				mx[cnt].nbrs = 0;
			}
			
			mx[temp-1].alloc(size);
			for (int i=0; i<size;i++) mx[temp-1].Push(elt[i]);
			mx[temp-1].nbrs = size;
			cnt = temp;
		}
		delete [] elt;
		elt = NULL;
		return mx;
	}
	
	long* id = new long[gObs];
	
	if (!ReadId(full_dbf_name, wxString(varname), id)) 
	{
		wxString msg = "Error: Variable name ";
		msg += wxString(varname);
		msg += " doesn't exist, \nsee the 1st line of your GWT file";
		wxMessageBox(msg);
		return NULL;
	}
	
	
	// Reading indirect Indices, the vae index is varname
	using namespace std;
	typedef map<long, long, less<long> > IDtoPkey;
	IDtoPkey theIDMap;
	
	for (int i=0;i<gObs; i++) 
	{
		theIDMap.insert(IDtoPkey::value_type((long) id[i], i));
	}
	
	GwtElement *mx	= new GwtElement [gObs];
	GwtNeighbor *elt = new GwtNeighbor [gObs];
	int			cnt=0;
	double	dist; 
	int			size;
	long		myID1 = -1, myID2 = -1;
	long		id1, id2, temp, myTemp; 
	
	IDtoPkey::iterator theIDIterator;
	
	for (cnt = 0;cnt < gObs;cnt++) 
	{
		mx[cnt].alloc(0);
		//		mx[cnt].nbrs = -1;
		mx[cnt].nbrs = 0;
	}
	
	
	// Read ist record
	// Map the ID into record_number
	ifl >> id1 >> id2 >> dist;
	myID1 = myID2 = -1;
	
	theIDIterator = theIDMap.find(id1);
	if(theIDIterator != theIDMap.end()) 
		myID1 = (*theIDIterator).second;
	theIDIterator = theIDMap.find(id2);
	if(theIDIterator != theIDMap.end()) 
		myID2 = (*theIDIterator).second;
	
	while (!ifl.eof()) 
	{
		temp = id1;
		myTemp = myID1;
		size = 0;
		while (temp==id1 && !ifl.eof()) 
		{
			elt[size].nbx = myID2;
			elt[size].weight = dist;
			
			ifl >> id1 >> id2 >> dist;
			size++;
			
			myID1 = myID2 = -1;
			theIDIterator = theIDMap.find(id1);
			if(theIDIterator != theIDMap.end()) 
				myID1 = (*theIDIterator).second;
			
			theIDIterator = theIDMap.find(id2);
			if(theIDIterator != theIDMap.end()) 
				myID2 = (*theIDIterator).second;
		}
		
		mx[myTemp].alloc(size);
		for (int i=0; i<size;i++) mx[myTemp].Push(elt[i]);
		mx[myTemp].nbrs = size;
	}
	
	delete [] elt;
	elt = NULL;
	
	delete [] id;
	id = NULL;
	theIDMap.erase(theIDMap.begin(), theIDMap.end());
	return mx;
	
}

