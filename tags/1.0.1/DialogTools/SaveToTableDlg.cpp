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
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../OpenGeoDa.h"
#include "SaveToTableDlg.h"

BEGIN_EVENT_TABLE( SaveToTableDlg, wxDialog )
    EVT_BUTTON( XRCID("wxID_OK"), SaveToTableDlg::OnOkClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), SaveToTableDlg::OnCloseClick )
END_EVENT_TABLE()

SaveToTableDlg::SaveToTableDlg(DbfGridTableBase* grid_base_s, wxWindow* parent,
							   const std::vector<SaveToTableEntry>& data_s,
							   const wxString& title, const wxPoint& pos,
							   const wxSize& size, long style)
: wxDialog(parent, wxID_ANY, title, pos, size, style),
data(data_s), grid_base(grid_base_s),
m_check(data_s.size()), m_cb(data_s.size())
{
	SetParent(parent);
	for (int i=0, iend=data.size(); i<iend; i++) {
		m_check[i] = new wxCheckBox(this, wxID_ANY, data[i].label,
									wxDefaultPosition, wxSize(150,30));
		m_cb[i] = new wxComboBox(this, wxID_ANY, wxEmptyString,
								 wxDefaultPosition, wxSize(180,30));
	}
	if (data.size() == 1) m_check[0]->SetValue(1);
	Init();
    CreateControls();
	SetTitle(title);
    Centre();
}

