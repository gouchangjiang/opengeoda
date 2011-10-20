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

#include "shp.h"
#include "shp2cnt.h"
#include "shp2gwt.h"
#include "../OpenGeoDa.h"
#include "../GenUtils.h"

#include "../Thiessen/VorDataType.h"
#include "Gauss.h"
const int kAscii = 0;

extern long GetShpFileSize(const wxString& fname); 

oGauss::oGauss(char *nme, const int numRows, const int numColumns,
			   const bool complex)  {
   char	dhtName[256], datName[256];
	GenUtils::extension(dhtName, nme, "dht");
   unsigned char uc1= 0xDA, uc2 = 0xEE;
   dht.open(dhtName, ios::out | ios::binary);
   dht.put(uc1);
   dht.put(uc2);
   const	short	ftype = 8;
   const	int		zero = 0, headerSize= 128+8*numColumns, rowSize= numColumns*8;
   dht.write((char *) &ftype, 2);
   dht.write((char *) &zero, 4);
   dht.write((char *) &numColumns, 4);
   dht.write((char *) &rowSize, 4);
   dht.write((char *) &headerSize, 4);
   dht.write((char *) &zero, 4);
   dht.write((char *) &zero, 4);
   short	flag= 0;
   if (complex) flag |= (1 << 12);		// set fourth bit to 1
   dht.write((char *) &flag, 2);
	// align to 4 bytes and fill with zeros
   dht.write((char *) &zero, 2);
   for (int cnt= 0; cnt < 24; ++cnt)
     dht.write((char *) &zero, 4);		// make 96 zeros
	GenUtils::extension(datName, nme, "dat");
   dat.open(datName, ios::out | ios::binary);
   return;
}

oGauss::~oGauss()  {	// close the files
   dat.close();
   dht.close();
   return;
}

//* produces an info message that a particular name is used more than once
void Warning(char * nme, const int found)  {
    char buf[256]= " Variable: ";
    strcat(buf, nme);
    if (found) strcat(buf, " is not found.");
      else strcat(buf, " is used more than once.");
	wxString myMsg = wxString::Format("%s",buf);
	myMsg += "\nWarning: variable excluded from output";
    wxMessageBox(myMsg);
    return;
}

void SmallSort(int * data, charPtr * nme, const int sz)  {
   int	offset= sz;
   bool	inOrder;
   do  {
     offset= (8 * offset) / 11;
     if (offset == 0) offset= 1;
     inOrder= true;
     for (int ix=0, jx= offset; ix < (sz-offset); ix++, jx++)  {
       if (data[ix] > data[jx])  {
         inOrder= false;
         int	tmp= data[ix];
         data[ix]= data[jx];
         data[jx]= tmp;
         charPtr tmpChar= nme[ix];
         nme[ix]= nme[jx];
         nme[jx]= tmpChar;
       }
     };
   }  while (!(offset == 1 && inOrder == true));
   return;
}

void SmallSort(int * data, charPtr * nme, const int sz, int * idx)  
{
   int	offset= sz;
   bool	inOrder;
   do  {
     offset= (8 * offset) / 11;
     if (offset == 0) offset= 1;
     inOrder= true;
     for (int ix=0, jx= offset; ix < (sz-offset); ix++, jx++)  {
       if (data[ix] > data[jx])  {
         inOrder= false;
         int	tmp= data[ix];
         data[ix]= data[jx];
         data[jx]= tmp;
         tmp= idx[ix];
         idx[ix]= idx[jx];
         idx[jx]= tmp;
         charPtr tmpChar= nme[ix];
         nme[ix]= nme[jx];
         nme[jx]= tmpChar;
       }
     };
   }  while (!(offset == 1 && inOrder == true));
   return;
}

