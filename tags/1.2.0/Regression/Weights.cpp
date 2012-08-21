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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "mix.h"
#include "Lite2.h"
#include "Weights.h"

/*
 Weights - constructor
 Creates instance of class Weights in the memory by reading
 specified file. Type of the file is determined by its extension.
 */
Weights::Weights(const char * fname)  
{
	char ext[4];
	// get extension
	const char* ch_p= fname;
	while (*ch_p && *ch_p != '.') ++ch_p;
	++ch_p;
	strncpy(ext, ch_p, 4);
	for (int cp= strlen(ext)-1; cp >= 0; --cp)
		ext[cp] = toupper(ext[cp]);
	// extension should be in ext array.
	
  if (strcmp(ext, "MAT") == 0)  {  WeightsMat(fname);  format= W_MAT;  }

    else if (strcmp(ext, "gwt") == 0)  {  WeightsGwt(fname);  format= W_GWT;  }
      else if (strcmp(ext, "gal") == 0)  {  WeightsGal(fname);  format= W_GAL;  }
        else  {  format= W_UNDEF;
//                     INIT_MSG
//                     cout << "unable to identify weights matrix: " << fname << ends;
//                 PASS_MSG(WRN_LAST);
                           };
  strncpy(name, fname, NAMELEN);
  char * ch= name;
  name[NAMELEN+1]= '\x0';
  while (*ch && *ch != '.') ++ch;
  *ch= '\x0';
  return;
}

Weights::Weights(const GalElement* my_gal, int num_obs)  
{
	WeightsGal(my_gal, num_obs);
	format= W_GAL;
	sprintf(name,"myGAL.txt");
}

/*
 */
void SL_Transform(WVector &to, WMap::input_iterator from, const size_t dim)  {
    to.alloc(dim, 0);         // create zero array of size dim
    for ( ; from; ++from)
        to[(*from).first]= (*from).second;
    return;
}


/*
 */
void SL_Transform(WVector &to, Iterator<INDEX> from, const size_t dim)  {
    to.alloc(dim, 0);              // create zero array of size dim
    VALUE         setvalue(1);
    for ( ; from; ++from)

        to[*from]= setvalue;
    return;
}

/*
 */
void SL_Transform(WMap &to, WIterator from, size_t Cnt)  {
    Cnt= Values(from, 0.0);           // # of non-zeros in the array
    to.alloc(Cnt);
    INDEX    key(0);
    while (Cnt--)  {
        while (!(*from))  {                   // skip zeros
            ++key;  ++from;  };
        to << WPair(key, *from);       // copy nonzero
        ++key;  ++from;
    };
    return;
}


/*
 */
void SL_Transform(WMap &to, Iterator<INDEX> from, size_t cnt)  {
    VALUE  setvalue(1);
    to.alloc(cnt= from.count());   // create map of the same size as set
    for ( ; from; ++from)
        to << WPair(*from, setvalue);
    return;
}

/*
 */
void SL_Transform(Vector<INDEX> &to, WIterator from, size_t cnt)  {
    cnt= Values(from, 0.0);           // # of non-zero elements
    to.alloc(cnt);       // create object of proper size
    INDEX     key(0);
    while (cnt--)  {
        while (!(*from))  {
            ++key;  ++from;  };
        to << key;
        ++key;  ++from;
    };
    return;
}

/*
 */
void SL_Transform(Vector<INDEX> &to, WMap::input_iterator from, size_t cnt)  {
    cnt= from.count();              // # of elements in the map
    to.alloc(cnt);                // create object of proper size
    for ( ; from; ++from)
        to <<  (*from).first;
    return;
}


/*
 */
template <class T, class F>
void SL_WeightsTransform(T & t, F it)  {
    size_t dim(it.count());
    t.alloc(dim);

    while (it)  {
        SL_Transform(*t, (*it)(), dim);
        ++it;  ++t;
    };
}

/*
 */
