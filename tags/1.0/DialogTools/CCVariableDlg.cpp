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
#include "../DataViewer/DbfGridTableBase.h"
#include "../OpenGeoDa.h"
#include "CCVariableDlg.h"

BEGIN_EVENT_TABLE( CCVariableDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_1"), CCVariableDlg::OnCMoveout1Click )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_2"), CCVariableDlg::OnCMoveout2Click )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_3"), CCVariableDlg::OnCMoveout3Click )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_4"), CCVariableDlg::OnCMoveout4Click )
    EVT_BUTTON( wxID_OK, CCVariableDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, CCVariableDlg::OnCancelClick )
END_EVENT_TABLE()

CCVariableDlg::CCVariableDlg(DbfGridTableBase* grid_base, wxWindow* parent,
							 wxWindowID id, const wxString& caption,
							 const wxPoint& pos, const wxSize& size,
							 long style )
{
    Create(parent, id, caption, pos, size, style);
	if(Conditionable::cViewType != 4) {
		FindWindow(XRCID("IDC_MOVEOUT_4"))->Enable(false);
		FindWindow(XRCID("IDC_EDIT4"))->Enable(false);
		FindWindow(XRCID("IDC_STATIC4"))->Enable(false);
	}

	m_list->Clear();
	std::vector<int> col_id_map;
	grid_base->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		m_list->Append(grid_base->col_data[col_id_map[i]]->name);
	}		
}

bool CCVariableDlg::Create( wxWindow* parent, wxWindowID id,
							const wxString& caption, const wxPoint& pos,
							const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    return true;
}

void CCVariableDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_CC_VARIABLES");
    m_list = XRCCTRL(*this, "IDC_LIST_VARIN", wxListBox);
    m_x = XRCCTRL(*this, "IDC_EDIT1", wxTextCtrl);
	m_x->SetMaxLength(0);
    m_y = XRCCTRL(*this, "IDC_EDIT2", wxTextCtrl);
	m_y->SetMaxLength(0);
    m_v1 = XRCCTRL(*this, "IDC_EDIT3", wxTextCtrl);
	m_v1->SetMaxLength(0);
    m_v2 = XRCCTRL(*this, "IDC_EDIT4", wxTextCtrl);
	m_v2->SetMaxLength(0);
}

void CCVariableDlg::OnOkClick( wxCommandEvent& event )
{
	cc1 = m_x->GetValue();
	cc2 = m_y->GetValue();
	cc3 = m_v1->GetValue();
	cc4 = (Conditionable::cViewType == 4) ? m_v2->GetValue() : "_dummy_name_";
	
	if( cc1 == wxEmptyString || cc2 == wxEmptyString ||
	   cc3 == wxEmptyString || cc4 == wxEmptyString )
	{
		wxMessageBox("Please select variables.");
		return;
	}

	event.Skip();
	EndDialog(wxID_OK);
}

void CCVariableDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}

void CCVariableDlg::OnCMoveout1Click( wxCommandEvent& event )
{
	if(m_list->GetSelection() >= 0)
	    m_x->SetValue(m_list->GetString(m_list->GetSelection()));
}

void CCVariableDlg::OnCMoveout2Click( wxCommandEvent& event )
{
	if(m_list->GetSelection() >= 0)
	    m_y->SetValue(m_list->GetString(m_list->GetSelection()));
}

void CCVariableDlg::OnCMoveout3Click( wxCommandEvent& event )
{
	if(m_list->GetSelection() >= 0)
	    m_v1->SetValue(m_list->GetString(m_list->GetSelection()));
}

void CCVariableDlg::OnCMoveout4Click( wxCommandEvent& event )
{
	if(m_list->GetSelection() >= 0)
	    m_v2->SetValue(m_list->GetString(m_list->GetSelection()));
}
