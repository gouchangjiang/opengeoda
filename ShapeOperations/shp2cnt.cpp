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
 Functions to create a contiguity file (.gal) from polygon coverage (shapefile).
 */
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "shp.h"
#include "shp2gwt.h"
#include "shp2cnt.h"

#include "../Thiessen/VorDataType.h"
#include <math.h>
#include <stdio.h>
#include "../GenUtils.h"
#include "../logger.h"
#include "ShapeFileHdr.h"
#include "ShapeFileTypes.h"

/*
 PartitionM
 */
class PartitionM  {
	private :
    double      step;
    int         elements, cells;
    int *       cell;
    int *       cellIndex;
    int *       lastIndex;
    RefPtr *    Refs;
	public :
    PartitionM(const int els, const int cls, const double range);
    virtual ~PartitionM();
	
    void include(const int incl);
    void remove(const int del);
    void initIx(const int incl, const double lwr, const double upr);
    int lowest (const int el) const  {  return cellIndex [ el ];  };
    int upmost(const int el) const  {  return lastIndex [ el ];  };
    int first(const int cl) const  {  return cell[ cl ];  };
    int tail(const int elt, const int cl) const  {
		return Refs[elt][cl-cellIndex[elt]].next;
    };
    int Sum() const;
};

/*
 PartitionM
 Constructor to initialize the partition
 */
PartitionM::PartitionM(const int els, const int cls, const double range) :
elements(els), cells(cls)  {
	cell= new int[ cells ];
	cellIndex= new int [ elements ];
	lastIndex= new int [ elements ];
	int cnt;
	for (cnt= 0; cnt < cells; ++cnt)
        cell [ cnt ] = GeoDaConst::EMPTY;
	Refs= new RefPtr [ elements ];
	for (cnt= 0; cnt < elements; ++cnt)
        Refs[cnt]= NULL;
	step= range / cells;
	return;
}

PartitionM::~PartitionM()  {
	if (cell)  {
		delete [] cell;
		cell= NULL;
	};
	if (cellIndex)  {
		delete [] cellIndex;
		cellIndex= NULL;
	};
	if (lastIndex)  {
		delete [] lastIndex;
		lastIndex= NULL;
	};
	if (Refs)  {
		for (int ref= 0; ref < elements; ++ref)
			if (Refs[ref]) delete [] Refs[ref];
		delete [] Refs;
		Refs= NULL;
	};
	cells= 0;
	elements= 0;
	return;
}

int PartitionM::Sum() const {
    int sum= 0;
    for (int cnt= 0; cnt < elements; ++cnt)
		sum += (lastIndex[cnt] - cellIndex[cnt] + 1);
    return sum;
};

void PartitionM::initIx(const int incl, const double lwr, const double upr)  {
	int lower= (int)floor(lwr/step), upper= (int)floor(upr/step);
	if (lwr < 0 || upper > cells || incl < 0 || incl >= elements)  
	{
		//     cout << "PartM: incl= " << incl << " l= " << lwr << "  " << upr; 
		exit(1);  
	};
	if (lower < 0) lower= 0;
    else if (lower >= cells) lower= cells-1;
	if (upper >= cells) upper= cells-1;
    else if (upper < 0) upper= 0;
	cellIndex [ incl ] = lower;
	lastIndex [ incl ] = upper;
	return;
}

/*
 PartitionM::include
 Include element incl with duration [lower, upper] into the partition.
 */
void PartitionM::include(const int incl)  {
	int cnt, lower= cellIndex [ incl ], upper= lastIndex [ incl ];
	RefPtr rptr= new RefStruct[ upper - lower + 1 ];
	Refs[incl]= rptr;
	for (cnt= upper-lower; cnt >= 0; --cnt)
		rptr[cnt]= Ref();
	for (cnt= lower; cnt <= upper; ++cnt)  {
		int old= cell [ cnt ];            // first element in the cell
		cell [ cnt ] = incl;              // new first element in the cell
		if (old != GeoDaConst::EMPTY)  {                  // the cell was not empty
			rptr[cnt-lower].next = old;            // OLD is the next element after incl in the list
			Refs[old][cnt-cellIndex[old]].prev= incl;    // incl is preceeding OLD in the list
		};
	};
	return;
};

