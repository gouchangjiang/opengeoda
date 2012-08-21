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

#include <set>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "../ShapeOperations/DbfFile.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../OpenGeoDa.h"
#include "../logger.h"
#include "SaveToTableDlg.h"

const int ID_ADD_BUTTON = wxID_HIGHEST+1;
const int ID_FIELD_CHOICE = wxID_HIGHEST+2;
const int ID_TIME_CHOICE = wxID_HIGHEST+3;
const int ID_CHECK = wxID_HIGHEST+4;

BEGIN_EVENT_TABLE( SaveToTableDlg, wxDialog )
	EVT_CHECKBOX( ID_CHECK, SaveToTableDlg::OnCheck )
	EVT_BUTTON( ID_ADD_BUTTON, SaveToTableDlg::OnAddFieldButton )
	EVT_CHOICE( ID_FIELD_CHOICE, SaveToTableDlg::OnFieldChoice )
	EVT_CHOICE( ID_TIME_CHOICE, SaveToTableDlg::OnTimeChoice )
    EVT_BUTTON( XRCID("wxID_OK"), SaveToTableDlg::OnOkClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), SaveToTableDlg::OnCloseClick )
END_EVENT_TABLE()

SaveToTableDlg::SaveToTableDlg(DbfGridTableBase* grid_base_s, wxWindow* parent,
							   const std::vector<SaveToTableEntry>& data_s,
							   const wxString& title, const wxPoint& pos,
							   const wxSize& size, long style)
: wxDialog(parent, wxID_ANY, title, pos, size, style),
data(data_s), grid_base(grid_base_s),
m_check(data_s.size()), m_add_button(data_s.size()),
m_field(data_s.size()), m_time(data_s.size()),
col_id_maps(data_s.size()),
is_space_time(grid_base_s->IsTimeVariant()),
all_init(false)
{
	for (int i=0, iend=data.size(); i<iend; i++) {
		m_check[i] = new wxCheckBox(this, ID_CHECK, data[i].label,
									wxDefaultPosition, wxSize(150, 30));
		m_add_button[i] = new wxButton(this, ID_ADD_BUTTON, "Add Variable");
		m_field[i] = new wxChoice(this, ID_FIELD_CHOICE,
								  wxDefaultPosition, wxSize(180, 20));
		if (is_space_time) {
			m_time[i] = new wxChoice(this, ID_TIME_CHOICE,
									 wxDefaultPosition, wxSize(180, 20));
		} else {
			m_time[i] = 0;
		}
	}
	if (data.size() == 1) m_check[0]->SetValue(1);
	InitTime();
	FillColIdMaps();
	InitFields();
	for (int i=0; i<data.size(); i++) {
		m_field[i]->SetSelection(m_field[i]->FindString(data[i].field_default));
	}
	CreateControls();
	for (int i=0; i<data.size(); i++) EnableTimeField(i);
	SetPosition(pos);
	SetTitle(title);
    Centre();
	all_init = true;
}

