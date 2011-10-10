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

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "UserConfigDlg.h"
#endif

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/*!
 * Includes
 */

#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>

#include "UserConfigDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * CUserConfigDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CUserConfigDlg, wxDialog )

/*!
 * CUserConfigDlg event table definition
 */

BEGIN_EVENT_TABLE( CUserConfigDlg, wxDialog )

////@begin CUserConfigDlg event table entries
    EVT_BUTTON( wxID_OK, CUserConfigDlg::OnOkClick )

////@end CUserConfigDlg event table entries

END_EVENT_TABLE()

/*!
 * CUserConfigDlg constructors
 */

CUserConfigDlg::CUserConfigDlg( )
{
}

CUserConfigDlg::CUserConfigDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CUserConfigDlg creator
 */

bool CUserConfigDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CUserConfigDlg member initialisation
    m_label = NULL;
    m_min = NULL;
    m_max = NULL;
////@end CUserConfigDlg member initialisation

////@begin CUserConfigDlg creation
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CUserConfigDlg creation
    return TRUE;
}

/*!
 * Control creation for CUserConfigDlg
 */

void CUserConfigDlg::CreateControls()
{    
////@begin CUserConfigDlg content construction
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_USERCONFIG_DIALOG");
    m_label = XRCCTRL(*this, wxString("wxID_STATIC"), wxStaticText);
    m_min = XRCCTRL(*this, wxString("ID_TEXTCTRL2"), wxTextCtrl);
    m_max = XRCCTRL(*this, wxString("ID_TEXTCTRL3"), wxTextCtrl);
    // Set validators
    if (FindWindow(XRCID("ID_TEXTCTRL2")))
        FindWindow(XRCID("ID_TEXTCTRL2"))->SetValidator( wxTextValidator(wxFILTER_NUMERIC, & s_int) );
    if (FindWindow(XRCID("ID_TEXTCTRL3")))
        FindWindow(XRCID("ID_TEXTCTRL3"))->SetValidator( wxTextValidator(wxFILTER_NUMERIC, & s_int2) );
////@end CUserConfigDlg content construction

    // Create custom windows not generated automatically here.

////@begin CUserConfigDlg content initialisation

////@end CUserConfigDlg content initialisation
}

/*!
 * Should we show tooltips?
 */

bool CUserConfigDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap CUserConfigDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CUserConfigDlg bitmap retrieval
    return wxNullBitmap;
////@end CUserConfigDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CUserConfigDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CUserConfigDlg icon retrieval
    return wxNullIcon;
////@end CUserConfigDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CUserConfigDlg::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in CUserConfigDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
	EndDialog(wxID_OK);
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in CUserConfigDlg. 
}


