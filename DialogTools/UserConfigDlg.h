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

#ifndef __GEODA_CENTER_USER_CONFIG_DLG_H__
#define __GEODA_CENTER_USER_CONFIG_DLG_H__

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "UserConfigDlg.cpp"
#endif


////@begin control identifiers
#define IDD_USERCONFIG_DIALOG 10041
#define SYMBOL_CUSERCONFIGDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CUSERCONFIGDLG_TITLE "User Configure"
#define SYMBOL_CUSERCONFIGDLG_IDNAME IDD_USERCONFIG_DIALOG
#define SYMBOL_CUSERCONFIGDLG_SIZE wxSize(400, 300)
#define SYMBOL_CUSERCONFIGDLG_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

/*!
 * UserConfigDlg class declaration
 */

class UserConfigDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( UserConfigDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    UserConfigDlg( );
    UserConfigDlg( wxWindow* parent, wxWindowID id = SYMBOL_CUSERCONFIGDLG_IDNAME, const wxString& caption = SYMBOL_CUSERCONFIGDLG_TITLE, const wxPoint& pos = SYMBOL_CUSERCONFIGDLG_POSITION, const wxSize& size = SYMBOL_CUSERCONFIGDLG_SIZE, long style = SYMBOL_CUSERCONFIGDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CUSERCONFIGDLG_IDNAME, const wxString& caption = SYMBOL_CUSERCONFIGDLG_TITLE, const wxPoint& pos = SYMBOL_CUSERCONFIGDLG_POSITION, const wxSize& size = SYMBOL_CUSERCONFIGDLG_SIZE, long style = SYMBOL_CUSERCONFIGDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin UserConfigDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end UserConfigDlg event handler declarations

////@begin UserConfigDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end UserConfigDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin UserConfigDlg member variables
    wxStaticText* m_label;
    wxTextCtrl* m_min;
    wxTextCtrl* m_max;
////@end UserConfigDlg member variables

	wxString s_int;
	wxString s_int2;

};

#endif
    // _USERCONFIGDLG_H_
