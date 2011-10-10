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

#ifndef __GEODA_CENTER_LISA_WHAT_2_OPEN_DLG_H__
#define __GEODA_CENTER_LISA_WHAT_2_OPEN_DLG_H__

////@begin control identifiers
#define IDD_LISAWINDOWS2OPEN 10000

class CLisaWhat2OpenDlg: public wxDialog
{    
    DECLARE_CLASS( CLisaWhat2OpenDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CLisaWhat2OpenDlg( );
    CLisaWhat2OpenDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "What windows to open?", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "What windows to open?", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CLisaWhat2OpenDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end CLisaWhat2OpenDlg event handler declarations

////@begin CLisaWhat2OpenDlg member function declarations

////@end CLisaWhat2OpenDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CLisaWhat2OpenDlg member variables
    wxCheckBox* m_check1;
    wxCheckBox* m_check2;
    wxCheckBox* m_check3;
    wxCheckBox* m_check4;
////@end CLisaWhat2OpenDlg member variables

	bool m_SigMap;
	bool m_ClustMap;
	bool m_BoxPlot;
	bool m_Moran;


};

#endif
    // _LISAWHAT2OPENDLG_H_
