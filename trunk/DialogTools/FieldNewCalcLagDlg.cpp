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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/GalWeight.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "SelectWeightDlg.h"
#include "../logger.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcLagDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcLagDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_LAG_RESULT"),
			   FieldNewCalcLagDlg::OnLagResultUpdated )
	EVT_CHOICE( XRCID("IDC_LAG_RESULT_TM"),
		   FieldNewCalcLagDlg::OnLagResultTmUpdated )
    EVT_CHOICE( XRCID("IDC_CURRENTUSED_W"),
			   FieldNewCalcLagDlg::OnCurrentusedWUpdated )
    EVT_CHOICE( XRCID("IDC_LAG_OPERAND"),
			   FieldNewCalcLagDlg::OnLagOperandUpdated )
	EVT_CHOICE( XRCID("IDC_LAG_OPERAND_TM"),
			   FieldNewCalcLagDlg::OnLagOperandTmUpdated )
	EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"), FieldNewCalcLagDlg::OnOpenWeightClick )
END_EVENT_TABLE()

FieldNewCalcLagDlg::FieldNewCalcLagDlg(Project* project_s,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption,
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), project(project_s),
grid_base(project_s->GetGridBase()), w_manager(project_s->GetWManager()),
is_space_time(project_s->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
	
	InitFieldChoices();
	m_text->SetValue(wxEmptyString);

	if (w_manager->IsDefaultWeight()) {
		m_weight->SetSelection(w_manager->GetCurrWeightInd());
	}
	all_init = true;
	Display();
}

void FieldNewCalcLagDlg::CreateControls()
{
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_LAG");
    m_result = XRCCTRL(*this, "IDC_LAG_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_LAG_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
    m_weight = XRCCTRL(*this, "IDC_CURRENTUSED_W", wxChoice);
    m_var = XRCCTRL(*this, "IDC_LAG_OPERAND", wxChoice);
	m_var_tm = XRCCTRL(*this, "IDC_LAG_OPERAND_TM", wxChoice);
    InitTime(m_var_tm);
	m_text = XRCCTRL(*this, "IDC_EDIT6", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcLagDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select a Result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_weight->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please specify a Weights matrix.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_var->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Variable field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	int result_col = col_id_map[m_result->GetSelection()];
	int var_col = col_id_map[m_var->GetSelection()];
	
	if (is_space_time &&
		!IsAllTime(result_col, m_result_tm->GetSelection()) &&
		IsAllTime(var_col, m_var_tm->GetSelection())) {
		wxString msg("When \"all times\" selected for variable, result "
					 "field must also be \"all times.\"");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	std::vector<int> time_list;
	if (IsAllTime(result_col, m_result_tm->GetSelection())) {
		time_list.resize(grid_base->time_steps);
		for (int i=0; i<grid_base->time_steps; i++) time_list[i] = i;
	} else {
		int tm = IsTimeVariant(result_col) ? m_result_tm->GetSelection() : 0;
		time_list.resize(1);
		time_list[0] = tm;
	}
	
	std::vector<double> data(grid_base->GetNumberRows(), 0);
	std::vector<bool> undefined(grid_base->GetNumberRows(), false);
	if (!IsAllTime(var_col, m_var_tm->GetSelection())) {
		int tm = IsTimeVariant(var_col) ? m_var_tm->GetSelection() : 0;
		grid_base->col_data[var_col]->GetVec(data, tm);
		grid_base->col_data[var_col]->GetUndefined(undefined, tm);
	}
	
	int rows = grid_base->GetNumberRows();
	std::vector<double> r_data(grid_base->GetNumberRows(), 0);
	std::vector<bool> r_undefined(grid_base->GetNumberRows(), false);
	
	GalWeight* gal_w = w_manager->GetGalWeight(m_weight->GetSelection());
	if (gal_w == NULL) return;
	GalElement* W = gal_w->gal;
	
	for (int t=0; t<time_list.size(); t++) {
		for (int i=0; i<rows; i++) {
			r_data[i] = 0;
			r_undefined[i] = false;
		}
		if (IsAllTime(var_col, m_var_tm->GetSelection())) {
			grid_base->col_data[var_col]->GetVec(data, time_list[t]);
			grid_base->col_data[var_col]->GetUndefined(undefined, time_list[t]);
		}
		// Row-standardized lag calculation.
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
			double lag = 0;
			if (W[i].size == 0) r_undefined[i] = true;
			for (int j=0; j<W[i].size && !r_undefined[i]; j++) {
				if (undefined[W[i].data[j]]) {
					r_undefined[i] = true;
				} else {
					lag += data[W[i].data[j]];
				}
			}
			r_data[i] = r_undefined[i] ? 0 : lag /= W[i].size;
		}
		grid_base->col_data[result_col]->SetFromVec(r_data, time_list[t]);
		grid_base->col_data[result_col]->SetUndefined(r_undefined,
													  time_list[t]);
	}
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}


void FieldNewCalcLagDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	wxString v_str_sel = m_var->GetStringSelection();
	int v_sel = m_var->GetSelection();
	wxString w_str_sel = m_weight->GetStringSelection();
	m_result->Clear();
	m_var->Clear();
	m_weight->Clear();

	grid_base->FillNumericColIdMap(col_id_map);
	
	wxString r_tm, v_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		v_tm << " (" << m_var_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			grid_base->col_data[col_id_map[i]]->time_steps > 1) {			
			m_result->Append(grid_base->col_data[col_id_map[i]]->name + r_tm);
			m_var->Append(grid_base->col_data[col_id_map[i]]->name + v_tm);
		} else {
			m_result->Append(grid_base->col_data[col_id_map[i]]->name);
			m_var->Append(grid_base->col_data[col_id_map[i]]->name);
		}
	}
	
	if (w_manager->GetNumWeights() > 0) {
		for (int i=0; i<w_manager->GetNumWeights(); i++) {
			m_weight->Append(w_manager->GetWFilename(i));
		}
	}
	if (m_result->GetCount() == prev_cnt) {
		m_result->SetSelection(r_sel);
	} else {
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	if (m_var->GetCount() == prev_cnt) {
		m_var->SetSelection(v_sel);
	} else {
		m_var->SetSelection(m_var->FindString(v_str_sel));
	}
	m_weight->SetSelection(m_weight->FindString(w_str_sel));

	Display();
}

void FieldNewCalcLagDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcLagDlg::Display()
{
	wxString s = "";
	wxString pre = "";
	wxString lhs = m_result->GetStringSelection();
	wxString rhs = "";
	wxString w_str = "";
	
	if (m_weight->GetSelection() != wxNOT_FOUND &&
		m_var->GetSelection() != wxNOT_FOUND)
	{
		wxFileName w_fn(m_weight->GetString(m_weight->GetSelection()));
		wxString w_str = w_fn.GetFullName();
		rhs << "W * " << m_var->GetStringSelection();
		pre << w_str << " is W matrix ==> ";
	}
	if (lhs.IsEmpty() && rhs.IsEmpty()) {
		s = "";
	} else if (!lhs.IsEmpty() && rhs.IsEmpty()) {
		s << lhs << " =";
	} else if (lhs.IsEmpty() && !rhs.IsEmpty()) {
		s << pre << rhs;
	} else {
		// a good time to enable the apply button.
		s << pre << lhs << " = " << rhs;
	}
	
	m_text->SetValue(s);
}

bool FieldNewCalcLagDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (grid_base->IsColTimeVariant(col_id));
}

bool FieldNewCalcLagDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!grid_base->IsColTimeVariant(col_id)) return false;
	return tm_sel == grid_base->time_steps;
}

void FieldNewCalcLagDlg::OnLagResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));	
    Display();
}

void FieldNewCalcLagDlg::OnLagResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcLagDlg::OnCurrentusedWUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcLagDlg::OnLagOperandUpdated( wxCommandEvent& event )
{
	int sel = m_var->GetSelection();
	m_var_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));	
    Display();
}

void FieldNewCalcLagDlg::OnLagOperandTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcLagDlg::OnOpenWeightClick( wxCommandEvent& event )
{
	SelectWeightDlg dlg(project, this);
	dlg.ShowModal();
	
	m_weight->Clear();
	for (int i=0; i<w_manager->GetNumWeights(); i++) {
		m_weight->Append(w_manager->GetWFilename(i));
	}
	if (w_manager->GetCurrWeightInd() >=0 ) {
		m_weight->SetSelection(w_manager->GetCurrWeightInd());
	}
	InitFieldChoices(); // Need to call this in case AddId was called.
	Display();
	UpdateOtherPanels();
}

void FieldNewCalcLagDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (grid_base->col_data[dlg.GetColId()]->time_steps > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnLagResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcLagDlg::InitTime(wxChoice* time_list)
{
	time_list->Clear();
	for (int i=0; i<grid_base->time_steps; i++) {
		wxString t;
		t << grid_base->time_ids[i];
		time_list->Append(t);
	}
	time_list->Append("all times");
	time_list->SetSelection(grid_base->time_steps);
	time_list->Disable();
	time_list->Show(is_space_time);
}

