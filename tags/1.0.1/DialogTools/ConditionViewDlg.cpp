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

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include "../OpenGeoDa.h"
#include "ConditionViewDlg.h"

BEGIN_EVENT_TABLE( CConditionViewDlg, wxDialog )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO1"), CConditionViewDlg::OnCRadio1Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO2"), CConditionViewDlg::OnCRadio2Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO3"), CConditionViewDlg::OnCRadio3Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO4"), CConditionViewDlg::OnCRadio4Selected )
    EVT_BUTTON( wxID_OK, CConditionViewDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, CConditionViewDlg::OnCancelClick )
END_EVENT_TABLE()

CConditionViewDlg::CConditionViewDlg( wxWindow* parent,
									 wxWindowID id,
									 const wxString& caption,
									 const wxPoint& pos,
									 const wxSize& size,
									 long style )
{
    Create(parent, id, caption, pos, size, style);

	Conditionable::cViewType =	1;
}


bool CConditionViewDlg::Create( wxWindow* parent,
							   wxWindowID id,
							   const wxString& caption,
							   const wxPoint& pos,
							   const wxSize& size,
							   long style )
{
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}

void CConditionViewDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_CONDITION_VIEW_SETTING");
}

void CConditionViewDlg::OnOkClick( wxCommandEvent& event )
{
	if(Conditionable::cViewType == 0) {
		wxMessageBox("Select a view type!");
		return;
	}
	event.Skip(); // wxDialog::OnOK(event);
	EndDialog(wxID_OK);
}

void CConditionViewDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}

void CConditionViewDlg::OnCRadio1Selected( wxCommandEvent& event )
{
    Conditionable::cViewType = 1;	
}

void CConditionViewDlg::OnCRadio2Selected( wxCommandEvent& event )
{
    Conditionable::cViewType = 2;	
}

void CConditionViewDlg::OnCRadio3Selected( wxCommandEvent& event )
{
    Conditionable::cViewType = 3;	
}

void CConditionViewDlg::OnCRadio4Selected( wxCommandEvent& event )
{
    Conditionable::cViewType = 4;	
}