/*
 PartitionM::remove
 Remove an element del from the partition.
 */
void PartitionM::remove(const int del)  {
	int   lower= cellIndex[ del ], upper= lastIndex [ del ], cnt;
	for (cnt= lower; cnt <= upper; ++cnt)  {
		RefStruct  cRef= Refs[del][cnt-lower];
		if (cRef.prev < 0)       // this is the first element in the list
			cell[ cnt ]= cRef.next;
		else
			Refs[cRef.prev][ cnt-cellIndex[cRef.prev] ].next= cRef.next;
		if (cRef.next != GeoDaConst::EMPTY)  // this is not the last element in the list
			Refs[cRef.next][cnt-cellIndex[cRef.next]].prev= cRef.prev;
	};
	delete [] Refs[del];
	Refs[del]= NULL;
	return;
}



// # of records in the shapefile == dimesion of the weights matrix
static long         gRecords= 0;
// locations of the polygon records in the shp file
static long *       gOffset= NULL;
// bounding boxes for each polygon
static Box *        gBox= NULL;
// bounding box for the entire map
static Box          gBigBox;
// partition constructed on lower(x) and upper(x) for each polygon
static BasePartition  gMinX, gMaxX;
// partition constructed on y for each polygon
static PartitionM * gY;

void ReadOffsets(const wxString& fname)  
{
	iShapeFile    shx(wxString(fname), "shx");
	char          hs[ 2*GeoDaConst::ShpHeaderSize ];
	shx.read((char *) &hs[0], 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hd(hs);
	long          offset, contents;
	gRecords= (hd.Length() - GeoDaConst::ShpHeaderSize) / 4;
	gOffset= new long [ gRecords ];
	
	for (long rec= 0; rec < gRecords; ++rec)  
	{
		offset= ReadBig(shx);
		contents= ReadBig(shx);
		offset *= 2;
		gOffset[rec]= offset;
	};
	return;
}

bool IsPointShapeFile(const wxString& fname)
{ // true: point
	iShapeFile   shp(fname, "shp");
	char         hs[ 2*GeoDaConst::ShpHeaderSize ];
	
	shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
	ShapeFileHdr       head(hs);
	
	shp.Recl(head.FileShape());
	return (head.FileShape() == ShapeFileTypes::SPOINT);
}

bool IsLineShapeFile(const wxString& fname)
{
	iShapeFile   shp(fname, "shp");
	char         hs[ 2*GeoDaConst::ShpHeaderSize ];
	
	shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
	ShapeFileHdr       head(hs);
	
	shp.Recl(head.FileShape());
	return (head.FileShape() == ShapeFileTypes::ARC);
}

bool IsPolygonShapeFile(const wxString& fname)
{
	iShapeFile   shp(fname, "shp");
	char         hs[ 2*GeoDaConst::ShpHeaderSize ];
	
	shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
	ShapeFileHdr       head(hs);
	
	shp.Recl(head.FileShape());
	return (head.FileShape() == ShapeFileTypes::POLYGON);
}

long GetShpFileSize(const wxString& fname)  
{
    iShapeFile    shx(fname, "shx");
    char          hs[ 2*GeoDaConst::ShpHeaderSize ];
    shx.read((char *) &hs[0], 2*GeoDaConst::ShpHeaderSize);
    ShapeFileHdr        hd(hs);
    long          n=0;
    n = (hd.Length() - GeoDaConst::ShpHeaderSize) / 4;
    return n;
}


bool ExistsShpShxDbf(const wxFileName& fname, bool* shp_found,
					 bool* shx_found, bool* dbf_found)
{
	wxFileName shp(fname);
	shp.SetExt("shp");
	wxFileName shx(fname);
	shx.SetExt("shx");
	wxFileName dbf(fname);
	dbf.SetExt("dbf");
	if (shp_found) *shp_found = shp.FileExists();
	if (shx_found) *shx_found = shx.FileExists();
	if (dbf_found) *dbf_found = dbf.FileExists();
	return shp.FileExists() && shx.FileExists() && dbf.FileExists();
}

void ReadBoxes(const wxString& fname)  
{
	iShapeFile   shp(fname, "shp");
	char          hs[ 2*GeoDaConst::ShpHeaderSize ];
	shp.read(hs, 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr hd(hs);
	if (ShapeFileTypes::ShapeType(hd.FileShape()) != ShapeFileTypes::POLYGON)  
	{
		//      cout << hs << endl;
		//	  cout << hd.FileShape() << endl;
		//      cout << " expecting POLYGON shape type " << endl;  
		exit(1);  
	};
	gBigBox= hd.BoundingBox();
	
	gBox= new Box [ gRecords ];
	for (long rec= 0; rec < gRecords; ++rec)  {
		shp.seekg(gOffset[rec]+12, ios::beg);
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
		gBox[rec] = Box(p1, p2);
#else
		shp >> gBox[rec];
#endif
		gBigBox += gBox[rec];                // make sure BigBox covers all boxes
	};
	return;
}


bool ReadId(const wxString& fname, const wxString& id, long* ids, long gObs)  
{
	if (ids == NULL || fname.empty() ||	id.empty())  
		return false;
	
	iDBF dbf(fname);
	if (!dbf.IsConnectedToFile())	{
		wxMessageBox("Error in opening the .dbf file!");
		return false;
	}
	
	wxString my_id = id;
	my_id.MakeUpper();
	
	const int column= dbf.FindField(my_id);
	if (column < 0) return false;
	
	for (int cnt= 0; cnt < gObs; ++cnt) {
		while (dbf.Pos() != column) dbf.Read();
		dbf.Read(ids[cnt]);
	}
	
	return true;
}

// tests if two boxes intersect each other
inline bool Intersect(const Box &a, const Box &b)  
{
    if (a._max().x < b._min().x) return false;      // a is left to b
    if (a._min().x > b._max().x) return false;      // a is right to b
    if (a._max().y < b._min().y) return false;      // a is down to b
    if (a._min().y > b._max().y) return false;      // a is up to b
    return true;
}

GalElement* MakeContiguity(const wxString& fname, const int crit)  
{
	int curr;
	iShapeFile    shp(fname, "shp");
	GalElement * gl= new GalElement [ gRecords ];
	
	if (!gl) return NULL;
	OgSet   Neighbors(gRecords), Related(gRecords);
	//  cout << "total steps= " << gMinX.Cells() << endl;
	for (int step= 0; step < gMinX.Cells(); ++step)  
	{
		// include all elements from xmin[step]
		for (curr= gMinX.first(step); curr != GeoDaConst::EMPTY; curr= gMinX.tail(curr))
			gY->include(curr);
		
		// test each element in xmax[step]
		for (curr= gMaxX.first(step); curr != GeoDaConst::EMPTY; curr= gMaxX.tail(curr))  {
			PolygonPartition       testPoly;
			shp.seekg(gOffset[curr]+12, ios::beg);
			testPoly.ReadShape(shp);               // load in the memory
			testPoly.MakePartition();
			
			// form a list of neighbors
			for (int cell= gY->lowest(curr); cell <= gY->upmost(curr); ++cell)  {
				int potential= gY->first( cell );
				while (potential != GeoDaConst::EMPTY)  {
					if (potential != curr)  Neighbors.Push( potential );
					potential= gY->tail(potential, cell);
				};
			};
			
			// test each potential neighbor
			for (int nbr= Neighbors.Pop(); nbr != GeoDaConst::EMPTY; nbr= Neighbors.Pop()) {
				if (Intersect(gBox[curr], gBox[nbr])) {
					PolygonPartition   nbrPoly;
					shp.seekg(gOffset[nbr]+12, ios::beg);
					nbrPoly.ReadShape(shp);
					
					// run sweep with testPoly as a host and nbrPoly as a guest
					int related = testPoly.sweep(nbrPoly, crit);
					if (related)  Related.Push( nbr );
				}
			}
			
			if (Related.Size() && gl[curr].alloc(Related.Size()))  {
				while ( Related.Size() )
					gl[curr].Push( Related.Pop() );
			};
			
			gY->remove(curr);       // remove from the partition
		};
	};
	
	return gl;
}

void ValueSort(const long * value, const int lower, const int upper)  
{
	int   i= lower, j= upper, t = 0;
	long    pivot= value[(lower+upper) >> 1];
	do  {
		while (value[i] < pivot) ++i;
		while (value[j] > pivot) --j;
		if (i < j)  SWAP(i, j, t);
		if (i <= j)  { ++i;  --j;  };
	}
	while (j >= i);
	if (j > lower) ValueSort(value, lower, j);
	if (i < upper) ValueSort(value, i, upper);
	return;
}


inline void Write(ofstream &out, const double val)  
{
    out.write((char *) &val, sizeof(double));
    return;
}

inline void Write(ofstream &out, const long val)  
{
	out.write((char *) &val, sizeof(long));
	return;
}


inline long Read(ifstream &in)  
{
	long   val;
	in.read( (char *) &val, sizeof(long) );
	return val;
}


inline double ReadD(ifstream &in)  
{
	double   val;
	in.read( (char *) &val, sizeof(double) );
	return val;
}



GalElement * MakeFull(GalElement *half)  
{
    long * Count= new long [ gRecords ], nbr, cnt;
    for (cnt= 0; cnt < gRecords; ++cnt)
		Count[ cnt ]= half[cnt].Size();
    for (cnt= 0; cnt < gRecords; ++cnt)
        for (nbr= half[cnt].Size()-1; nbr >= 0; --nbr)
			++Count [ half[cnt].elt(nbr) ];
    GalElement * full= new GalElement [ gRecords ];
	
    for (cnt= 0; cnt < gRecords; ++cnt)
		full[cnt].alloc( Count[cnt] );
    for (cnt= 0; cnt < gRecords; ++cnt)
		for (nbr= half[cnt].Size()-1; nbr >= 0; --nbr)  {
			long val= half[cnt].elt(nbr);
			full[cnt].Push(val);
			full[val].Push(cnt);
		};
    for (cnt= 0; cnt < gRecords; ++cnt)  {
		if (full[cnt].Size() > 1)
			ValueSort(full[cnt].dt(), 0, full[cnt].Size()-1);
    };
	delete [] Count;
	Count = NULL;
    return full;
};



void WriteGal(const GalElement *full, const char *fname, const int criteria)  
{
    char    fn[256];
	const long c = criteria;
	const long r = gRecords;
	
    strcpy(fn, fname);
    strcat(fn, ".wts");
    ofstream    bin(fn, ios::binary);
    Write(bin, (long) 0);
    Write(bin, c);
    Write(bin, r);
    for (long cnt = 0; cnt < gRecords; ++cnt)
		full[cnt].Write(bin);
    return;
}

bool SaveGal(const GalElement *full, 
			 const wxString& ifname, 
			 const wxString& ofname, 
			 const wxString& vname, 
			 const long Obs)
{
	LOG_MSG("Entering SaveGal, (5 args)");
	if (full == NULL  || 
		ifname.empty() ||
		ofname.empty() ) 
		return false;
	
	wxString fn = ifname;
	wxString fngal = ofname;
	
	// cut out the extension. ".shp"
	fn = fn.substr(0,fn.length()-4);
	fngal = fngal.substr(0,fngal.length()-4);
	
	long *ids = NULL;
	
	if (!vname.empty()) {
		ids = new long [Obs];
		if (!ReadId(fn, vname, ids, Obs)) {
			if (ids) delete [] ids;
			ids = NULL;
		}
	}
	
	fngal += ".gal";
	ofstream    out(fngal.wx_str());
	
	wxString local = GenUtils::GetTheFileTitle(fn);
	
	LOG_MSG(local);
	if (ids != NULL) {
		out << "0 " << Obs << " " << local.c_str();
		out << " " << vname.c_str() << endl;
	}
	else 
		out << Obs << endl;
	
	for (int cnt= 0; cnt < Obs; ++cnt) {
		if (ids != NULL) 
			out << ids[cnt];
		else 
			out << cnt+1;
		
		out << " " << full[cnt].Size() << endl;
		for (int cp= full[cnt].Size(); --cp >= 0;) {
			if (ids) 
				out << ids[full[cnt].elt(cp)];
			else 
				out << full[cnt].elt(cp)+1;
			
			if (cp > 0) out << " ";
		}
		out << endl;
	}
	
	if (ids != NULL) 
		delete [] ids;
	ids = NULL;
	
	LOG_MSG("Exiting SaveGal, (5 args)");
	return true;
}



GalElement * ReadGal(const char *fname, const int criteria)  
{
	char  fn[256];
	strcpy(fn, fname);
	strcat(fn, ".wts");
	ifstream  bin(fn, ios::binary);
	if (bin.fail())  return NULL;             // weights file does not exist
	if (Read(bin) != 0) return NULL;          // it is not a contiguity file
	if (Read(bin) != criteria) return NULL;   // different crierion of contiguity
	long dim= Read(bin);
	GalElement * mx= new GalElement [ dim ];
	for (long cnt = 0; cnt < dim; ++cnt)
		mx[cnt].Read(bin);
	return mx;
}


GalElement* shp2gal(const wxString& fname, int criteria, bool save)  
{
	GalElement * full;
	ReadOffsets(fname);
	ReadBoxes(fname);
	
	long gx, gy, cnt, total=0;
	gx= gRecords / 8 + 2;
	
	gMinX.alloc(gRecords, gx, gBigBox._max().x - gBigBox._min().x );
	gMaxX.alloc(gRecords, gx, gBigBox._max().x - gBigBox._min().x );
	for (cnt= 0; cnt < gRecords; ++cnt) {
		gMinX.include( cnt, gBox[cnt]._min().x - gBigBox._min().x );
		gMaxX.include( cnt, gBox[cnt]._max().x - gBigBox._min().x );
	}
	
	gy= (int)(sqrt((long double)gRecords) + 2);
	do {
		gY= new PartitionM(gRecords, gy, gBigBox._max().y - gBigBox._min().y );
		for (cnt= 0; cnt < gRecords; ++cnt)
			gY->initIx( cnt, gBox[cnt]._min().y - gBigBox._min().y, 
					   gBox[cnt]._max().y - gBigBox._min().y );
		total= gY->Sum();
		if (total > gRecords * 8) {
			delete gY;
			gy = gy/2 + 1;
			total= 0;
		}
	} while ( total == 0);
	
	GalElement * gl= MakeContiguity(fname, criteria);
	
	if (gY) delete gY; gY = 0;
	if (gOffset) delete [] gOffset; gOffset = 0;
	if (gBox) delete [] gBox; gBox = 0;
	
	full = MakeFull(gl);
	if (gl) delete [] gl; gl = 0;
	return full;
}

/** IndexSort(): Sort array value. Algorithm: QuickSort */
void IndexSortD(const double * value, int * index,
				const int lower, const int upper)  
{
	int   i= lower, j= upper, t = 0;
	double    pivot= value[index[(lower+upper) >> 1]];
	do  {
		while (value[index[i]] < pivot) ++i;
		while (value[index[j]] > pivot) --j;
		if (i < j)  SWAP(index[i], index[j], t);
		if (i <= j)  { ++i;  --j;  };
	}
	while (j >= i);
	if (j > lower) IndexSortD(value, index, lower, j);
	if (i < upper) IndexSortD(value, index, i, upper);
	return;
}

void IndexSortD(const std::vector<double>& value, std::vector<int>& index,
				const int lower, const int upper)  
{
	int   i= lower, j= upper, t = 0;
	double    pivot= value[index[(lower+upper) >> 1]];
	do  {
		while (value[index[i]] < pivot) ++i;
		while (value[index[j]] > pivot) --j;
		if (i < j)  SWAP(index[i], index[j], t);
		if (i <= j)  { ++i;  --j;  };
	}
	while (j >= i);
	if (j > lower) IndexSortD(value, index, lower, j);
	if (i < upper) IndexSortD(value, index, i, upper);
}


// Lag: True; otherwise Cummulative
GalElement *HOContiguity(const int p, long obs, GalElement *W, bool Lag)
{
	
	if (obs		< 1 ||
		p			<= 1 ||
		p			> obs-1)
		return NULL;
	
	int j, QueueCnt, c, irow, CurrIx, LastIx;
	int* nLag				= new int [p];
	GalElement *HO	= new GalElement[obs];
	long *Queue			= new long [obs];
	long *dt;
	int *OC					= new int [obs];
	
	if (W			== NULL ||
		nLag	== NULL ||
		HO		== NULL ||
		Queue == NULL ||
		OC		== NULL)
		return NULL;
	
	for (j=0; j < obs; j++) OC[j] = 0;
	for (irow = 0; irow < obs; irow++) 
	{
		OC[irow] = -1;
		dt = W[irow].dt(); // neighbors of irow
		QueueCnt	= W[irow].Size();
		for (j=0; j<QueueCnt; j++) 
		{
			Queue[j] = dt[j];
			OC[Queue[j]] = 1;
		}
		CurrIx = 0; int k;
		k= LastIx = QueueCnt;
		for (c=2;c<=p; c++) 
		{
			nLag[c-2] = 0;
			LastIx = k;
			for (;CurrIx <LastIx; CurrIx++) 
			{
				dt = W[Queue[CurrIx]].dt();
				int Nbrs = W[Queue[CurrIx]].Size();
				for ( j=0; j< Nbrs; j++) 
				{
					if (OC[dt[j]] == 0) 
					{
						OC[dt[j]] = c;
						Queue[k] = dt[j];
						k++;
						nLag[c-2]++;
					}
				}
			}
		}
		
		LastIx = k;
		if (!Lag) 
		{
			HO[irow].alloc(nLag[p-2]);
			k = nLag[p-2];
		}
		else HO[irow].alloc(LastIx);
		
		for (j=0;j<LastIx;j++) 
		{
			if (Lag) 
			{
				HO[irow].Push(Queue[j]);
			}
			else 
			{
				
				if (OC[Queue[j]] == p) 
				{
					HO[irow].Push(Queue[j]);
				}
				
			}
			OC[Queue[j]] = 0;
		}
		OC[irow] = 0;
	}
	return HO;
}

#include <map>

GalElement* ReadTxtGal(const char *fname, long gObs,
					   const wxString& full_dbf_name)  
{
	ifstream    ifl(fname, ios::in);
	if (ifl.fail()) {
		wxMessageBox("Error: Could not open file");
		return NULL;
	}
	
	wxString unique_id_msg;
	unique_id_msg = "Warning, your weights file relies on record order ";
	unique_id_msg += "rather than an ID Variable.  Please consider ";
	unique_id_msg += "generating a new weights file that uses an ID ";
	unique_id_msg += "Variable.";
	long type; long obs=0;
	ifl >> type;
	
	// Reading direct Indices
	if (type > 1) {  // type is # of observation
		wxMessageBox(unique_id_msg);
		if (type == gObs) 
		{
			
			GalElement *mx = new GalElement [ gObs ];
			
			obs = type;
			//==
			long id, dim, mydt;
			
			for (long cnt = 0; cnt < obs; cnt++) 
			{
				
				ifl >> id >> dim;
				long * dt = new long[dim];
				
				if (dim < 0 || dim > obs-1)
				{
					wxMessageBox("Error: Invalid # of neighbors");
					ifl.close();
					delete [] mx;
					mx = NULL;
					return NULL;
				}
				
				for (int i=0;i< dim;i++) 
				{
					
					ifl >> mydt;
					dt[i] = mydt-1;
				}
				
				if (mx[cnt].ReadTxt(dim, dt, obs) == -1)
				{
					delete [] dt;
					dt = NULL;
					wxMessageBox("Error: The neighbor index is out of bound");
					return NULL;
				}
				delete [] dt;
				dt = NULL;
			}
			return mx;
		}
		else
		{
			wxMessageBox("Error: Could not open file.");
			ifl.close();
		}
		return NULL;
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
		GalElement *mx = new GalElement [ gObs ];
		obs = type;
		for (long cnt = 0; cnt < obs; cnt++) 
		{
			if (mx[cnt].ReadTxt(ifl, obs) == -1)
			{
				wxMessageBox("1615:The Neighbor index is out of bound!");
				return NULL;
			}
		}
		return mx;
	}
	
	long* id = new long[gObs];
	
	if (!ReadId(full_dbf_name, varname, id, gObs)) 
	{
		wxString msg = "Variable name ";
		msg = msg + wxString::Format("%s",varname) + ":";
		msg = msg + " is unknown, \nsee the 1st line of your gal file";
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
	
	GalElement *mx = new GalElement [ gObs ];
	long *dt = new long[gObs];
	int dim=0;
	
	IDtoPkey::iterator theIDIterator;
	for (long cnt = 0; cnt < obs; cnt++) 
	{
		long id; int dim; 
		long mydt, myID = -1;
		ifl >> id >> dim;
		
		if (dim < 0 || dim > obs-1)
		{
			wxMessageBox("Error: Number of neighbors is too big");
			return NULL;
		}
		
		for (int i=0;i< dim;i++) 
		{
			ifl >> mydt;
			theIDIterator = theIDMap.find(mydt);
			if(theIDIterator != theIDMap.end()) 
				myID = (*theIDIterator).second;
			dt[i] = myID;
		}
		if (mx[cnt].ReadTxt(dim, dt, obs) == -1)
		{
			wxMessageBox("Error: The neighbor index is out of bounds");
			return NULL;
		}
		
	}
	
	//ifl.clrlock();
	ifl.close();
	
	delete [] id;
	id = NULL;
	theIDMap.erase(theIDMap.begin(), theIDMap.end());
	return mx;
}

void DevFromMean(int nObs, DataPoint* RawData)
{
	double sum = 0;
	int cnt = 0 ;
	
	double sumY = 0.0, sumX = 0.0;
	for (cnt= 0; cnt < nObs; ++cnt) 
	{
		sumY += RawData[cnt].vertical;
		sumX += RawData[cnt].horizontal;
	}
	const double meanY = sumY / nObs;
	const double meanX = sumX / nObs;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		RawData[cnt].vertical   -= meanY;
		RawData[cnt].horizontal -= meanX;
	}
}

double* DevFromMeanR(int nObs, double* RawData)
{
	double        sum= 0;
	double *d = new double[nObs];
	double sumX = 0.0;
	int cnt = 0;
	for (cnt= 0; cnt < nObs; ++cnt) 
	{
		sumX += RawData[cnt];
	}
	
	const double  meanX = sumX / nObs;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		d[cnt] = RawData[cnt] - meanX;
	}
	
	return d;
}

void DevFromMean(int nObs, double* RawData)
{
	double sumX = 0.0;
	int cnt = 0;
	for (cnt= 0; cnt < nObs; ++cnt) 
	{
		sumX += RawData[cnt];
	}
	const double  meanX = sumX / nObs;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		RawData[cnt] -= meanX;
	}
}

