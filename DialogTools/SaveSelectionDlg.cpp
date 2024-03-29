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
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include "../GeoDaConst.h"
#include "../Project.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
#include "SaveSelectionDlg.h"

BEGIN_EVENT_TABLE( SaveSelectionDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_ADD_FIELD"), SaveSelectionDlg::OnAddField )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE_TM"),
			   SaveSelectionDlg::OnSaveFieldChoiceTm )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE"),
				 SaveSelectionDlg::OnSaveFieldChoice )
	EVT_CHECKBOX( XRCID("ID_SEL_CHECK_BOX"), SaveSelectionDlg::OnSelCheckBox )
	EVT_TEXT( XRCID("ID_SEL_VAL_TEXT"),
			 SaveSelectionDlg::OnSelUnselTextChange )
	EVT_CHECKBOX( XRCID("ID_UNSEL_CHECK_BOX"),
				 SaveSelectionDlg::OnUnselCheckBox )
	EVT_TEXT( XRCID("ID_UNSEL_VAL_TEXT"),
			 SaveSelectionDlg::OnSelUnselTextChange )
	EVT_BUTTON( XRCID("ID_APPLY_SAVE_BUTTON"),
			   SaveSelectionDlg::OnApplySaveClick )
	EVT_BUTTON( XRCID("wxID_CANCEL"), SaveSelectionDlg::OnCancelClick )
END_EVENT_TABLE()

SaveSelectionDlg::SaveSelectionDlg(Project* project_s,
								   wxWindow* parent,
								   wxWindowID id,
								   const wxString& caption,
								   const wxPoint& pos,
								   const wxSize& size, long style )
: project(project_s), grid_base(project_s->GetGridBase()),
m_all_init(false), is_space_time(project_s->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
	InitTime();
	FillColIdMap();
	InitField();
	m_all_init = true;
}

void SaveSelectionDlg::CreateControls()
{
	if (grid_base->IsTimeVariant()) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_SAVE_SELECTION_TM");
	} else {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_SAVE_SELECTION");
	}

	m_save_field_choice =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE")), wxChoice);
	m_save_field_choice_tm = 0;
	if (FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM"))) {
		m_save_field_choice_tm =
			wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM")),
											wxChoice);
	}

	m_sel_check_box = wxDynamicCast(FindWindow(XRCID("ID_SEL_CHECK_BOX")),
									wxCheckBox); 
	
	m_sel_val_text = wxDynamicCast(FindWindow(XRCID("ID_SEL_VAL_TEXT")),
								   wxTextCtrl);
	m_sel_val_text->Clear();
	m_sel_val_text->AppendText("1");
	m_sel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_unsel_check_box = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_CHECK_BOX")),
									  wxCheckBox); 
	
	m_unsel_val_text = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_VAL_TEXT")),
									 wxTextCtrl);	
	m_unsel_val_text->Clear();
	m_unsel_val_text->AppendText("0");
	m_unsel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_apply_save_button = wxDynamicCast(
						FindWindow(XRCID("ID_APPLY_SAVE_BUTTON")), wxButton);
	m_apply_save_button->Disable();
}

void SaveSelectionDlg::InitTime()
{
	if (!is_space_time) return;
	for (int t=0; t<grid_base->time_steps; t++) {
		wxString tm;
		tm << grid_base->time_ids[t];
		m_save_field_choice_tm->Append(tm);
	}
	m_save_field_choice_tm->SetSelection(0);
	m_save_field_choice_tm->Disable();
}

void SaveSelectionDlg::FillColIdMap()
{
	col_id_map.clear();
	grid_base->FillNumericColIdMap(col_id_map);
}

void SaveSelectionDlg::InitField()
{
	// assumes that FillColIdMap and InitTime has been called previously
	m_save_field_choice->Clear();
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		DbfColContainer& cd = *grid_base->col_data[col];
		if (grid_base->IsColTimeVariant(col)) {
			wxString t;
			int t_sel = m_save_field_choice_tm->GetSelection();
			t << " (" << grid_base->time_ids[t_sel] << ")";
			m_save_field_choice->Append(cd.name + t);
		} else {
			m_save_field_choice->Append(cd.name);
		}
	}
}


