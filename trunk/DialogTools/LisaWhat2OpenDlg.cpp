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
////@end includes

#include "LisaWhat2OpenDlg.h"

IMPLEMENT_CLASS( CLisaWhat2OpenDlg, wxDialog )

BEGIN_EVENT_TABLE( CLisaWhat2OpenDlg, wxDialog )

////@begin CLisaWhat2OpenDlg event table entries
    EVT_BUTTON( wxID_OK, CLisaWhat2OpenDlg::OnOkClick )

////@end CLisaWhat2OpenDlg event table entries

END_EVENT_TABLE()

/*!
 * CLisaWhat2OpenDlg constructors
 */

CLisaWhat2OpenDlg::CLisaWhat2OpenDlg( )
{
}

CLisaWhat2OpenDlg::CLisaWhat2OpenDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CLisaWhat2OpenDlg creator
 */

bool CLisaWhat2OpenDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}

/*!
 * Control creation for CLisaWhat2OpenDlg
 */

void CLisaWhat2OpenDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_LISAWINDOWS2OPEN");
    if (FindWindow(wxXmlResource::GetXRCID("IDC_CHECK1")))
        m_check1 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK1")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK2")))
        m_check2 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK2")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK3")))
        m_check3 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK3")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK4")))
        m_check4 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK4")), wxCheckBox);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDOK
 */

void CLisaWhat2OpenDlg::OnOkClick( wxCommandEvent& event )
{
    // Insert custom code here

	m_SigMap = m_check1->GetValue();
	m_ClustMap = m_check2->GetValue();
	m_BoxPlot = m_check3->GetValue();
	m_Moran = m_check4->GetValue();

	event.Skip(); // wxDialog::OnOK(event);
	EndDialog(wxID_OK);	
}


/*!
 * Should we show tooltips?
 */

bool CLisaWhat2OpenDlg::ShowToolTips()
{
    return true;
}
