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

#ifndef __GEODA_CENTER_DBF_2_SHP_DLG_H__
#define __GEODA_CENTER_DBF_2_SHP_DLG_H__

////@begin control identifiers
#define IDD_CONVERT_DBF2SHP 10000
#define IDOK_ADD 10001
#define IDOKDONE 10010
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

class DBF2SHPDlg: public wxDialog
{    
    DECLARE_CLASS( DBF2SHPDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DBF2SHPDlg( );
    DBF2SHPDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "Convert DBF to SHP", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "Convert DBF to SHP", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DBF2SHPDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDOK_ADD
    void OnOkAddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_OSHP
    void OnCOpenOshpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDOKDONE
    void OnOkdoneClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_IDBF
    void OnCOpenIdbfClick( wxCommandEvent& event );

////@end DBF2SHPDlg event handler declarations

////@begin DBF2SHPDlg member function declarations

////@end DBF2SHPDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DBF2SHPDlg member variables
    wxTextCtrl* m_inputfile;
    wxTextCtrl* m_outputfile;
    wxChoice* m_X;
    wxChoice* m_Y;
////@end DBF2SHPDlg member variables

	wxString fn;
};

#endif
    // _DBF2SHPDLG_H_