void Weights::Transform(const WeightsType otype)  {
  if (format == W_UNDEF) return;
  
  if (format != otype)  {
  
    switch (format)  {
    
      case W_MAT : if (otype == W_GWT) SL_WeightsTransform(gt, mt());
                      else SL_WeightsTransform(gl, mt());
                    Destroy(mt());    break;
                    
      case W_GWT : if (otype == W_MAT) SL_WeightsTransform(mt, gt());
                      else SL_WeightsTransform(gl, gt());
                    Destroy(gt());    break;
                    
      case W_GAL : if (otype == W_MAT) SL_WeightsTransform(mt, gl());
                      else SL_WeightsTransform(gt, gl());
                    Destroy(gl());    break;
                    
      case W_UNDEF : 	/*  data should have format other than UNDEF */
                        break;
                        
    };
    
    format= otype;
    
  };
}

/*
 */
void Weights::Put(const char * fname, const WeightsType otype)  
{
  if (otype != W_UNDEF) Transform(otype);
  switch(format)  {
    case W_MAT : PutMat(fname);  break;
    case W_GWT : PutGwt(fname);  break;
    case W_GAL : PutGal(fname);  break;
    case W_UNDEF: /*  nothing to output */
        break;
  };
  return;
}

/*
        Weights::names
   Creates a vector of CNT. Uses data member keys to find the name of each row/column
   of the weights matrix with given offset. Assumes that the elements in keys are sorted
   in ascending order of first component (to simplify search of offset for given name). This
   vector performs ordering of names in the order of their occurence in the matrix.
   first - is name of a row, second is its offset in the matrix.
 */
void Weights::names(Vector<CNT> &table)  {
  table.alloc(key.count());
  keymap::input_iterator itr= key();
  while (itr)  {
    table[(*itr).second] = (*itr).first;
    ++itr;
    ++table;         // advance tmp pointer
  };
  return;
}

/*
 */
void Weights::PutMat(const char * fname)  {
/*
  OUTPUTstream          out(fname, W_extension[W_MAT]);
  mat::input_iterator   row= mt();
  Vector<CNT>             real_names;
  names(real_names);
  out << mt.count() << mt.count() << endl;      // # of rows & columns in the matrix
  while (row)  {
//    PrintContainer(out, (*row)());
  };  
*/
  return;
}

/*
 */
void Weights::PutGwt(const char * fname)  {
/*
  OUTPUTstream          out(fname, W_extension[W_GWT]);
  gwt::input_iterator   row= gt();
  Vector<CNT>           real_names;
  names(real_names);
  out << gt.count() << endl;            // # of rows in the matrix
  int rowCnt= 1;
  while (row)  {
        map::input_iterator  it= (*row)();
        while (it)  {
          out << rowCnt << (*it).first+1 << (*it).second << endl;
          ++it;
        };
        ++row;
        ++rowCnt;
  };
*/
  return;

}

/*
 */
void Weights::PutGal(const char * fname)  
{
/*
  OUTPUTstream          out(fname, W_extension[W_GAL]);
  gal::input_iterator   row= gl();
  Vector<CNT>           real_names;
  names(real_names);
  Iterator<CNT>         row_name= real_names();
  out << gl.count()  << endl;           // # of rows in the matrix
  while (row)  {
        out << *row_name << (*row).count() << endl;
        set::input_iterator     it=(*row)();
        while (it)  {
          out << ' ' << real_names[*it];
          ++it;
        };
        out << endl;
        ++row_name;  ++row;
  };
*/
  return;

}

/*
 */
Weights::~Weights()  
{
  Destroy(mt());
  Destroy(gt());
  Destroy(gl());
  format= W_UNDEF;
  wdim= 0;
  return;
}



/*
 */
void Weights::WeightsMat(const char * fname)
{
	return;
}

void Weights::WeightsGwt(const char * fname)
{
	return;
}

