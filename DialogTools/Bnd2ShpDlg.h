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

#ifndef __GEODA_CENTER_BND2SHP_DLG_H__
#define __GEODA_CENTER_BND2SHP_DLG_H__

#define IDD_CONVERT_BOUNDARY_TO_SHP 10000
//#define IDC_FIELD_ASC 10002
//#define IDC_FIELD_SHP 10005
#define ID_STATIC_BOX 10009

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * Bnd2ShpDlg class declaration
 */

class Bnd2ShpDlg: public wxDialog
{    
    DECLARE_CLASS( Bnd2ShpDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    Bnd2ShpDlg( );
    Bnd2ShpDlg( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Convert Boundary to SHP",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Convert Boundary to SHP",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin Bnd2ShpDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CREATE
    void OnCreateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_IASC
    void OnCOpenIascClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_OSHP
    void OnCOpenOshpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCANCEL
    void OnCancelClick( wxCommandEvent& event );
    static bool ShowToolTips();

////@begin Bnd2ShpDlg member variables
    wxTextCtrl* m_inputfile;
    wxTextCtrl* m_outputfile;
////@end Bnd2ShpDlg member variables

	wxString fn;
};

#endif
    // _BND2SHPDLG_H_
