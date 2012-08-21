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

#include <wx/xrc/xmlres.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../ShapeOperations/ShapeFileTypes.h"
#include "../ShapeOperations/ShapeFileTriplet.h"
#include "../Thiessen/VorDataType.h"

#include "DBF2SHPDlg.h"


IMPLEMENT_CLASS( DBF2SHPDlg, wxDialog )


BEGIN_EVENT_TABLE( DBF2SHPDlg, wxDialog )

    EVT_BUTTON( XRCID("IDOK_ADD"), DBF2SHPDlg::OnOkAddClick )
    EVT_BUTTON( XRCID("IDC_OPEN_OSHP"), DBF2SHPDlg::OnCOpenOshpClick )
    EVT_BUTTON( XRCID("IDOKDONE"), DBF2SHPDlg::OnOkdoneClick )
    EVT_BUTTON( XRCID("IDC_OPEN_IDBF"), DBF2SHPDlg::OnCOpenIdbfClick )

END_EVENT_TABLE()

/*!
 * DBF2SHPDlg constructors
 */


bool CreatePointShapeFile(
                char* otfl,         // Target File name
                const int nPoint,   // Number of point or records
                double *x,          // x-coordinates vector
                double *y)          // y-ccordinates vector
{
	
	//Do we really need this code?
	DPOINT pp1, pp2;
	double min1_x, min1_y, max1_x, max1_y;
	min1_x = 1e255;
	min1_y = 1e255;
	max1_x = -1e255;
	max1_y = -1e255;
	
	double xv,yv;
	
	for(int rec1=0; rec1<nPoint; rec1++) {
		xv = x[rec1];
		yv = y[rec1];
		
		if(xv < min1_x)
			min1_x = xv;
		if(yv < min1_y)
			min1_y = yv;
		if(xv > max1_x)
			max1_x = xv;
		if(yv > max1_y)
			max1_y = yv;
	}
	
	pp1.x = min1_x;
	pp1.y = min1_y;
	pp2.x = max1_x;
	pp2.y = max1_y;
	
	myBox myfBox;
	myfBox.p1 = pp1;
	myfBox.p2 = pp2;
	////////////////////////////////
	// Declare the bounding box as Box class object
	BasePoint p1(myfBox.p1.x,myfBox.p1.y);
	BasePoint p2(myfBox.p2.x,myfBox.p2.y);
	Box xoBox(p1,p2);
	
	// Declare the triplet (the .shp, .shx, .dbf) as oShapeFileTriplet class object
	oShapeFileTriplet Triple(otfl,xoBox, "POLY", ShapeFileTypes::SPOINT);
	// Allocate a pointer of AbstractShape to manage the point Shapefile
	AbstractShape* shape = new Ppoint;
	
	// store the point dataset into the triplet and
	// at the same time re-evaluate the bounding box according to
	// the dataset
	
	double max_x=x[0], max_y=y[0], min_x=x[0], min_y=y[0];
	
	for (long rec=0; rec < nPoint; rec++) {
		shape->AssignPointData(x[rec],y[rec]);
		Triple << *shape;
		if (max_x < x[rec]) max_x = x[rec];
		if (min_x > x[rec]) min_x = x[rec];
		if (max_y < y[rec]) max_y = y[rec];
		if (min_y > y[rec]) min_y = y[rec];
	}
	
	// Update (to make wider) the bounding box by delta_x and and delta_y
	double delta_x = (max_x - min_x)/20;
	double delta_y = (max_y - min_y)/20;
	
	min_x = min_x - delta_x;
	min_y = min_y - delta_y;
	max_x = max_x + delta_x;
	max_y = max_y + delta_y;
	
	BasePoint a(min_x,min_y);
	BasePoint b(max_x,max_y);
	Box fBox(a,b);
	
	// Update the bounding box
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	
	delete shape;
	shape = NULL;
	
	return true;
}


DBF2SHPDlg::DBF2SHPDlg( )
{
}

DBF2SHPDlg::DBF2SHPDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);


	FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR2"))->Enable(false);
	FindWindow(XRCID("IDOK_ADD"))->Enable(false);


}

/*!
 * DBF2SHPDlg creator
 */

bool DBF2SHPDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DBF2SHPDlg member initialisation
////@end DBF2SHPDlg member initialisation

////@begin DBF2SHPDlg creation
    SetParent(parent);
    CreateControls();
    Centre();
////@end DBF2SHPDlg creation
    return true;
}

/*!
 * Control creation for DBF2SHPDlg
 */

