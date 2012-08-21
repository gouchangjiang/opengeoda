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

#ifndef __GEODA_CENTER_REGRESSION_REPORT_DLG_H__
#define __GEODA_CENTER_REGRESSION_REPORT_DLG_H__

#include <wx/xrc/xmlres.h>

////@begin control identifiers
#define IDD_REGRESSION_REPORT 10039
#define SYMBOL_CREGRESSIONREPORTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CREGRESSIONREPORTDLG_TITLE "Regression Report"
#define SYMBOL_CREGRESSIONREPORTDLG_IDNAME IDD_REGRESSION_REPORT
#define SYMBOL_CREGRESSIONREPORTDLG_SIZE wxSize(600, 400)
#define SYMBOL_CREGRESSIONREPORTDLG_POSITION wxDefaultPosition
////@end control identifiers

class RegressionReportDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( RegressionReportDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    RegressionReportDlg( );
    RegressionReportDlg( wxWindow* parent, wxString showText,
        wxWindowID id = SYMBOL_CREGRESSIONREPORTDLG_IDNAME,
        const wxString& caption = SYMBOL_CREGRESSIONREPORTDLG_TITLE,
        const wxPoint& pos = SYMBOL_CREGRESSIONREPORTDLG_POSITION,
        const wxSize& size = SYMBOL_CREGRESSIONREPORTDLG_SIZE,
        long style = SYMBOL_CREGRESSIONREPORTDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent,
        wxWindowID id = SYMBOL_CREGRESSIONREPORTDLG_IDNAME,
        const wxString& caption = SYMBOL_CREGRESSIONREPORTDLG_TITLE,
        const wxPoint& pos = SYMBOL_CREGRESSIONREPORTDLG_POSITION,
        const wxSize& size = SYMBOL_CREGRESSIONREPORTDLG_SIZE,
        long style = SYMBOL_CREGRESSIONREPORTDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin RegressionReportDlg event handler declarations

    /// wxEVT_CLOSE_WINDOW event handler for IDD_REGRESSION_REPORT
    void OnClose(wxCloseEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnFontChanged(wxCommandEvent& event);

////@end RegressionReportDlg event handler declarations

////@begin RegressionReportDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end RegressionReportDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin RegressionReportDlg member variables
    wxTextCtrl* m_textbox;
    wxString results;
////@end RegressionReportDlg member variables
};

#endif
    // _REGRESSIONREPORTDLG_H_
