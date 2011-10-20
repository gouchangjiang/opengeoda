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

#include <wx/xrc/xmlres.h> // XRC XML resouces
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
#include "VariableSettingsDlg.h"

BEGIN_EVENT_TABLE(VariableSettingsDlg, wxDialog)
	EVT_LISTBOX_DCLICK(XRCID("IDC_LIST_VARIABLE1"),
					   VariableSettingsDlg::OnListVariable1DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("IDC_LIST_VARIABLE2"),
					   VariableSettingsDlg::OnListVariable2DoubleClicked)
	EVT_CHECKBOX(XRCID("ID_CHECKBOX"), VariableSettingsDlg::OnCheckboxClick)
	EVT_BUTTON(XRCID("IDOK"), VariableSettingsDlg::OnOkClick)
	EVT_BUTTON(XRCID("IDCANCEL"), VariableSettingsDlg::OnCancelClick)
END_EVENT_TABLE()

VariableSettingsDlg::VariableSettingsDlg( bool IsUnivariate, wxString shapefl,
										 long gObs, wxString v1, wxString v2,
										 double* d1, double* d2,
										 bool m_VarDef,
										 DbfGridTableBase* grid_base_s )
: varFile(shapefl), IsU(IsUnivariate), m_data1(d1), m_data2(d2),
grid_base(grid_base_s), m_Var1(v1), m_Var2(v2), m_CheckDefault(m_VarDef),
m_all_init(false)
{
	pLB1 = NULL;
	pLB2 = NULL;
	pCheck = NULL;
	SetParent(0);
	CreateControls();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	Centre();
	InitFieldChoices();
	m_all_init = true;

	pCheck->SetValue(m_CheckDefault);
}

void VariableSettingsDlg::CreateControls() {
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_VAR_SETTING");
	pLB1 = XRCCTRL(*this, "IDC_LIST_VARIABLE1", wxListBox);
	pLB2 = XRCCTRL(*this, "IDC_LIST_VARIABLE2", wxListBox);
	pCheck = XRCCTRL(*this, "ID_CHECKBOX", wxCheckBox);

	if (IsU) {
		pLB2->Enable(false);
	}
}

void VariableSettingsDlg::OnListVariable1DoubleClicked(wxCommandEvent& event)
{
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable2DoubleClicked(wxCommandEvent& event)
{
	OnOkClick(event);
}

void VariableSettingsDlg::OnCheckboxClick(wxCommandEvent& event) {
	if (!m_all_init) return;
	m_CheckDefault = pCheck->GetValue();
}

void VariableSettingsDlg::OnCancelClick(wxCommandEvent& event)
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void VariableSettingsDlg::OnOkClick(wxCommandEvent& event)
{
	if (pLB1->GetSelection() == wxNOT_FOUND) {
		wxString msg("No field chosen for X.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	m_Var1 = pLB1->GetString(pLB1->GetSelection());
	if (!IsU) {
		if (pLB2->GetSelection() == wxNOT_FOUND) {
			wxString msg("No field chosen for Y.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		m_Var2 = pLB2->GetString(pLB2->GetSelection());
	}

	InitData();
	event.Skip();
	EndDialog(wxID_OK);
}

void VariableSettingsDlg::InitFieldChoices()
{
	LOG_MSG("Entering VariableSettingsDlg::InitFieldChoices");

	pLB1->Clear();
	pLB2->Clear();
	grid_base->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		pLB1->Append(grid_base->col_data[col_id_map[i]]->name);
		pLB2->Append(grid_base->col_data[col_id_map[i]]->name);
	}
	pLB1->SetSelection(pLB1->FindString(m_Var1));
	pLB2->SetSelection(pLB2->FindString(m_Var2));
	
	LOG_MSG("Exiting VariableSettingsDlg::InitFieldChoices");
}


/** This function initializes two global vectors of doubles that correspond to
 the user-selected columns Table. */
void VariableSettingsDlg::InitData()
{
	LOG_MSG("Entering VariableSettingsDlg::InitData");
	std::vector<double> data(grid_base->GetNumberRows(), 0);
	std::vector<bool> undefined(grid_base->GetNumberRows());

	int col1 = col_id_map[pLB1->GetSelection()];
	grid_base->col_data[col1]->GetVec(data);
	grid_base->col_data[col1]->GetUndefined(undefined);
	for (int i=0, iend=data.size(); i<iend; i++) {
		m_data1[i] = undefined[i] ? 0.0 : data[i];
	}
	
	if (!IsU) {
		int col2 = col_id_map[pLB2->GetSelection()];
		grid_base->col_data[col2]->GetVec(data);
		grid_base->col_data[col1]->GetUndefined(undefined);
		for (int i=0, iend=data.size(); i<iend; i++) {
			m_data2[i] = undefined[i] ? 0.0 : data[i];
		}
	}
	LOG_MSG("Exiting VariableSettingsDlg::InitData");
}