void DBF2SHPDlg::CreateControls()
{    
////@begin DBF2SHPDlg content construction

    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_CONVERT_DBF2SHP");
    m_inputfile = XRCCTRL(*this, "IDC_FIELD_DBF", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_FIELD_SHP", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_X = XRCCTRL(*this, "IDC_KEYVAR", wxChoice);
    m_Y = XRCCTRL(*this, "IDC_KEYVAR2", wxChoice);
////@end DBF2SHPDlg content construction

    // Create custom windows not generated automatically here.

////@begin DBF2SHPDlg content initialisation

////@end DBF2SHPDlg content initialisation
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDOK_ADD
 */

void DBF2SHPDlg::OnOkAddClick( wxCommandEvent& event )
{
    // Insert custom code here


	wxString m_iDBF = m_inputfile->GetValue();
	wxString m_oSHP = m_outputfile->GetValue();

	iDBF tb(m_iDBF);
	
	int numfields = tb.GetNumOfField();

	int n_recs = tb.GetNumOfRecord(); 

	int idx_x = m_X->GetSelection();
	int idx_y = m_Y->GetSelection();

	double *x,*y;
	x = new double[n_recs];
	y = new double[n_recs];

	char ty_x = tb.GetFieldType(idx_x); 
	char ty_y = tb.GetFieldType(idx_y); 

	if(ty_x == 'N')
	{
        	tb.GetDblDataArray(wxString(tb.GetFieldName(idx_x)),x);
	}
	else
	{
		wxMessageBox("Unsupported X-coordinate type!");
		delete [] x;
		x = NULL;
		delete [] y;
		y = NULL;
		return;
	}

	if(ty_y == 'N')
	{
		iDBF tb2(m_iDBF);
		tb2.GetDblDataArray(wxString(tb.GetFieldName(idx_y)),y);

	}
	else
	{
		wxMessageBox("Unsupported Y-coordinate type!");
		delete [] x;
		x = NULL;
		delete [] y;
		y = NULL;
		return;
	}

	char buf_ofl[512];
	strcpy( buf_ofl, (const char*)m_oSHP.mb_str(wxConvUTF8) );
	char* ofl = buf_ofl;

	CreatePointShapeFile(ofl, n_recs, x, y);

	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;

	wxString strResult = m_oSHP;
	wxString DirName;

	int pos = strResult.Find('.',true);
	if (pos >= 0) 
		DirName = strResult.Left(pos);
	else 
		DirName = strResult;

	DirName = DirName+ ".dbf";

	char buf_nfl[512];
	strcpy( buf_nfl, (const char*)DirName.mb_str(wxConvUTF8) );
	char* nfl = buf_nfl;

	//Read only problem
	wxRemoveFile(DirName);

	if(wxCopyFile(m_iDBF, DirName, false))
	{
		event.Skip(); // wxDialog::OnOK(event);
		EndDialog(wxID_OK);		
	}
	else
	{
		wxMessageBox("Can't create DBF file associated with SHP file!");
    }

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_OSHP
 */

void DBF2SHPDlg::OnCOpenOshpClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxFileDialog dlg
                 (
                    this,
                    "Output Shp file",
                    wxEmptyString,
                    fn + ".shp",
                    "Shp files (*.shp)|*.shp",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

	wxString	m_path = wxEmptyString;// m_path contains the path and the file name + ext


    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);


		FindWindow(XRCID("IDOK_ADD"))->Enable(true);
	}

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDOKDONE
 */

void DBF2SHPDlg::OnOkdoneClick( wxCommandEvent& event )
{
    // Insert custom code here
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_IDBF
 */

void DBF2SHPDlg::OnCOpenIdbfClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxFileDialog dlg
                 (
                    this,
                    "Input DBF file",
                    "",
                    "",
                    "Dbf files (*.dbf)|*.dbf"
                 );

	wxString	m_path = wxEmptyString;// m_path contains the path and the file name + ext


    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString InFile = m_path;
		m_inputfile->SetValue(InFile);

                fn = dlg.GetFilename();
                int pos = fn.Find('.', true);
                if (pos >= 0) fn = fn.Left(pos);
        
		iDBF tb(InFile);
		if (tb.IsConnectedToFile()) 
		{
			int numfields; 
			numfields = tb.GetNumOfField();
			m_X->Clear();
			m_Y->Clear();

			for (int i=0; i<numfields; i++) 
			{
				wxString s = wxString::Format("%s",tb.GetFieldName(i));
				if (tb.GetFieldType(i)== 'N' || 
				    tb.GetFieldType(i)== 'C' ||
						tb.GetFieldType(i)== 'F')
				{
					m_X->Append(s);
					m_Y->Append(s);
				}
			}

			m_X->SetSelection(0);
			m_Y->SetSelection(0);

			FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(true);
			FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(true);
			FindWindow(XRCID("IDC_KEYVAR"))->Enable(true);
			FindWindow(XRCID("IDC_KEYVAR2"))->Enable(true);
		} // connection
		else
		{
			wxMessageBox("Error: Please check header line of weights file.");
		}// no connection

	}


}

/*!
 * Should we show tooltips?
 */

bool DBF2SHPDlg::ShowToolTips()
{
    return true;
}