int CleanFields(int * fld, charPtr * nme, const int sz, int* idx)  
{
   int  clean= true;
   SmallSort(fld, nme, sz, idx);
   int	bad= -1, newSize= sz, cp= 0;
   do  {
     while ( fld[cp] == bad  && cp < newSize )  
		 {
       --newSize;
//       Warning(nme[cp], bad == -1);
       clean= false;
       for(int cs= cp; cs < newSize; ++cs)  
			 {
         fld[cs]= fld[cs+1];
         nme[cs]= nme[cs+1];
       };
     };
     bad= fld[cp];
     cp++;
   }  while (cp < newSize);
   return clean ? newSize : -newSize;
}

int CleanFields(int * fld, charPtr * nme, const int sz)  
{
   int  clean= true;
   SmallSort(fld, nme, sz);
   int	bad= -1, newSize= sz, cp= 0;
   do  {
     while ( fld[cp] == bad  && cp < newSize )  
		 {
       --newSize;
//       Warning(nme[cp], bad == -1);
       clean= false;
       for(int cs= cp; cs < newSize; ++cs)  
			 {
         fld[cs]= fld[cs+1];
         nme[cs]= nme[cs+1];
       };
     };
     bad= fld[cp];
     cp++;
   }  while (cp < newSize);
   return clean ? newSize : -newSize;
}

/*==========================================================================*/
/*	input:	input (DBF) file name											*/
/*  output:	output file name												*/
/*  fld	:	an array of field index											*/
/*	zs	:	number of fields being read										*/
/*==========================================================================*/
void ExportAscii(iDBF &input, ofstream &output, int * fld, const int sz)  
{
	const int records= input.Records();
	int current = 0, needPos = 0;
	for (current= 0; current < records; ++current) {
		for (needPos= 0; needPos < sz-1; ++needPos) {
			while (input.Pos() != fld[needPos]) input.Read();
			double result;
			input.Read(result);
			output << result << ", ";
		}
		while (input.Pos() != fld[needPos]) input.Read();
		double result;
		input.Read( result );
		output << result;
		output << endl;
	}
}

void ExportGauss(iDBF &input, oGauss &output, int * fld, const int sz)  
{
	const int records= input.Records();
	for (int cp= 0; cp < sz; ++cp)	// put the names of the fields to be exported into the header
		output.addName( input.field(fld[cp])->Name );
	for (int current= 0; current < records; ++current)  
	{
		for (int needPos= 0; needPos < sz; ++needPos)  
		{
			while (input.Pos() != fld[needPos]) input.Read();
			double    result;
			input.Read( result );
			output.write(result);
		}
	}
	return;
}

//* MakeCopy -- returns a copy of the string
inline char *MakeCopy(char * src)  
{
  char * tmp= new char[strlen(src)+1];
  if (src && *src)  strcpy(tmp, src);
    else strcpy(tmp, "");
  return tmp;
}

//*** all parameters are converted to the UPPER case.
charPtr * CookParams(char * params, int cnt)  
{
	char * ch= params;
	GenUtils::String2Upper(params);
	
	for (int i=0; i< cnt; i++) {
		while (*ch != ',') ++ch;
		*ch++= '\x0';						// replace comma with 'end-of-line'
	}
	
	charPtr * hdl= new charPtr[cnt];			// create vector of pointers
	ch= params;
	for (int cp= 1; cp < cnt; ++cp) {
		hdl[cp-1]= ch;
		while (*ch) ++ch;					// skip to the end of teh parameter
		++ch;								// skip end-of-line; now ch points to the next parameter
	}
	hdl[cnt-1]= ch;
	//* now hdl[0] is the first parameter, hdl[1] is the second, etc.
	//* commas are gone, they are replaced with end-of-line to separate parameters from each other.
	return hdl;
}