void Weights::WeightsGal(const char * fname)  
{
/*
  typedef pairstruct<CNT, INDEX> local_pair;
  INPUTstream   inp(fname, NULL);
  INDEX         Rows, disp(0), sz;
  CNT           row;
  inp >> Rows;
  gal           tgt(Rows);
  key.alloc(Rows);
  while(inp >> row && disp < Rows)  
	{
    key << local_pair(row, disp);
    inp >> sz;
    (*tgt).alloc(sz);
    if (sz)  {
      CopyInput(*tgt, inp, sz);
      HeapSort((*tgt)());
    };
//    PrintContainer(cout, (*tgt)());
    ++disp;  ++tgt;
  };
  HeapSort(key());
  gl.alloc(Rows);
  gal::input_iterator itemp= tgt();
  while (itemp)  {
    (*gl).alloc((*itemp).count());
    for (set::input_iterator iset= (*itemp)(); iset; ++iset)  
		{
      local_pair * t= Find(key(), *iset);
      if (t) *gl << t->second;
//        else  Messenger(ERR_LAST, " GAL reference outside the matrix");
    };
    ++itemp;  ++gl;
  };
  wdim= Rows;
*/
  return;
}


/*
 */
void Weights::WeightsGal(const GalElement *my_gal, int num_obs)  
{
  typedef pairstruct<CNT, INDEX> local_pair;
  INDEX         Rows, disp, sz;
  CNT           row;


	disp = 0;
	Rows = num_obs;
  gal           tgt(Rows);
  key.alloc(Rows);

  for (INDEX k=0;k<Rows;k++)
	{
		row = k;
    key << local_pair(row, disp);
		sz = my_gal[k].Size();
    (*tgt).alloc(sz);
    if (sz)  
		{
			const long *nblist = my_gal[k].dt(); 
      CopyInput(*tgt, nblist , sz);
      HeapSort((*tgt)());
    };
    ++disp;  ++tgt;

  }

  HeapSort(key());
  gl.alloc(Rows);
  gal::input_iterator itemp= tgt();
  while (itemp)  {
    (*gl).alloc((*itemp).count());
    for (set::input_iterator iset= (*itemp)(); iset; ++iset)  
		{
      local_pair * t= Find(key(), *iset);
      if (t) *gl << t->second;
//        else  Messenger(ERR_LAST, " GAL reference outside the matrix");
    };
    ++itemp;  ++gl;
  };
  wdim= Rows;
  return;
}

void Weights::WeightsGwt(const GwtElement *my_gwt, int num_obs)  
{
  typedef pairstruct<CNT, INDEX> local_pair;
  INDEX         Rows, disp, sz;
  CNT           row;

	int j = 0;
	INDEX k = 0;
	disp = 0;
	Rows = num_obs;
  gwt           tgt(Rows);
  key.alloc(Rows);

  for (k=0;k<Rows;k++)
	{
		row = k;
    key << local_pair(row, disp);
		sz = my_gwt[k].Size();
    (*tgt).alloc(sz);
    if (sz)  
		{
			const long *nblist = my_gwt[k].GetData();
			double *weightlist = new double [my_gwt[k].Size()];
			for (j = 0; j < my_gwt[k].Size(); j++) weightlist[j] = my_gwt[k].elt(j).weight;
      CopyInput(*tgt, nblist, weightlist, sz);
      HeapSort((*tgt)());
    };
    ++disp;  ++tgt;

  }

  HeapSort(key());
  gt.alloc(Rows);
  gwt::input_iterator itemp= tgt();
  while (itemp)  {
    (*gt).alloc((*itemp).count());
    for (WMap::input_iterator iset= (*itemp)(); iset; ++iset)  
		{
      local_pair * t= FindW(key(), *iset);
      if (t) *gt << pairstruct<INDEX, VALUE>(t->second, (*iset).second);
//        else  Messenger(ERR_LAST, " GAL reference outside the matrix");
    };
    ++itemp;  ++gt;
  };
  wdim= Rows;
  return;
}

/*
 */
void Weights::Export(gwt &a)  {
  Transform(W_GWT);
  a.alloc(gt.count());
  gwt::input_iterator it= gt();
  while (it)  {
     (*a).copy((*it)());
     ++a;  ++it;
  };
  return;
}

/*
 */
void Weights::Export(mat &a)  {
  Transform(W_MAT);
  a.alloc(mt.count());
  mat::input_iterator it= mt();
  while (it)  {
     (*a).copy((*it)());
     ++a;  ++it;
  };
  return;
}


/*
 */
void Weights::Export(gal &a)  {
  Transform(W_GAL);
  a.alloc(gl.count());
  gal::input_iterator it= gl();
  while (it)  {
     (*a).copy((*it)());
     ++a;  ++it;
  };
  return;
}

