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
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
#include "SaveSelectionDlg.h"

BEGIN_EVENT_TABLE( SaveSelectionDlg, wxDialog )
	EVT_COMBOBOX( XRCID("ID_SAVE_FIELD_CB"), SaveSelectionDlg::OnSaveFieldCB )
	EVT_TEXT( XRCID("ID_SAVE_FIELD_CB"), SaveSelectionDlg::OnSaveFieldCBtext )
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
m_all_init(false)
{
	SetParent(parent);
    CreateControls();
    Centre();
	Init();
	m_all_init = true;
}

void SaveSelectionDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_SAVE_SELECTION");
	m_save_field_cb = wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CB")),
									wxComboBox);
	
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
}

void SaveSelectionDlg::Init()
{
	m_save_field_cb->Clear();
	grid_base->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		m_save_field_cb->Append(grid_base->col_data[col_id_map[i]]->name);
	}
	m_save_field_cb->ChangeValue("SELECT");
}

void SaveSelectionDlg::OnSaveFieldCB( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSaveFieldCBtext( wxCommandEvent& event )
{
	CheckApplySaveSettings();
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
	
	wxString t = m_save_field_cb->GetValue().Upper();
	t.Trim(false);
	t.Trim(true);
	bool target_field_empty = t.IsEmpty();
	
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
	wxString sf_name = m_save_field_cb->GetValue().Upper();
	sf_name.Trim(false);
	sf_name.Trim(true);
	
	wxString bad_name_msg_start = "\"";
	wxString bad_name_msg_end = "\" is an invalid "
	"field name.  A valid field name is between one and ten "
	"characters long.  The first character must be alphabetic,"
	" and the remaining characters can be either alphanumeric "
	"or underscores.";
	if (!DbfFileUtils::isValidFieldName(sf_name)) {
		wxString msg = bad_name_msg_start + sf_name + bad_name_msg_end;
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	int write_col = wxNOT_FOUND;
	for (int i=0, iend=col_id_map.size();
		 i<iend && write_col == wxNOT_FOUND; i++) {
		if (grid_base->col_data[col_id_map[i]]->name == sf_name) {
			write_col = col_id_map[i];
		}
	}
	
	bool new_col_added = false;
	if (write_col == wxNOT_FOUND) {
		for (int i=0, iend=grid_base->GetNumberCols(); i<iend; i++) {
			if (grid_base->col_data[i]->name == sf_name) {
				wxString msg;
				msg << "\"" << sf_name << "\" already exists in table, but ";
				msg << "is not of numeric type.  Please choose an existing ";
				msg << "numeric field, or chose a new field name.";
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
				dlg.ShowModal();
				return;
			}
		}
		
		// We know that new field name is valid, so create a new field
		// that will accomodate chosen value to write out.  The newly
		// created field is of integral type with length of 7 to
		// allow numbers up to 9 999 999 or as small as -999 999
		write_col = grid_base->GetNumberCols();
		grid_base->InsertCol(write_col, GeoDaConst::long64_type, sf_name,
							 7, 0, 0, false, true);
		new_col_added = true;
	}
	
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
		cd.GetVec(t);
		cd.GetUndefined(undefined);
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
		cd.SetFromVec(t);
		cd.SetUndefined(undefined);
	} else if (cd.type == GeoDaConst::double_type) {
		std::vector<double> t(grid_base->GetNumberRows());
		cd.GetVec(t);
		cd.GetUndefined(undefined);
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
		cd.SetFromVec(t);
		cd.SetUndefined(undefined);
	} else {
		wxString msg = "Chosen field is not a numeric type.  This is likely ";
		msg << "a bug. Please report this.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	// if we just added a new column, reinitialize with the new
	// column as a possible choice.
	if (new_col_added) {
		Init();
		m_save_field_cb->SetSelection(m_save_field_cb->GetCount()-1);
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
