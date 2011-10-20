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
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
#include "PCPDlg.h"

BEGIN_EVENT_TABLE( PCPDlg, wxDialog )
    EVT_BUTTON( wxID_OK, PCPDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, PCPDlg::OnCancelClick )

	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), PCPDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), PCPDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"),
					   PCPDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), PCPDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), PCPDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"),
					   PCPDlg::OnExclListDClick )
END_EVENT_TABLE()

PCPDlg::PCPDlg(DbfGridTableBase* grid_base_s, wxWindow* parent,
				 wxWindowID id, const wxString& title, const wxPoint& pos,
				 const wxSize& size, long style )
: grid_base(grid_base_s)
{
    SetParent(parent);
	grid_base->FillNumericColIdMap(col_id_map);
    CreateControls();
	Init();
	SetPosition(pos);
    Centre();
}

void PCPDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_PCP");
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")),
								   wxListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")),
								   wxListBox);	
}

void PCPDlg::Init()
{
	std::map<wxString, int> dbf_fn_freq; // field name frequency
	std::set<wxString> dups;
	dedup_to_id.clear();
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = grid_base->col_data[col_id_map[i]]->name.Upper();
		if (dbf_fn_freq.find(name) != dbf_fn_freq.end()) {
			dbf_fn_freq[name]++;
		} else {
			dbf_fn_freq[name] = 1;
		}
	}
	
	for (std::map<wxString, int>::iterator it=dbf_fn_freq.begin();
		 it!=dbf_fn_freq.end(); it++) {
		if ((*it).second > 1) dups.insert((*it).first);
	}
	
	std::map<wxString, int> dups_cntr;
	for (std::set<wxString>::iterator it=dups.begin(); it!=dups.end(); it++) {
		dups_cntr[(*it)] = 1;
	}
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = grid_base->col_data[col_id_map[i]]->name.Upper();
		wxString dedup_name = name;
		if (dbf_fn_freq[name] > 1) {
			dedup_name << " (" << dups_cntr[name]++ << ")";
		}
		dedup_to_id[dedup_name] = col_id_map[i]; // map to grid_base col id
		m_exclude_list->Append(dedup_name);
	}
	
	UpdateOkButton();
}

void PCPDlg::OnIncAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();
	
	UpdateOkButton();
}

void PCPDlg::OnIncOneClick( wxCommandEvent& ev)
{
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateOkButton();
}

void PCPDlg::OnIncListDClick( wxCommandEvent& ev)
{
	OnExclOneClick(ev);
}

void PCPDlg::OnExclAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateOkButton();
}

void PCPDlg::OnExclOneClick( wxCommandEvent& ev)
{
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
		Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateOkButton();
}

void PCPDlg::OnExclListDClick( wxCommandEvent& ev)
{
	OnIncOneClick(ev);
}

void PCPDlg::OnOkClick( wxCommandEvent& event )
{
	int n_pcp_obs_sel = m_include_list->GetCount();
	
	pcp_col_ids.resize(m_include_list->GetCount());

	// dups tell us which strings need to be renamed, while
	// dedup_to_id tell us which col id this maps to in the original dbf
	// we need to create a final list of names to col id.
	
	std::map<wxString,int> fname_to_id; // field name to col id map
	std::map<int,wxString> inc_order_to_fname; // keep track of order
	
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		pcp_col_ids[i] = dedup_to_id[m_include_list->GetString(i)];
	}
	
	event.Skip();
	EndDialog(wxID_OK);
}

void PCPDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);

}

void PCPDlg::UpdateOkButton()
{
	FindWindow(XRCID("wxID_OK"))->Enable(m_include_list->GetCount() >= 2);
}


