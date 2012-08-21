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

#ifndef __GEODA_CENTER_THIESSEN_POLYGON_DLG_H__
#define __GEODA_CENTER_THIESSEN_POLYGON_DLG_H__

extern long GetShpFileSize(const wxString& fname);
extern bool CreatePolygonShapeFile(char* otfl, char *infl, const int nPolygon,
								   myPolygon* Polygons, myBox B);
extern bool CreatePointShapeFile(char* otfl, char* infl, const int nPoint,
								 vector<double>& x, vector<double>& y, myBox myfBox);

extern void	ReadOffsets(const wxString& flnm);
extern int XYSort(const void* , const void*);

////@begin includes
////@end includes

/*!
 * Control identifiers
 */

////@begin control identifiers
#define IDD_VORONOI 10000
//#define IDC_STATIC 10010
//#define IDC_BROWSE_IFILE 10015
//#define IDC_VORONOI_INFILE 10001
//#define ID_STATICTEXT 10004
//#define IDC_BROWSE_OFILE 10002
//#define IDC_VORONOI_OTFILE 10003
//#define IDC_REFERENCEFILE_CHK 10016
//#define IDC_VORONOI_REFFILE 10014
//#define IDC_REFERENCEFILE 10017
#define ID_MAP1 10007
#define ID_MAP2 10012
#define IDCREATE 10005
//#define IDC_VOR_RESET 10006
//#define IDCANCEL 10008
////@end control identifiers

/*!
 * Compatibility
 */  

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ThiessenPolygonDlg class declaration
 */

class ThiessenPolygonDlg: public wxDialog
{    
    DECLARE_CLASS( ThiessenPolygonDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ThiessenPolygonDlg( );
    ThiessenPolygonDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "SHAPE CONVERSION", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );
	ThiessenPolygonDlg(bool mean_center, bool fltype, wxWindow* pParent, wxWindowID id = -1, const wxString& caption = "SHAPE CONVERSION", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "SHAPE CONVERSION", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ThiessenPolygonDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BROWSE_IFILE
    void OnCBrowseIfileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BROWSE_OFILE
    void OnCBrowseOfileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_REFERENCEFILE_CHK
    void OnCReferencefileChkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_REFERENCEFILE
    void OnCReferencefileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCREATE
    void OnCreateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_VOR_RESET
    void OnCVorResetClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end ThiessenPolygonDlg event handler declarations

////@begin ThiessenPolygonDlg member function declarations

////@end ThiessenPolygonDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ThiessenPolygonDlg member variables
    wxTextCtrl* m_inputfile;
    wxTextCtrl* m_outputfile;
    wxCheckBox* m_check;
    wxTextCtrl* m_boundingfile;
////@end ThiessenPolygonDlg member variables

//	bool		Ismap3;
//	bool		Ismap4;
	wxString		inFile, otFile, o_path;
	wxString fn;
	int m_nCount;
	int m_nTimer;
	bool m_fltype, m_mean_center;

	enum { nMaxCount = 10000 };
	myPolygon* myP;
	Thiessen*	TT;
	char*		c_RefFile;

	void InsertAreaField_dbf(wxString otfl, double* Area);
	wxString GetInFileName() {return inFile; };
	wxString GetOtFileName() {return otFile; };
	void RedoTheProcess();

	int CheckDuplicate(myPoint *pt, long nP);

};

#endif
    // _THIESSENPOLYGONDLG_H_
