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

#ifndef __GEODA_CENTER_PCP_DLG_H__
#define __GEODA_CENTER_PCP_DLG_H__

#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>

class DbfGridTableBase;

class PCPDlg: public wxDialog
{    
public:
    PCPDlg(DbfGridTableBase* grid_base, wxWindow* parent,
			wxWindowID id = wxID_ANY, 
			const wxString& title = "Parallel Coordinate Plot", 
			const wxPoint& pos = wxDefaultPosition, 
			const wxSize& size = wxDefaultSize,
			long style = wxCAPTION|wxSYSTEM_MENU );

	std::vector<int> pcp_col_ids;

private:
    void CreateControls();
	void Init();
	
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
	
	void OnIncAllClick( wxCommandEvent& ev );
	void OnIncOneClick( wxCommandEvent& ev );
	void OnIncListDClick(wxCommandEvent& ev );
	void OnExclAllClick( wxCommandEvent& ev );
	void OnExclOneClick( wxCommandEvent& ev );
	void OnExclListDClick(wxCommandEvent& ev );
	void UpdateOkButton();
	
	wxListBox* m_exclude_list;
	wxListBox* m_include_list;
	
	std::map<wxString, int> dedup_to_id;
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4.  Numeric fields only.
	std::vector<int> col_id_map;
	
	DbfGridTableBase* grid_base;
	
	DECLARE_EVENT_TABLE();
};

#endif