void SaveToTableDlg::CreateControls()
{
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

	//wxBoxSizer *space_sizer = new wxBoxSizer(wxHORIZONTAL);
	//space_sizer->AddSpacer(400);
	//top_sizer->Add(space_sizer, 0, wxALL, 1);	
	
	// data.size() rows, 2 columns, vgap=3, hgap=3
	wxFlexGridSizer *fg_sizer = new wxFlexGridSizer((int) data.size(), 2, 3, 3);
	for (int i=0, iend=data.size(); i<iend; i++) {
		fg_sizer->Add(m_check[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
		fg_sizer->Add(m_cb[i], 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);
	}
	top_sizer->Add(fg_sizer, 0, wxALL, 8); // border of 8 around fg_sizer
	
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
	button_sizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
	top_sizer->Add(button_sizer, 0, wxALL|wxALIGN_CENTER, 5);
	
	SetSizerAndFit(top_sizer);
}

void SaveToTableDlg::OnOkClick( wxCommandEvent& event )
{
	wxString bad_name_msg_start = "\"";
	wxString bad_name_msg_end = "\" is an invalid "
	"field name.  A valid field name is between one and ten "
	"characters long.  The first character must be alphabetic,"
	" and the remaining characters can be either alphanumeric "
	"or underscores.";	
	
	std::vector<int> is_check(data.size());
	for (int i=0, e=data.size(); i<e; i++) {
		is_check[i]=m_check[i]->GetValue()==1;
	}
	bool any_checked = false;
	for (int i=0, e=data.size(); i<e; i++) if (is_check[i]) any_checked = true;
	if (!any_checked) return;
	
	std::vector<wxString> fname(data.size());
	for (int i=0, iend=data.size(); i<iend; i++) {
		fname[i] = m_cb[i]->GetValue().Upper();
		fname[i].Trim(true);
		fname[i].Trim(false);
	}
	
	// Throw all fname[i] into a set container and check for duplicates while
	// adding them in.
	std::set<wxString> names;
	std::set<wxString>::iterator it;
	for (int i=0, iend=fname.size(); i<iend; i++) {
		it = names.find(fname[i]);
		if (it != names.end()) {
			wxMessageDialog dlg(this, "Error: duplicate field names specified.",
								"Error", wxOK | wxICON_ERROR );
			dlg.ShowModal();
			return;
		}
		names.insert(fname[i]);
	}
	
	for (int i=0, iend=data.size(); i<iend; i++) {
		// if col already exists, check that it is the correct type
		if (is_check[i] && grid_base->ColNameExists(fname[i])) {
			int mcol = grid_base->FindColId(fname[i]);
			DbfColContainer& cd = *grid_base->col_data[mcol];
			if (data[i].type == GeoDaConst::double_type) {
				if (cd.type != GeoDaConst::double_type) {
					wxMessageDialog dlg(this, "Error: \"" + fname[i] +  
										"\" is not a floating point field.",
										"Error", wxOK | wxICON_ERROR );
					dlg.ShowModal();
					return;
				}
			} else { // assume long64_type
				if (cd.type != GeoDaConst::double_type &&
					cd.type != GeoDaConst::long64_type) {
					wxMessageDialog dlg(this, "Error: \"" + fname[i] +  
										"\" is not a numeric field.",
										"Error", wxOK | wxICON_ERROR );
					dlg.ShowModal();
					return;
				}
			}
		}
		
		// if col doesn't exist, check that it is a valid field name
		if (is_check[i] && !grid_base->ColNameExists(fname[i]) &&
			!DbfFileUtils::isValidFieldName(fname[i])) {
			wxString msg = bad_name_msg_start + fname[i] + bad_name_msg_end;
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
			dlg.ShowModal();
			return;
		}
	}
	
	for (int i=0, iend=data.size(); i<iend; i++) {
		if (is_check[i] && !grid_base->ColNameExists(fname[i])) {
			AddColumn(fname[i], data[i].type);
		}
		if (is_check[i]) {
			int mcol = grid_base->FindColId(fname[i]);
			DbfColContainer& cd = *grid_base->col_data[mcol];
			if (data[i].d_val) {
				cd.SetFromVec(*data[i].d_val);
			} else if (data[i].l_val) {
				cd.SetFromVec(*data[i].l_val);
			}
		}
	}
	GeneralWxUtils::EnableMenuItem(MyFrame::theFrame->GetMenuBar(),
								   XRCID("ID_NEW_TABLE_SAVE"),
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

void SaveToTableDlg::AddColumn(wxString colname, GeoDaConst::FieldType dtype)
{
	wxString name = colname.Upper();
	name.Trim(false);
	name.Trim(true);
	int field_len;
	int decimals = 0;
	int displayed_decimals = 0;
	if (dtype == GeoDaConst::long64_type) {
		field_len = GeoDaConst::default_dbf_long_len;
	} else { // assume dtype == GeoDaConst::double_type
		field_len = GeoDaConst::default_dbf_double_len;
		decimals = GeoDaConst::default_dbf_double_decimals;
		displayed_decimals = decimals;
	}
	grid_base->InsertCol(grid_base->GetNumberCols(), dtype, name, field_len,
						 decimals, displayed_decimals, false, true);
}

void SaveToTableDlg::Init()
{
	for (int i=0, iend=data.size(); i<iend; i++) m_cb[i]->Clear();

	std::vector<int> cb_set_i(data.size(), -1);
	int long_cnt = 0;
	int double_cnt = 0;
	std::vector<int> cb_cnt(data.size(), 0);

	std::vector<int> col_id_map;
	grid_base->FillColIdMap(col_id_map);
	
	for(int i=0; i<grid_base->GetNumberCols(); i++) {
		int mcol = col_id_map[i];
		DbfColContainer& cd = *grid_base->col_data[mcol];
		if (cd.type == GeoDaConst::double_type) {
			for (int j=0, jend=data.size(); j<jend; j++) {
				if (data[j].type == GeoDaConst::double_type) {
					m_cb[j]->Append(cd.name);
					if (cd.name.CmpNoCase(data[j].field_default) == 0) {
						cb_set_i[j] = double_cnt;
					}
				}
			}
			double_cnt++;
		}
		if (cd.type == GeoDaConst::double_type ||
			cd.type == GeoDaConst::long64_type) {
			for (int j=0, jend=data.size(); j<jend; j++) {
				if (data[j].type == GeoDaConst::long64_type) {
					m_cb[j]->Append(cd.name);
					if (cd.name.CmpNoCase(data[j].field_default) == 0) {
						cb_set_i[j] = long_cnt;
					}
				}
			}
			long_cnt++;
		}
	}
	
	for (int i=0, iend=data.size(); i<iend; i++) {
		if (data[i].type == GeoDaConst::double_type) {
			if (cb_set_i[i] == -1) {
				m_cb[i]->Append(data[i].field_default);
				m_cb[i]->SetSelection(double_cnt);
			} else {
				m_cb[i]->SetSelection(cb_set_i[i]);
			}
		} else if (data[i].type == GeoDaConst::long64_type) {
			if (cb_set_i[i] == -1) {
				m_cb[i]->Append(data[i].field_default);
				m_cb[i]->SetSelection(long_cnt);
			} else {
				m_cb[i]->SetSelection(cb_set_i[i]);
			}
		}
	}
}