void GaussExIm(char * fnme, char *otfl, char * list, int cnt, int outOption)  
{
	char *Fn= MakeCopy(fnme);
	char *Id= MakeCopy(list);
	charPtr *vars= CookParams(Id, cnt);
	long rc= 0;
	iDBF input(Fn);		// open dbf file for input
	int* fld= new int[ cnt ];
	int cp = 0;
	for (cp= 0; cp < cnt; ++cp)  {
		// determine location (column) for each field
		fld[cp]= input.FindField(wxString(vars[cp]));	
	}
	cnt= CleanFields(fld, vars, cnt);
	if (cnt < 0) { rc= 1; cnt= -cnt; }
	if (cnt > 0) {
		// file is Ok, fields are Ok -> do output
		if (outOption == 0) {			
			// output as Ascii file
			ofstream   output(otfl);

			output << input.Records() << ", " << cnt << endl;

			for (cp= 0; cp < cnt-1; ++cp)
				output << "\"" << vars[cp] << "\"" << ", "; 
			output << "\"" << vars[cp] << "\"";

			output << endl;
			output.precision(15);
			ExportAscii(input, output, fld, cnt);
		}  
		else   
		{						
			// output as Gauss 32 bit binary
			oGauss	output(otfl, input.Records(), cnt, false);
			ExportGauss(input, output, fld, cnt);
		}
	}
	// clean up

	delete [] Fn;
	Fn = NULL;
	delete [] Id;
	Id = NULL;
	delete [] vars;
	vars = NULL;
	delete [] fld;
	fld = NULL;
	return;
}

bool AddCentroidsDBF(wxString fname, wxString odbfo, wxString keyname, 
					 double* key, const std::vector<wxString>& keyst,
					 bool keytype, bool m_mean_center)
{
	if (fname.IsEmpty() || odbfo.IsEmpty() || keyname.IsEmpty() )
		return false;

	char nmX[12]="X_COORD", nmY[12]="Y_COORD";
	nmX[8]= '\x0';
	nmY[8]= '\x0';

	char  otfile[128];
	strcpy(otfile, odbfo);

	long obs = GetShpFileSize(fname);
	
	if (obs < 1)
		return false;

	myBox *B=new myBox;
	vector<double> x(obs);
	vector<double> y(obs);

	long np = obs;
	if (!ComputeXY(fname, &np, x, y, B, m_mean_center))
		return false;

	int	posX=0, posY=0;
	delete B;
	B = NULL;

	iDBF idbf(fname);		// open dbf file for input
	int	nfields = idbf.GetNumOfField();
	int nfieldsx = nfields;
	posX = idbf.FindField(wxString(nmX));
	posY = idbf.FindField(wxString(nmY));

	int k=0;

	//int *fld = new int[nfields];
	//char *ty = new char[nfields];
	//int  *w  = new int[nfields];

	vector<int> fld(nfields);
	vector<char> ty(nfields);
	vector<int> w(nfields);


	for (int i=0; i<nfields ; i++) 
	{
		ty.at(i) = idbf.field(i)->Type;
		w.at(i)  = idbf.field(i)->Width;
		if (i != posX && i != posY) {
			if (ty.at(i) == 'N' || ty.at(i) == 'C' ||
				ty.at(i) == 'F' || ty.at(i) == 'D' ) 
			{
				fld.at(k) = i;
				k++;

			}
		}
	}

	DBF_descr *dbfdesc = new DBF_descr [4];

	dbfdesc[0]		= new DBF_field("RECNUM", 'N',8,0);
	if (keytype)
		dbfdesc[1]  = new DBF_field(keyname, 'N',16,6);
	else
		dbfdesc[1]  = new DBF_field(keyname, 'C',9);
	dbfdesc[2]		= new DBF_field(nmX, 'N',16,6);
	dbfdesc[3]		= new DBF_field(nmY, 'N',16,6);

	oDBF odbf(odbfo, dbfdesc, obs,4);
	
	const int records= idbf.Records();

	for (long int current= 0; current < records; ++current)  
	{
		long curn = current + 1;
		odbf.Write(curn);
		if (keytype) 
		{
			double keyval = key[current];
			odbf.Write(keyval);
		}
		else 
		{
			odbf.Write(keyst[current]);
		}
		odbf.Write(x.at(current));
		odbf.Write(y.at(current));		
	}


	odbf.close();

	delete [] dbfdesc;
	dbfdesc = NULL;

	return true;
}