void SaveToTableDlg::CreateControls()
{
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

	//wxBoxSizer *space_sizer = new wxBoxSizer(wxHORIZONTAL);
	//space_sizer->AddSpacer(400);
	//top_sizer->Add(space_sizer, 0, wxALL, 1);	
	
	int fg_cols = is_space_time ? 4 : 3;
	// data.size() rows, fg_cols columns, vgap=3, hgap=3
	wxFlexGridSizer *fg_sizer = new wxFlexGridSizer((int) data.size(),
													fg_cols, 3, 3);
	for (int i=0, iend=data.size(); i<iend; i++) {
		fg_sizer->Add(m_check[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
		fg_sizer->Add(m_add_button[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
		fg_sizer->Add(m_field[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
		if (is_space_time) {
			fg_sizer->Add(m_time[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
		}
	}
	top_sizer->Add(fg_sizer, 0, wxALL, 8); // border of 8 around fg_sizer
	
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_ok_button = new wxButton(this, wxID_OK, "OK");
	m_ok_button->Disable();
	button_sizer->Add(m_ok_button, 0, wxALL, 5);
	button_sizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
	top_sizer->Add(button_sizer, 0, wxALL|wxALIGN_CENTER, 5);
	
	SetSizerAndFit(top_sizer);
}

void SaveToTableDlg::OnAddFieldButton( wxCommandEvent& event )
{
	LOG_MSG("Entering SaveToTableDlg::OnAddFieldButton");
	if (!all_init) return;
	wxButton* obj = (wxButton*) event.GetEventObject();
	bool found = false;
	int obj_id = -1;
	for (int i=0; i<m_add_button.size() && !found; i++) {
		if (obj == m_add_button[i]) {
			found = true;
			obj_id = i;
		}
	}
	if (obj_id == -1) {
		wxString msg = "Could not determine which Add Variable button was "
			"pressed. Please report this error.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	LOG_MSG(wxString::Format("Add Variable button# pressed: %d", obj_id));
	
	// remember existing col choices before adding a new column.
	std::vector<wxString> prev_col_nm(data.size());
	for (int i=0; i<data.size(); i++) {
		prev_col_nm[i] = m_field[i]->GetStringSelection();
	}
	
	DataViewerAddColDlg dlg(grid_base, this, false, true,
							data[obj_id].field_default);
	if (dlg.ShowModal() != wxID_OK) return;
	int col = dlg.GetColId();

	GeoDaConst::FieldType type = grid_base->GetColType(col);
	if (type != data[col].type && data[col].type == GeoDaConst::long64_type &&
		type != GeoDaConst::double_type) return;
	// reinitialize all field lists, but set list corresponding to button
	// to newly created field
	FillColIdMaps();
	InitFields();
	
	for (int i=0; i<data.size(); i++) {
		int sel = m_field[i]->FindString(prev_col_nm[i]);
		if (sel != wxNOT_FOUND) m_field[i]->SetSelection(sel);
	}
	
	m_field[obj_id]->SetSelection(m_field[obj_id]->
								  FindString(dlg.GetColName()));

	EnableTimeField(obj_id);
	UpdateOkButton();
	LOG_MSG("Exiting SaveToTableDlg::OnAddFieldButton");
}

void SaveToTableDlg::OnFieldChoice( wxCommandEvent& event )
{
	if (!all_init) return;
	wxChoice* obj = (wxChoice*) event.GetEventObject();
	bool found = false;
	int obj_id = -1;
	for (int i=0; i<m_field.size() && !found; i++) {
		if (obj == m_field[i]) {
			found = true;
			obj_id = i;
		}
	}
	if (obj_id == -1) {
		wxString msg = "Could not determine which Field Choice was "
		"selected. Please report this error.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	EnableTimeField(obj_id);
	UpdateOkButton();
}

void SaveToTableDlg::OnTimeChoice( wxCommandEvent& event )
{
	if (!all_init) return;
	wxChoice* obj = (wxChoice*) event.GetEventObject();
	bool found = false;
	int obj_id = -1;
	for (int i=0; i<m_time.size() && !found; i++) {
		if (obj == m_time[i]) {
			found = true;
			obj_id = i;
		}
	}
	if (obj_id == -1) {
		wxString msg = "Could not determine which Time Choice was "
		"selected. Please report this error.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	UpdateFieldTms(obj_id);
}

void SaveToTableDlg::OnCheck( wxCommandEvent& event )
{
	if (!all_init) return;
	UpdateOkButton();
}

void SaveToTableDlg::UpdateOkButton()
{
	// Ensure that at least one check box is selected
	// and that all checkbox rows have a field selected
	std::vector<int> is_check(data.size());
	for (int i=0, e=data.size(); i<e; i++) {
		is_check[i]=m_check[i]->GetValue()==1;
	}
	bool any_checked = false;
	for (int i=0, e=data.size(); i<e; i++) if (is_check[i]) any_checked = true;
	if (!any_checked) {
		m_ok_button->Disable();
		return;
	}
	for (int i=0, e=data.size(); i<e; i++) {
		if (is_check[i] && wxNOT_FOUND == m_field[i]->GetSelection()) {
			m_ok_button->Disable();
			return;
		}
	}
	m_ok_button->Enable();
}

void SaveToTableDlg::EnableTimeField(int button)
{
	if (!is_space_time) return;
	if (m_field[button]->GetSelection() == wxNOT_FOUND) {
		m_time[button]->Disable();
		return;
	}
	int col = col_id_maps[button][m_field[button]->GetSelection()];
	m_time[button]->Enable(grid_base->IsColTimeVariant(col));
}

void SaveToTableDlg::UpdateFieldTms(int button)
{
	if (!is_space_time) return;
	
	int prev_col = -1;
	if (m_field[button]->GetSelection() != wxNOT_FOUND) {
		prev_col = col_id_maps[button][m_field[button]->GetSelection()];
	}
	
	InitField(button);
	
	if (prev_col != -1) {
		for (int i=0; i<col_id_maps[button].size(); i++) {
			if (prev_col == col_id_maps[button][i]) {
				m_field[button]->SetSelection(i);
			}
		}
	}
}

void SaveToTableDlg::OnOkClick( wxCommandEvent& event )
{
	std::vector<int> is_check(data.size());
	for (int i=0, e=data.size(); i<e; i++) {
		is_check[i]=m_check[i]->GetValue()==1;
	}
	bool any_checked = false;
	for (int i=0, e=data.size(); i<e; i++) if (is_check[i]) any_checked = true;
	if (!any_checked) return;
	
	std::vector<wxString> fname(data.size());
	for (int i=0, iend=data.size(); i<iend; i++) {
		fname[i] = m_field[i]->GetStringSelection();
	}
	
	// Throw all fname[i] into a set container and check for duplicates while
	// adding them in.
	std::set<wxString> names;
	std::set<wxString>::iterator it;
	for (int i=0, iend=fname.size(); i<iend; i++) {
		wxString s = fname[i];
		if (grid_base->IsTimeVariant() && m_time[i]->IsEnabled()) {
			s << " (" << m_time[i]->GetStringSelection() << ")";
		}
		it = names.find(s);
		if (it != names.end()) {
			wxMessageDialog dlg(this, "Duplicate field names specified.",
								"Error", wxOK | wxICON_ERROR );
			dlg.ShowModal();
			return;
		}
		names.insert(s);
	}
	
	for (int i=0, iend=data.size(); i<iend; i++) {
		if (is_check[i]) {
			int col = col_id_maps[i][m_field[i]->GetSelection()];
			DbfColContainer& cd = *grid_base->col_data[col];
			int time = is_space_time ? m_time[i]->GetSelection() : 0;
			if (data[i].d_val) {
				cd.SetFromVec(*data[i].d_val, time);
			} else if (data[i].l_val) {
				cd.SetFromVec(*data[i].l_val, time);
			}
		}
	}
	GeneralWxUtils::EnableMenuItem(MyFrame::theFrame->GetMenuBar(),
								   XRCID("ID_SAVE_PROJECT"),
								   grid_base->ChangedSinceLastSave());
	 
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	event.Skip();
	EndDialog(wxID_OK);	
}

void SaveToTableDlg::OnCloseClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CLOSE);
}

void SaveToTableDlg::InitTime()
{
	if (!is_space_time) return;
	for (int j=0, jend=data.size(); j<jend; j++) {
		for (int t=0; t<grid_base->time_steps; t++) {
			wxString tm;
			tm << grid_base->time_ids[t];
			m_time[j]->Append(tm);
		}
		m_time[j]->SetSelection(0);
		m_time[j]->Disable();
	}	
}

void SaveToTableDlg::FillColIdMaps()
{
	std::vector<int> tmp_col_id_map;
	grid_base->FillColIdMap(tmp_col_id_map);
	for (int i=0; i<col_id_maps.size(); i++) col_id_maps[i].clear();
	
	for (int i=0; i<grid_base->GetNumberCols(); i++) {
		int col = tmp_col_id_map[i];
		DbfColContainer& cd = *grid_base->col_data[col];
		if (cd.type == GeoDaConst::double_type) {
			for (int j=0, jend=data.size(); j<jend; j++) {
				if (data[j].type == GeoDaConst::double_type) {
					col_id_maps[j].push_back(col);
				}
			}
		}
		if (cd.type == GeoDaConst::double_type ||
			cd.type == GeoDaConst::long64_type) {
			for (int j=0, jend=data.size(); j<jend; j++) {
				if (data[j].type == GeoDaConst::long64_type) {
					col_id_maps[j].push_back(col);
				}
			}
		}
	}
}

void SaveToTableDlg::InitField(int button)
{
	// assumes that FillColIdMaps and InitTime has been called previously
	m_field[button]->Clear();
	
	for (int i=0, iend=col_id_maps[button].size(); i<iend; i++) {
		int col = col_id_maps[button][i];
		DbfColContainer& cd = *grid_base->col_data[col];
		//if (grid_base->IsColTimeVariant(col)) {
		//	wxString t;
		//	int t_sel = m_time[button]->GetSelection();
		//	t << " (" << grid_base->time_ids[t_sel] << ")";
		//	m_field[button]->Append(cd.name + t);
		//} else {
			m_field[button]->Append(cd.name);
		//}
	}
}

void SaveToTableDlg::InitFields()
{
	// assumes that FillColIdMaps and InitTime has been called previously
	for (int i=0; i<data.size(); i++) InitField(i);
}
