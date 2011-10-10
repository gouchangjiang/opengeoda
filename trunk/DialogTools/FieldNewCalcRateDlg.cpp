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
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/GalWeight.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "SelectWeightDlg.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcRateDlg, wxPanel )
	EVT_CHOICE( XRCID("IDC_RATE_OPERATOR"),
			   FieldNewCalcRateDlg::OnMethodChange )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"),
			   FieldNewCalcRateDlg::OnAddColumnClick )
	EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"),
			   FieldNewCalcRateDlg::OnOpenWeightClick )
END_EVENT_TABLE()

FieldNewCalcRateDlg::FieldNewCalcRateDlg(Project* project_s,
										 wxWindow* parent,
										 wxWindowID id, const wxString& caption,
										 const wxPoint& pos, const wxSize& size,
										 long style )
: all_init(false), project(project_s),
grid_base(project_s->GetGridBase()), w_manager(project_s->GetWManager())
{
	SetParent(parent);
    CreateControls();
    Centre();

	m_method->Append("Raw Rate");
	m_method->Append("Excess Risk");
	m_method->Append("Empirical Bayes");
	m_method->Append("Spatial Rate");
	m_method->Append("Spatial Empirical Bayes");
	m_method->Append("EB Rate Standardization");
	m_method->SetSelection(0);

	InitFieldChoices();

	if (w_manager->IsDefaultWeight()) {
		m_weight->SetSelection(w_manager->GetCurrWeightInd());
	}
	all_init = true;
}

void FieldNewCalcRateDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_RATE");
    m_result = XRCCTRL(*this, "IDC_RATE_RESULT", wxChoice);
    m_event = XRCCTRL(*this, "IDC_RATE_OPERAND1", wxChoice);
    m_method = XRCCTRL(*this, "IDC_RATE_OPERATOR", wxChoice);
    m_base = XRCCTRL(*this, "IDC_RATE_OPERAND2", wxChoice);
    m_weight = XRCCTRL(*this, "IDC_RATE_WEIGHT", wxChoice);
	m_weight_button = XRCCTRL(*this, "ID_OPEN_WEIGHT", wxBitmapButton);
	m_weight->Enable(false);
	m_weight_button->Enable(false);
}

void FieldNewCalcRateDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select a Result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	const int op = m_method->GetSelection();
	if ((op == 3 || op == 4) && m_weight->GetSelection() == wxNOT_FOUND) {
		wxString msg("Weight matrix required for chosen spatial "
					 "rate method.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_event->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Event field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	if (m_base->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Base field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}	
	
	std::vector<double> data(grid_base->GetNumberRows());
	const int result_col = col_id_map[m_result->GetSelection()];
	const int w = m_weight->GetSelection();
	const int cop1 = col_id_map[m_event->GetSelection()];
	const int cop2 = col_id_map[m_base->GetSelection()];
	
	const int obs = grid_base->GetNumberRows();

	GalElement* W;
	if (op == 3 || op == 4)	{
		if (!w_manager) return;
		if (w_manager->GetNumWeights() < 0) return;
		if (w_manager->IsGalWeight(w)) {
			W = (w_manager->GetGalWeight(w))->gal;
		} else {
			wxString msg("Only weights files internally converted "
						 "to GAL format currently supported.  Please "
						 "report this error.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		if (W == NULL) return;
	}

	const int c_op1 = 0; //m_pIndexDataType[cop1];
	const int c_op2 = 0; // m_pIndexDataType[cop2];

	std::vector<bool> undefined(grid_base->GetNumberRows());
	
	grid_base->col_data[cop1]->GetUndefined(undefined);
	bool Event_undefined = false;
	for (int i=0; i<obs && !Event_undefined; i++) {
		if (undefined[i]) Event_undefined = true;
	}
	if (Event_undefined) {
		wxString msg("Event field has undefined values.  Please define "
					 "missing values or choose a different field. ");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	grid_base->col_data[cop2]->GetUndefined(undefined);
	bool Base_undefined = false;
	for (int i=0; i<obs && !Base_undefined; i++) {
		if (undefined[i]) Base_undefined = true;
	}
	if (!Event_undefined && Base_undefined) {
		wxString msg("Base field has undefined values.  Please define "
					 "missing values or choose a different field. ");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	grid_base->col_data[cop2]->GetVec(data);	
	bool Base_non_positive = false;
	for (int i=0; i<obs && !Base_non_positive; i++) {
		if (data[i] <= 0) Base_non_positive = true;
	}
	
	if (Base_non_positive) {
		wxString msg("Base field has zero or negative values, but all base "
					 "values must be strictly greater than zero. "
					 "Computation aborted. ");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	double* P = new double[obs];
	double* E = new double[obs];
	double* r = new double[obs];
	
	for (int i=0; i<obs; i++) P[i] = data[i];
	
	grid_base->col_data[cop1]->GetVec(data);
	for (int i=0; i<obs; i++) E[i] = data[i];
	
	for (int i=0; i<obs; i++) r[i] = -9999;
	
	bool has_undefined = false;
	std::vector<bool> undef_r;
	switch (op) {
		case 0:
			GeoDaAlgs::RateSmoother_RawRate(obs,P,E,r, undef_r);
			break;
		case 1:
			GeoDaAlgs::RateSmoother_ExcessRisk(obs,P,E,r, undef_r);
			break;
		case 2:
			GeoDaAlgs::RateSmoother_EBS(obs,P,E,r, undef_r);
			break;
		case 3:
			has_undefined = GeoDaAlgs::RateSmoother_SRS(obs,W,P,E,r, undef_r);
			break;
		case 4:
			has_undefined = GeoDaAlgs::RateSmoother_SEBS(obs,W,P,E,r, undef_r);
			break;
		case 5:
			GeoDaAlgs::RateStandardizeEB(obs,P,E,r, undef_r);
			break;
		default:
			break;
	}
	
	for (int i=0; i<obs; i++) data[i] = r[i];
	grid_base->col_data[result_col]->SetFromVec(data);
	grid_base->col_data[result_col]->SetUndefined(undef_r);
	
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	if (P) delete [] P; P = NULL;
	if (E) delete [] E; E = NULL;
	if (r) delete [] r; r = NULL;
	
	if (has_undefined) {
		wxString msg("Some calculated values were undefined and this is "
					 "most likely due to neighborless observations in the "
					 "weight matrix. Rate calculation successful for "
					 "observations with neighbors.");
		wxMessageDialog dlg (this, msg, "Success / Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	} else {
		wxString msg("Rate calculation successful.");
		wxMessageDialog dlg (this, msg, "Success", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
}


void FieldNewCalcRateDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	wxString e_str_sel = m_event->GetStringSelection();
	wxString b_str_sel = m_base->GetStringSelection();
	wxString w_str_sel = m_weight->GetStringSelection();
	m_result->Clear();
	m_event->Clear();
	m_base->Clear();
	m_weight->Clear();

	grid_base->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		m_result->Append(grid_base->col_data[col_id_map[i]]->name);
		m_event->Append(grid_base->col_data[col_id_map[i]]->name);
		m_base->Append(grid_base->col_data[col_id_map[i]]->name);
	}
	
	if (w_manager->GetNumWeights() > 0) {
		for (int i=0; i< w_manager->GetNumWeights(); i++) {
			m_weight->Append(w_manager->GetWFilename(i));
		}
	}
	m_result->SetSelection(m_result->FindString(r_str_sel));
	m_event->SetSelection(m_event->FindString(e_str_sel));
	m_base->SetSelection(m_base->FindString(b_str_sel));
	m_weight->SetSelection(m_weight->FindString(w_str_sel));
}

void FieldNewCalcRateDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	l_panel->InitFieldChoices(); 
}

void FieldNewCalcRateDlg::OnOpenWeightClick( wxCommandEvent& event )
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
	InitFieldChoices(); // call in case AddId was called.
	UpdateOtherPanels();
}

void FieldNewCalcRateDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	m_result->SetSelection(m_result->FindString(dlg.GetColName()));
	UpdateOtherPanels();
}

void FieldNewCalcRateDlg::OnMethodChange( wxCommandEvent& event )
{
	const int op = m_method->GetSelection();
	m_weight->Enable(op == 3 || op == 4);
	m_weight_button->Enable(op == 3 || op == 4);
}
