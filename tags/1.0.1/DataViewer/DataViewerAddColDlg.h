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

#ifndef __GEODA_CENTER_DATA_VIEWER_ADD_COL_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_ADD_COL_DLG_H__

#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <set>
#include <map>
#include "DbfGridTableBase.h"

class DataViewerAddColDlg: public wxDialog
{
public:
    DataViewerAddColDlg(DbfGridTableBase* grid_base,
						wxWindow* parent );
    void CreateControls();
	void OnChoiceType( wxCommandEvent& ev );
	void SetDefaultsByType(GeoDaConst::FieldType type);
    void OnOkClick( wxCommandEvent& ev );
	void OnEditName( wxCommandEvent& ev );
	void OnEditLength( wxCommandEvent& ev );
	void OnEditDecimals( wxCommandEvent& ev );
	void OnEditDisplayedDecimals( wxCommandEvent& ev );
	void UpdateMinMaxValues();
	void UpdateApplyButton();
	
	wxString GetColName();
	
	wxButton* m_apply_button;
	wxTextCtrl* m_name;
	bool m_name_valid;
	wxChoice* m_type;
	wxChoice* m_insert_pos;
	wxStaticText* m_length_lable;
	wxTextCtrl* m_length;
	int m_length_val;
	bool m_length_valid;
	wxStaticText* m_decimals_lable;
	wxTextCtrl* m_decimals;
	int m_decimals_val;
	bool m_decimals_valid;
	wxStaticText* m_displayed_decimals_lable;
	wxTextCtrl* m_displayed_decimals;
	bool m_displayed_decimals_valid;
	wxStaticText* m_max_label;
	wxStaticText* m_max_val;
	wxStaticText* m_min_label;
	wxStaticText* m_min_val;
	GeoDaConst::FieldType cur_type;
	wxGrid* grid;
	DbfGridTableBase* grid_base;
private:
	wxString final_col_name;
	std::set<wxString> curr_col_labels;
	DECLARE_EVENT_TABLE()
};

#endif
