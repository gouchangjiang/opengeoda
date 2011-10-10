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

#ifndef __GEODA_CENTER_HIST_INTERVAL_DLG_H__
#define __GEODA_CENTER_HIST_INTERVAL_DLG_H__

#define IDD_INTERVALS 10000

class CHistIntervalDlg: public wxDialog
{    
    DECLARE_CLASS( CHistIntervalDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CHistIntervalDlg( );
    CHistIntervalDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "Intervals in the Histogram", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "Intervals in the Histogram", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CHistIntervalDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end CHistIntervalDlg event handler declarations

////@begin CHistIntervalDlg member function declarations

////@end CHistIntervalDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CHistIntervalDlg member variables
    wxTextCtrl* m_intervals;
////@end CHistIntervalDlg member variables

	wxString s_int;
};

#endif
    // _HISTINTERVALDLG_H_
