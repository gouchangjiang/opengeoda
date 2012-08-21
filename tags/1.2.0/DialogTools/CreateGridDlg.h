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

#ifndef __GEODA_CENTER_CREATE_GRID_DLG_H__
#define __GEODA_CENTER_CREATE_GRID_DLG_H__

extern bool CreateGridShapeFile(wxString otfl, int nRows,
								int nCols, double *xg, double *yg,
								myBox myfBox); 

extern int XYSort(const void* , const void*);

#define IDD_CREATE_GRID 10000
#define ID_CREATE 10025

class CreateGridDlg: public wxDialog
{    
    DECLARE_CLASS( CreateGridDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CreateGridDlg( );
    CreateGridDlg( wxWindow* parent, wxWindowID id = -1,
				   const wxString& caption = "Creating Grid",
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Creating Grid",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CreateGridDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_REFERENCEFILE
    void OnCReferencefileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BROWSE_OFILE
    void OnCBrowseOfileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_REFERENCEFILE2
    void OnCReferencefile2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for IDC_EDIT1
    void OnCEdit1Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for IDC_EDIT3
    void OnCEdit3Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for IDC_EDIT2
    void OnCEdit2Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for IDC_EDIT4
    void OnCEdit4Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CREATE
    void OnCreateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for IDC_RADIO1
    void OnCRadio1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for IDC_RADIO2
    void OnCRadio2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for IDC_RADIO3
    void OnCRadio3Selected( wxCommandEvent& event );

////@end CreateGridDlg event handler declarations

////@begin CreateGridDlg member function declarations

////@end CreateGridDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CreateGridDlg member variables
    wxTextCtrl* m_outputfile;
    wxTextCtrl* m_inputfile_ascii;
    wxTextCtrl* m_lower_x;
    wxTextCtrl* m_upper_x;
    wxTextCtrl* m_lower_y;
    wxTextCtrl* m_upper_y;
    wxTextCtrl* m_inputfileshp;
    wxTextCtrl* m_rows;
    wxTextCtrl* m_cols;
////@end CreateGridDlg member variables

	void EnableItems();
	bool CheckBBox();
	void CreateGrid();  

	int m_check;

	int		m_nCount;
	int		m_nTimer;
	enum { nMaxCount = 10000 };
	Box m_BigBox;
	double	m_xBot,m_yBot,m_xTop,m_yTop;
	bool		hasCreated;

	wxString s_lower_x, s_lower_y, s_top_x, s_top_y;
	wxString s_row, s_col, fn;

	bool isCreated;
};

#endif
    // _CREATEGRIDDLG_H_
