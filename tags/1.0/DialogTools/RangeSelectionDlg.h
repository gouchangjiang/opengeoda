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

#ifndef __GEODA_CENTER_RANGE_SELECTION_DLG_H__
#define __GEODA_CENTER_RANGE_SELECTION_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h> 

class Project;
class DbfGridTableBase;

class RangeSelectionDlg: public wxDialog
{    
public:
    RangeSelectionDlg( Project* project, wxWindow* parent,
					  const wxString& title = "Selection Dialog", 
					  const wxPoint& pos = wxDefaultPosition );

    void CreateControls();
	void OnFieldChoice( wxCommandEvent& event );
	void OnRangeTextChange( wxCommandEvent& event );
	void OnSelRangeClick( wxCommandEvent& event );
	void OnSelUndefClick( wxCommandEvent& event );
	void OnInvertSelClick( wxCommandEvent& event );
	void OnSaveFieldCB( wxCommandEvent& event );
	void OnSaveFieldCBtext( wxCommandEvent& event );
	void OnSelCheckBox( wxCommandEvent& event );
	void OnUnselCheckBox( wxCommandEvent& event );
	void OnSelUnselTextChange( wxCommandEvent& event);
	void OnApplySaveClick( wxCommandEvent& event );
	void OnCloseClick( wxCommandEvent& event );
	void Init();

	wxChoice* m_field_choice;
	wxTextCtrl* m_min_text;
	wxStaticText* m_field_static_txt;
	wxStaticText* m_field2_static_txt;
	wxTextCtrl* m_max_text;
	wxButton* m_sel_range_button;
	wxButton* m_sel_undef_button;
	wxButton* m_invert_sel_button;
	wxComboBox* m_save_field_cb;
	wxCheckBox* m_sel_check_box;	
	wxTextCtrl* m_sel_val_text;
	wxCheckBox* m_unsel_check_box;	
	wxTextCtrl* m_unsel_val_text;
	wxButton* m_apply_save_button;
	
	bool m_selection_made; // true once a selection has been made
	bool m_all_init;
	void CheckRangeButtonSettings();
	void CheckApplySaveSettings();
	
private:
	// col_id_map[i] is a map from the i'th item in the fields drop-down
	// to the actual col_id_map.  Items in the fields dropdown are in the
	// order displayed in wxGrid
	std::vector<int> col_id_map;
	DbfGridTableBase* grid_base;
	Project* project;
	// The last mapped col_id for which selection was applied.
	// This value is used for the save results apply funciton.
	int current_sel_mcol;
	
	DECLARE_EVENT_TABLE()
};

#endif
