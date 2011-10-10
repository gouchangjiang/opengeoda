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

/////////////////////////////////////////////////////////////////////////////
// Name:        RegressionProgressReportDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/05/04 20:17:37
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _REGRESSIONPROGRESSREPORTDLG_H_
#define _REGRESSIONPROGRESSREPORTDLG_H_

#ifdef __GNUG__
#pragma interface "RegressionProgressReportDlg.cpp"
#endif

#define IDD_REGRESSION_MONITOR 10000
//#define IDC_LIST1 10002

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CRegressionProgressReportDlg class declaration
 */

class CRegressionProgressReportDlg: public wxDialog
{    
    DECLARE_CLASS( CRegressionProgressReportDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CRegressionProgressReportDlg( );
    CRegressionProgressReportDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("PROGRESS REPORT"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("PROGRESS REPORT"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CRegressionProgressReportDlg event handler declarations
////@end CRegressionProgressReportDlg event handler declarations

////@begin CRegressionProgressReportDlg member function declarations
////@end CRegressionProgressReportDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CRegressionProgressReportDlg member variables
////@end CRegressionProgressReportDlg member variables
};

#endif
    // _REGRESSIONPROGRESSREPORTDLG_H_