/*
 */
void Weights::MakeSymmetricStructure()  {
    const int		dim= gt.count();
    typedef Iterator<WPair>	RowIterator;
    Vector<RowIterator>		state;
    RowIterator	row;
    Iterator<WMap>	it;
    int		cnt, rowId;
    Vector<int>		counts(dim, 0);
    state.alloc( dim );				// allocate vector state
    for (cnt= 0; cnt < dim; ++cnt)
        state << gt[cnt]();					// initialize state

    for (rowId= 0; rowId < dim; ++rowId)   {
        for (row= state[rowId]; row; ++row)  {
            int nextId = (*row).first;
            if (nextId == rowId)  {			// this is a diagonal element
                ++counts[rowId];
            }  else   if (nextId < rowId)  {
                ++counts[rowId];		// real existing pair;
                ++counts[nextId];  		// pair (nextId, rowId) = 0
            }  else  {		// begin partial processing of row nextId
                int	nextOtherId = NeighborId(state[nextId]);
                while (nextOtherId < rowId)  {	// row nextId has neighbors with id less than rowId

                    ++counts[nextId];				// to accomodate real pair(nextId, nextOtherId)
                    ++counts[nextOtherId];		// to accomodate pair(nextOtherId, nextId) = 0
                    ++state[nextId];				// advance state to next weight
                    nextOtherId= NeighborId(state[nextId]);
                }; 
                if (nextOtherId == rowId)  {  // symmetric entry
                    ++counts[nextId];				// to accomodate real pair(nextId, nextOtherId)
                    ++counts[rowId];		// to accomodate real pair(nextOtherId, nextId)
                    ++state[nextId];				// advance state to next weight
                }  else  {  
                    ++counts[rowId];
                    ++counts[nextId];
                };
            };	// end else   

        };

    }; 
    return;
}

void Weights::setWeights(const int row, const int loc[], const int sz)  {
    if (row >= dim()) return;
    if (format == W_GWT)  {
        gt[row].alloc(sz);
        for (int cnt = 0; cnt < sz; ++cnt)  
            gt[row] << WPair( loc[cnt], 1.0 );
    }  else  if (format == W_GAL)  {
        gl[row].alloc(sz);
        for (int cnt = 0; cnt < sz; ++cnt)
            gl[cnt] << loc[cnt];
    }  else if (format == W_MAT)  {
        mt[ row ].alloc( (INDEX) dim(), 0.0 );
        for (int cnt = 0; cnt < sz; ++cnt)
            mt[row][loc[cnt]] = 1.0;
    };
}
            
void Weights::setWeights(const int row, const int loc[], const double weight[], const int sz)  {
    if (row >= dim()) return;
    if (format == W_GWT)  {
        gt[row].alloc(sz);
        for (int cnt = 0; cnt < sz; ++cnt)  
            gt[row] << WPair( loc[cnt], weight[cnt] );
    }  else  if (format == W_GAL)  {
        return;
    }  else if (format == W_MAT)  {
        mt[ row ].alloc( (INDEX) dim(), 0.0 );
        for (int cnt = 0; cnt < sz; ++cnt)
            mt[row][loc[cnt]] = weight[cnt];
    };
}
            
void Weights::ComputeTranspose(GWT & transpose)  {
    const int		dim= gt.count();
    int		cnt;
    Vector<int>	count(dim, 0);		// will contain number of non-zero elements in the rows of transposed mx
    Iterator<WMap>	it;
    Iterator<WPair>	row;
    for (it= gt(); it; ++it)  			// initialize counts
        for (row= (*it)(); row; ++row)
            ++count[(*row).first];
    transpose.alloc( dim );			// allocate transpose
    for (cnt = 0; cnt < dim; ++cnt)  {
        (*transpose).alloc(count[cnt]);
        ++transpose;
    };
    for (cnt = 0; cnt < dim; ++cnt)
        for (row= gt[cnt](); row; ++row)
            transpose[ (*row).first ] << WPair(cnt, (*row).second);
    //    PrintM(transpose());        
    return;      
}



