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

#include <wx/valtext.h>

#include <wx/image.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces


#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../Thiessen/VorDataType.h"

#include "../DialogTools/Statistics.h"
#include "../Thiessen/Thiessen.h"
#include "../GenUtils.h"

#include "ThiessenPolygonDlg.h"

/*!
 * ThiessenPolygonDlg type definition
 */

IMPLEMENT_CLASS( ThiessenPolygonDlg, wxDialog )

/*!
 * ThiessenPolygonDlg event table definition
 */

BEGIN_EVENT_TABLE( ThiessenPolygonDlg, wxDialog )

////@begin ThiessenPolygonDlg event table entries
    EVT_BUTTON( XRCID("IDC_BROWSE_IFILE"), ThiessenPolygonDlg::OnCBrowseIfileClick )

    EVT_BUTTON( XRCID("IDC_BROWSE_OFILE"), ThiessenPolygonDlg::OnCBrowseOfileClick )

    EVT_CHECKBOX( XRCID("IDC_REFERENCEFILE_CHK"), ThiessenPolygonDlg::OnCReferencefileChkClick )

    EVT_BUTTON( XRCID("IDC_REFERENCEFILE"), ThiessenPolygonDlg::OnCReferencefileClick )

    EVT_BUTTON( XRCID("IDCREATE"), ThiessenPolygonDlg::OnCreateClick )

    EVT_BUTTON( XRCID("IDC_VOR_RESET"), ThiessenPolygonDlg::OnCVorResetClick )

    EVT_BUTTON( XRCID("IDCANCEL"), ThiessenPolygonDlg::OnCancelClick )

////@end ThiessenPolygonDlg event table entries

END_EVENT_TABLE()
  
/*!
 * ThiessenPolygonDlg constructors
 */

ThiessenPolygonDlg::ThiessenPolygonDlg( )
{
	myP = NULL;
	TT = NULL;
	c_RefFile = NULL;
//	m_map3 = NULL;
//	m_map4 = NULL;
}

ThiessenPolygonDlg::ThiessenPolygonDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
	myP = NULL;
	TT = NULL;
	c_RefFile = NULL;
//	m_map3 = NULL;
//	m_map4 = NULL;

	Create(parent, id, caption, pos, size, style);
}

ThiessenPolygonDlg::ThiessenPolygonDlg(bool mean_center, bool fltype, wxWindow* pParent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{

//	Ismap3 = false;
//	Ismap4 = false;
	inFile = wxEmptyString;
	otFile = wxEmptyString;
	o_path = wxEmptyString;
	m_nCount = 0;
	myP = NULL;
	TT = NULL;
	c_RefFile = NULL;
//	m_map3 = NULL;
//	m_map4 = NULL;
	m_fltype = fltype;
	m_mean_center = mean_center;

	Create(pParent, id, caption, pos, size, style);



	FindWindow(XRCID("IDCREATE"))->Enable(false);
	FindWindow(XRCID("IDC_VOR_RESET"))->Enable(false);
	FindWindow(XRCID("IDC_BROWSE_OFILE"))->Enable(false);
	FindWindow(XRCID("IDC_REFERENCEFILE_CHK"))->Enable(false);
	FindWindow(XRCID("IDC_REFERENCEFILE"))->Enable(false);

	

}

/*!
 * ThiessenPolygonDlg creator
 */

bool ThiessenPolygonDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ThiessenPolygonDlg member initialisation
////@end ThiessenPolygonDlg member initialisation

////@begin ThiessenPolygonDlg creation
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ThiessenPolygonDlg creation
    return true;
}

/*!
 * Control creation for ThiessenPolygonDlg
 */