void DevFromMean(int nObs, double** RawData,
				 int deps, int startfrom)
{
	int i = 0, cnt = 0;
	for (i=startfrom;i<deps;i++)
	{
		double        sum= 0;
		double sumX = 0.0;
		for (cnt= 0; cnt < nObs; ++cnt) 
		{
			sumX += RawData[i][cnt];
		}
		
		const double  meanX = sumX / nObs;
		for (cnt= 0; cnt < nObs; ++cnt)
		{
			RawData[i][cnt] -= meanX;
		}
	}
}

bool StandardizeData(int nObs, double* RawData)
{
	DevFromMean(nObs, RawData);
	double sumX= 0;
	int cnt = 0;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		sumX += RawData[cnt] * RawData[cnt];
	}
	const double sdevX = sqrt(sumX / (nObs-1));
	if (sdevX == 0.0) 
		return false;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		RawData[cnt] /= sdevX;
	}
	return true;
}

bool CheckDataValidity(int obs, double* xx)  
{
	int cnt;
	double        sum= 0;
	
	double *x = new double[obs+1];
	// Standardized X
	for (cnt= 0; cnt < obs; ++cnt) {
		x[cnt] = xx[cnt];
		sum += x[cnt];
	}
	
	const double meanX = sum / obs;
	
	if (meanX != 0) {
		// remove the mean
		for (cnt= 0; cnt < obs; ++cnt)
			x[cnt] -= meanX;
	}
	
	sum= 0;                   // accumulate the sum of squares
	
	for (cnt= 0; cnt < obs; ++cnt)
		sum += x[cnt] * x[cnt];
	
	const double sdevX= sqrt(sum / (obs - 1));
	
	bool valid = (sdevX > 0.0);
	
	if (x) delete [] x; x=0;
	
	return valid;
}

