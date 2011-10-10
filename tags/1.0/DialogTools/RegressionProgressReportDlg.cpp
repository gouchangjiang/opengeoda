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
// Name:        RegressionProgressReportDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/05/04 20:17:37
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "RegressionProgressReportDlg.h"
#endif

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
////@end includes

#include "RegressionProgressReportDlg.h"

////@begin XPM images

////@end XPM images

/*!
 * CRegressionProgressReportDlg type definition
 */

IMPLEMENT_CLASS( CRegressionProgressReportDlg, wxDialog )

/*!
 * CRegressionProgressReportDlg event table definition
 */

BEGIN_EVENT_TABLE( CRegressionProgressReportDlg, wxDialog )

////@begin CRegressionProgressReportDlg event table entries
////@end CRegressionProgressReportDlg event table entries

END_EVENT_TABLE()

/*!
 * CRegressionProgressReportDlg constructors
 */

CRegressionProgressReportDlg::CRegressionProgressReportDlg( )
{
}

CRegressionProgressReportDlg::CRegressionProgressReportDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CRegressionProgressReportDlg creator
 */

bool CRegressionProgressReportDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CRegressionProgressReportDlg member initialisation
////@end CRegressionProgressReportDlg member initialisation

////@begin CRegressionProgressReportDlg creation
    SetParent(parent);
    CreateControls();
    Centre();
////@end CRegressionProgressReportDlg creation
    return true;
}

/*!
 * Control creation for CRegressionProgressReportDlg
 */

void CRegressionProgressReportDlg::CreateControls()
{    
////@begin CRegressionProgressReportDlg content construction

    wxXmlResource::Get()->LoadDialog(this, GetParent(), _T("IDD_REGRESSION_MONITOR"));
////@end CRegressionProgressReportDlg content construction

    // Create custom windows not generated automatically here.

////@begin CRegressionProgressReportDlg content initialisation

////@end CRegressionProgressReportDlg content initialisation
}

/*!
 * Should we show tooltips?
 */

bool CRegressionProgressReportDlg::ShowToolTips()
{
    return true;
}