void ThiessenPolygonDlg::CreateControls()
{    
////@begin ThiessenPolygonDlg content construction

    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_VORONOI");
    m_inputfile = XRCCTRL(*this, "IDC_VORONOI_INFILE", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_VORONOI_OTFILE", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_check = XRCCTRL(*this, "IDC_REFERENCEFILE_CHK", wxCheckBox);
    m_boundingfile = XRCCTRL(*this, "IDC_VORONOI_REFFILE", wxTextCtrl);
	m_boundingfile->SetMaxLength(0);
////@end ThiessenPolygonDlg content construction

    // Create custom windows not generated automatically here.
  
////@begin ThiessenPolygonDlg content initialisation

////@end ThiessenPolygonDlg content initialisation
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON
 */

void ThiessenPolygonDlg::OnCBrowseOfileClick( wxCommandEvent& event )
{
    // Insert custom code here
 
//	if (Ismap4) 
//		map2->Remove();
//	Ismap4 = false;

    wxFileDialog dlg
                 (
                    this,
                    "Output Shapefile",
                    wxEmptyString,
                    fn + ".shp",
                    "Shp files (*.shp)|*.shp",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

    if (dlg.ShowModal() == wxID_OK) {
		o_path = dlg.GetPath();
		otFile = o_path;
		m_outputfile->SetValue(o_path);
 
		FindWindow(XRCID("IDC_REFERENCEFILE_CHK"))->Enable(true);
		FindWindow(XRCID("IDCREATE"))->Enable(true);
	}
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCREATE
 */

void ThiessenPolygonDlg::OnCreateClick( wxCommandEvent& event )
{
    // Insert custom code here

	FindWindow(XRCID("IDCREATE"))->Enable(false);

	inFile = m_inputfile->GetValue();
	otFile = m_outputfile->GetValue();

	char buf_infl[512];
	strcpy( buf_infl, (const char*)inFile.mb_str(wxConvUTF8) );
	char* infl = buf_infl;

	char buf_otfl[512];
	strcpy( buf_otfl, (const char*)otFile.mb_str(wxConvUTF8) );
	char* otfl = buf_otfl;

	long nPoints; myBox* B=new myBox;
	nPoints = GetShpFileSize(inFile);
	
	vector<double> x(nPoints+1);
	vector<double> y(nPoints+1);
  
	
	wxString m_RefFile = m_boundingfile->GetValue();

	m_RefFile.Trim(false);

	if (m_check->GetValue() && m_RefFile != wxEmptyString)
	{

		if (!ComputeXY(inFile, &nPoints, x, y, B, m_mean_center, m_RefFile))
		{
			RedoTheProcess();
			return;
		} 
	}
	else
	{

		if (!ComputeXY(inFile, &nPoints, x, y, B, m_mean_center))
		{
			RedoTheProcess();
			return;
		}

	}


	myPoint *pt = NULL;
	pt = new myPoint[nPoints+1];

	if (pt == NULL) 
	{
		RedoTheProcess();
		return;

	}

	for (int i=0;i<nPoints;i++) {
		pt[i].x = x.at(i); pt[i].y = y.at(i);
	}
	int (*compare) (const void*, const void*);
	compare = XYSort;
	qsort((void*) pt, nPoints, sizeof(myPoint), compare);

	int k=CheckDuplicate(pt, nPoints);

	if (k>0) {
		wxMessageBox(wxString::Format("Error!!, There are %d non-unique points",
									  k));
		OnCVorResetClick(event);
		return;
	}

	myBox BB;
	BB.p1.x = B->p1.x;BB.p1.y = B->p1.y;
	BB.p2.x = B->p2.x;BB.p2.y = B->p2.y;

	bool drawMap = false;

	char dbffl[128];
	GenUtils::extension(dbffl,infl,"dbf");
	if (m_fltype) 
	{
		TT = new Thiessen(nPoints,x,y,BB);
		if (TT == NULL) 
		{
			wxMessageBox("Fail in creating Thiessen Polygons!");
			RedoTheProcess();
			return;
		}		

		myP = TT->GetPolygons();
		BB.p1.x -= 0.01; BB.p1.y -= 0.01;
		BB.p2.x += 0.01; BB.p2.y += 0.01;

		if ((drawMap = CreatePolygonShapeFile(otfl, dbffl, nPoints, myP, BB)))
		{

		}
		else
		{
			wxMessageBox("Fail in writing the output file!");
			RedoTheProcess();
			return;
		}

	}
	else 
	{
		if ((drawMap = CreatePointShapeFile(otfl, dbffl, nPoints, x,y, BB))) 
		{

		}
		else
		{
			wxMessageBox("Fail in writing the output file!");
			RedoTheProcess();
			return;
		}

	}
	delete [] pt;
	pt = NULL;

	wxMessageBox("Successfully created!");
	event.Skip(); // wxDialog::OnOK(event);

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_VOR_RESET
 */

void ThiessenPolygonDlg::OnCVorResetClick( wxCommandEvent& event )
{

//	if (Ismap4) 
//		map2->Remove();
//	if (Ismap3) 
//		map1->Remove();

//	Ismap3 = false;
//	Ismap4 = false;
	inFile = wxEmptyString;
	otFile = wxEmptyString;

	m_inputfile->SetValue(wxEmptyString);
	m_outputfile->SetValue(wxEmptyString);
	m_boundingfile->SetValue(wxEmptyString);

	m_check->SetValue(false);
	o_path = wxEmptyString;
	m_nCount = 0;
	

	FindWindow(XRCID("IDCREATE"))->Enable(false);
	FindWindow(XRCID("IDC_BROWSE_OFILE"))->Enable(false);
	FindWindow(XRCID("IDC_VOR_RESET"))->Enable(false);
	FindWindow(XRCID("IDC_REFERENCEFILE_CHK"))->Enable(false);
	FindWindow(XRCID("IDC_REFERENCEFILE"))->Enable(false);
//	FindWindow(XRCID("IDCANCEL"))->Enable(false);	
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCANCEL
 */

void ThiessenPolygonDlg::OnCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
	if (m_nCount == 0) {      // prior to Start button
		event.Skip(); // wxDialog::OnCancel(event);
	}
	else {                    // computation in progress
		m_nCount = nMaxCount; // Force exit from OnStart
		event.Skip(); // wxDialog::OnCancel(event);
	}
	EndDialog(wxID_CANCEL);
	
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_REFERENCEFILE
 */

void ThiessenPolygonDlg::OnCReferencefileClick( wxCommandEvent& event )
{
    // Insert custom code here

    wxFileDialog dlg
                 (
                    this,
                    "Input Shp file",
                    "",
                    "",
                    "Shp files (*.shp)|*.shp"
                 );


	wxString	m_path = wxEmptyString;// m_path contains the path and the file name + ext

    if (dlg.ShowModal() == wxID_OK)
    {
		m_path = dlg.GetPath();
		m_boundingfile->SetValue(m_path);
	}


}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BROWSE_IFILE
 */

void ThiessenPolygonDlg::OnCBrowseIfileClick( wxCommandEvent& event )
{
    // Insert custom code here


	wxString msg1="This file is not a point Shapefile";
	wxString msg2="This file is not a polygon Shapefile";


    wxFileDialog dlg
                 (
                    this,
                    "Input Shp file",
                    "",
                    "",
                    "Shp files (*.shp)|*.shp"
                 );


	wxString	m_path = wxEmptyString; // m_path contains the path and
										//the file name + ext

    if (dlg.ShowModal() == wxID_OK) {
		m_path = dlg.GetPath();

        fn = dlg.GetFilename();
        int pos = fn.Find('.', true);
        if (pos >= 0) fn = fn.Left(pos);
        

		ReadOffsets(m_path);

		if (IsPointShapeFile(m_path)==m_fltype)
		{

//			if (Ismap3) 
//				map1->Remove();
//			if (Ismap4) 
//				map2->Remove();

//			Ismap3 = Ismap4 = false;

//			map1->AddMap(m_path);

//			Ismap3 = true;
//			Ismap4 = false;

			inFile = m_path;
			otFile = wxEmptyString;

			m_inputfile->SetValue(m_path);

			FindWindow(XRCID("IDC_BROWSE_OFILE"))->Enable(true);
			FindWindow(XRCID("IDC_VOR_RESET"))->Enable(true);
		}
		else {
			if (m_fltype) 
				wxMessageBox(msg1);
			else 
				wxMessageBox(msg2);
		}


	}
}

/*!
 * Should we show tooltips?
 */

bool ThiessenPolygonDlg::ShowToolTips()
{
    return true;
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_REFERENCEFILE_CHK
 */

void ThiessenPolygonDlg::OnCReferencefileChkClick( wxCommandEvent& event )
{
    // Insert custom code here
	m_boundingfile->SetValue(wxEmptyString);
	FindWindow(XRCID("IDC_REFERENCEFILE"))->Enable(m_check->GetValue());

}




void ThiessenPolygonDlg::InsertAreaField_dbf(wxString otfl, double* Area)
{

}

void ThiessenPolygonDlg::RedoTheProcess()
{
	FindWindow(XRCID("IDCREATE"))->Enable(false);
	FindWindow(XRCID("IDC_BROWSE_OFILE"))->Enable(true);
	FindWindow(XRCID("IDC_VOR_RESET"))->Enable(true);
	FindWindow(XRCID("IDCANCEL"))->Enable(true);
}

int ThiessenPolygonDlg::CheckDuplicate(myPoint *pt, long nP)
{
	int k=0;
	double dx = (pt[nP-1].x - pt[0].x) / 1E32;
	double dy = (pt[nP-1].y - pt[0].y) / 1E32;
	for (long i=1; i<nP;i++) 
	{
		if ((pt[i].x == pt[i-1].x) && 
			(pt[i].y == pt[i-1].y)) 
		{
			k++;
			pt[i-1].y += dy;
			pt[i-1].x += dx;
			dy += dy/ (double) k;
			dx += dx/ (double) k;
		}
	}
	return k-1;	

}