/** The globally defined instance of GeoDaEventType, gEvent. */
GeoDaEventType	gEvent = NO_EVENTS;

/** Update the bitmap boolean selection array according to the global
 selection-type enum  instance varaiable gEvent
 
 Update boolean array #bitmap according to gEvent, which is defined as a global
 variable in shp2cnt.cpp.  gEvent is an instance of enum #GeoDaEventType which
 is defined in shp2cnt.h
 */
bool Selection::Update()
{
	LOG_MSG("Entering Selection::Update");
	int cpt;
	saveBuffer= bufferSize; // save the bufferSize value before stack Pop ops
	switch (gEvent)  {
		case NEW_SELECTION :
		{
			clear();  // set all entries in bitmap to false
		}
		// Note: NEW_SELECTION is always followed by ADD_SELECTION
		//  since there is no break statement.  This is intentional.
		case ADD_SELECTION :
		{
			// select all entries on buffer stack
			while( (cpt=Pop()) != GeoDaConst::EMPTY) select( cpt );
		}
			break;
		case DEL_SELECTION :
		{
			// deselect all entries on buffer stack
			while( (cpt=Pop()) != GeoDaConst::EMPTY) deselect( cpt );
		}
			break;
		default :
		{
			return false;
		}
	};
	bufferSize= saveBuffer;  // reset bufferSize to original value before Update
	LOG_MSG("Exiting Selection::Update");
	return true;
}

void Selection::Reset(bool toEmpty)
{
	bufferSize = toEmpty ? 0 : saveBuffer;
}

void Selection::select(int elt)
{
	//LOG_MSG(wxString::Format("Selection::select(%d)",elt));
	if (elt>=0 && elt <= bmSize) bitmap [ elt] = true;
}

void Selection::deselect(int elt)
{
	//LOG_MSG(wxString::Format("Selection::deselect(%d)",elt));
	if (elt>=0 && elt <= bmSize) bitmap [ elt] = false;
}
	
bool Selection::selected(int elt) const
{
	return bitmap[elt];
}

void Selection::clear()
{
	//LOG_MSG("In Selection::clear()");
	for (int cnt=0; cnt<bmSize; cnt++) {
		bitmap[cnt] = false;
	}
}
