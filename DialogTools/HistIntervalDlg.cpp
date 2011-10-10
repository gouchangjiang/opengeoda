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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

////@begin includes
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>
////@end includes

#include "HistIntervalDlg.h"

/*!
 * CHistIntervalDlg type definition
 */

IMPLEMENT_CLASS( CHistIntervalDlg, wxDialog )

/*!
 * CHistIntervalDlg event table definition
 */

BEGIN_EVENT_TABLE( CHistIntervalDlg, wxDialog )
    EVT_BUTTON( wxID_OK, CHistIntervalDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, CHistIntervalDlg::OnCancelClick )
END_EVENT_TABLE()

/*!
 * CHistIntervalDlg constructors
 */

CHistIntervalDlg::CHistIntervalDlg( )
{

}

CHistIntervalDlg::CHistIntervalDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
	s_int = "7";
}

/*!
 * CHistIntervalDlg creator
 */

bool CHistIntervalDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}

/*!
 * Control creation for CHistIntervalDlg
 */

void CHistIntervalDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_INTERVALS");
    if (FindWindow(wxXmlResource::GetXRCID("IDC_EDIT_INTERVAL")))
        m_intervals = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_INTERVAL")), wxTextCtrl);

    // Set validators
    if (FindWindow(XRCID("IDC_EDIT_INTERVAL")))
        FindWindow(XRCID("IDC_EDIT_INTERVAL"))->SetValidator( wxTextValidator(wxFILTER_NUMERIC, & s_int) );
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CHistIntervalDlg::OnOkClick( wxCommandEvent& event )
{
    event.Skip();
	EndDialog(wxID_OK);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void CHistIntervalDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
	EndDialog(wxID_CANCEL);

}

/*!
 * Should we show tooltips?
 */

bool CHistIntervalDlg::ShowToolTips()
{
    return true;
}