void SaveSelectionDlg::OnAddField( wxCommandEvent& event )
{	
	DataViewerAddColDlg dlg(grid_base, this, false, true, "SELECT");
	if (dlg.ShowModal() != wxID_OK) return;
	int col = dlg.GetColId();
	if (grid_base->GetColType(col) != GeoDaConst::long64_type &&
		grid_base->GetColType(col) != GeoDaConst::double_type) return;

	FillColIdMap();
	InitField();
	
	for (int i=0; i<col_id_map.size(); i++) {
		if (col == col_id_map[i]) {
			m_save_field_choice->SetSelection(i);
		}
	}
	
	EnableTimeField();
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSaveFieldChoice( wxCommandEvent& event )
{
	EnableTimeField();
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSaveFieldChoiceTm( wxCommandEvent& event )
{
	if (!is_space_time) return;
	
	int prev_col = -1;
	if (m_save_field_choice->GetSelection() != wxNOT_FOUND) {
		prev_col = col_id_map[m_save_field_choice->GetSelection()];
	}
	
	InitField();
	
	if (prev_col != -1) {
		for (int i=0; i<col_id_map.size(); i++) {
			if (prev_col == col_id_map[i]) {
				m_save_field_choice->SetSelection(i);
			}
		}
	}
	CheckApplySaveSettings();
}

void SaveSelectionDlg::EnableTimeField()
{
	if (!is_space_time) return;
	if (m_save_field_choice->GetSelection() == wxNOT_FOUND) {
		m_save_field_choice_tm->Disable();
		return;
	}
	int col = col_id_map[m_save_field_choice->GetSelection()];
	m_save_field_choice_tm->Enable(grid_base->IsColTimeVariant(col));
}

void SaveSelectionDlg::OnSelCheckBox( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnUnselCheckBox( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSelUnselTextChange( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void SaveSelectionDlg::CheckApplySaveSettings()
{
	if (!m_all_init) return;
	
	bool target_field_empty = m_save_field_choice->GetSelection()==wxNOT_FOUND;
	
	// Check that m_sel_val_text and m_unsel_val_text is valid.
	// If not valid, set text color to red.
	double val;
	wxString sel_text = m_sel_val_text->GetValue();
	bool sel_valid = sel_text.ToDouble(&val);
	{
		wxTextAttr style(m_sel_val_text->GetDefaultStyle());
		style.SetTextColour(*(sel_valid ? wxBLACK : wxRED));
		m_sel_val_text->SetStyle(0, sel_text.length(), style);
	}
	wxString unsel_text = m_unsel_val_text->GetValue();
	bool unsel_valid = unsel_text.ToDouble(&val);
	{
		wxTextAttr style(m_unsel_val_text->GetDefaultStyle());
		style.SetTextColour(*(unsel_valid ? wxBLACK : wxRED));
		m_unsel_val_text->SetStyle(0, unsel_text.length(), style);
	}
	
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
	
	m_apply_save_button->Enable(!target_field_empty &&
								(sel_checked || unsel_checked) &&
								((sel_checked && sel_valid) || !sel_checked) &&
								((unsel_checked && unsel_valid) ||
								 !unsel_checked));
}

/** The Apply button is only enable when Selected / Unselected values
 are valid (only when checked), and at least one checkbox is
 selected.  The Target Field is not empty, but has not been
 checked for validity. */
void SaveSelectionDlg::OnApplySaveClick( wxCommandEvent& event )
{
	int write_col = col_id_map[m_save_field_choice->GetSelection()];
	
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
	
	double sel_c = 1;
	if (sel_checked) {
		wxString sel_c_str = m_sel_val_text->GetValue();
		sel_c_str.Trim(false); sel_c_str.Trim(true);
		sel_c_str.ToDouble(&sel_c);
	}
	double unsel_c = 0;
	if (unsel_checked) {
		wxString unsel_c_str = m_unsel_val_text->GetValue();
		unsel_c_str.Trim(false); unsel_c_str.Trim(true);
		unsel_c_str.ToDouble(&unsel_c);
	}
	
	int sf_tm = 0;
	if (grid_base->col_data[write_col]->time_steps > 1 &&
		m_save_field_choice_tm) {
		sf_tm = m_save_field_choice_tm->GetSelection();
	}
	
	std::vector<bool>& h = project->highlight_state->GetHighlight();
	// write_col now refers to a valid field in grid base, so write out
	// results to that field.
	int obs = h.size();
	DbfColContainer& cd = *grid_base->col_data[write_col];
	std::vector<bool> undefined;
	if (cd.type == GeoDaConst::long64_type) {
		wxInt64 sel_c_i = sel_c;
		wxInt64 unsel_c_i = unsel_c;
		std::vector<wxInt64> t(grid_base->GetNumberRows());
		cd.GetVec(t, sf_tm);
		cd.GetUndefined(undefined, sf_tm);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c_i;
					undefined[i] = false;
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c_i;
					undefined[i] = false;
				}
			}
		}
		cd.SetFromVec(t, sf_tm);
		cd.SetUndefined(undefined, sf_tm);
	} else if (cd.type == GeoDaConst::double_type) {
		std::vector<double> t(grid_base->GetNumberRows());
		cd.GetVec(t, sf_tm);
		cd.GetUndefined(undefined, sf_tm);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c;
					undefined[i] = false;
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c;
					undefined[i] = false;
				}
			}
		}
		cd.SetFromVec(t, sf_tm);
		cd.SetUndefined(undefined, sf_tm);
	} else {
		wxString msg = "Chosen field is not a numeric type.  This is likely ";
		msg << "a bug. Please report this.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	wxString msg = "Values assigned to target field successfully.";
	wxMessageDialog dlg(this, msg, "Success", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
	OnCancelClick(event);
}

void SaveSelectionDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}
