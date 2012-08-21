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

#ifndef __GEODA_CENTER_SHP_2_CNT_H__
#define __GEODA_CENTER_SHP_2_CNT_H__

#include <wx/filename.h>
#include "GalWeight.h"
#include <vector>

#define geoda_sqr(x) ( (x) * (x) )
void IndexSortD(const double * value, int* index, const int lower,
				const int upper);
void IndexSortD(const std::vector<double>& value, std::vector<int>& index,
				const int lower, const int upper);
bool IsPointShapeFile(const wxString& fname);
bool IsLineShapeFile(const wxString& fname);
bool IsPolygonShapeFile(const wxString& fname);
long GetShpFileSize(const wxString& fname);
GalElement* HOContiguity(const int p, long obs, GalElement *W, bool Lag);
GalElement* shp2gal(const wxString& fname, int criteria, bool save= true);
bool SaveGal(const GalElement *full, const wxString& ifname, 
			 const wxString& ofname, const wxString& vname,
			 const std::vector<wxInt64>& id_vec);

/**
 This is the core of the existing highlight/linking/brushing mechanism in
 GeoDa.  The bitmap bool array records the highlight/unhighlighted state
 for each underlying SHP file observation in the currently opened SHP file.
 Eventually this class will be replaced by the new HighlightState class.  Until
 everything is migrated over, the Selection class and HighlightState class
 will communicate with each to stay in sync.
 */
class Selection  
{
private:
	bool* bitmap;
    int* buffer;
	/** Size of the bitmap array.  Only changed in Init(). */
	int bmSize;
    int bufferSize;
	int saveBuffer;
public:
	bool* GetBitmap() { return bitmap; }
	int GetBitmapSize() { return bmSize; }
	int GetBufferSize() { return bufferSize; }
	/** deselect all elements */
	void clear();
	/** select single element in bitmap */
	void select(int elt);
	/** deselect a single element in bitmap */
    void deselect(int elt);
    Selection() : buffer(0), bitmap(0), bufferSize(0), bmSize(0) {}
	/**
	 Initialized the Selection class instance to a new size.  Existing
	 arrays will be deleted as needed and reallocated.
	 */
	bool Init(int sz)  
	{
		bmSize = sz;
		if (buffer) {
			delete [] buffer;
			buffer = 0;
		}
		if (bitmap) {
			delete [] bitmap;
			bitmap = 0;
		}
		buffer = new int[sz+2];
		bitmap = new bool[sz+2];
		if (!buffer || !bitmap) {
			bmSize= 0;
			return false;
		}
		bufferSize = 0;
		saveBuffer = 0;
		for (int cnt= 0; cnt < bmSize; ++cnt) {
			bitmap[cnt]    = false;
			buffer[cnt]    = 0;
		}
		return true;
	}

    virtual ~Selection() {
		if (buffer)	delete [] buffer; buffer = 0;
		if (bitmap) delete [] bitmap; buffer = 0;
	}
    bool Update();

    void Push( int elt ) {
        if (bufferSize < bmSize) 
			buffer[bufferSize++] = elt;
    }
    int Pop()  {
        if (bufferSize)
            return buffer [--bufferSize];
        return -1; // defined as GeoDaConst::EMPTY elsewhere.
    }
	void Reset(bool toEmpty=false);
	bool selected(int elt) const;
    void Invert() {
		for (int cnt=0; cnt < bmSize; ++cnt) bitmap[cnt] = !bitmap[cnt];
	}

};

/** Enum type for global variable ::gEvent which is declared OpenGeoDa.cpp and
 is used by Selection class and all classes using Selection to communicate
 what type of update is being done to the Selection bitmap array that
 records the current highlight state of all SHP observations. */
enum GeoDaEventType  {
    NO_EVENTS,
    NEW_SELECTION,
    ADD_SELECTION,
    DEL_SELECTION
};


#endif

