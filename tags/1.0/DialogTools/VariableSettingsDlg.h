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

#ifndef __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___
#define __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___

#include <vector>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

class DbfGridTableBase;

class VariableSettingsDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
	VariableSettingsDlg( bool IsUnivariate, wxString shapefl, long gObs, 
						 wxString v1, wxString v2, double* d1, double* d2,
						 bool m_VarDef, DbfGridTableBase* grid_base);
    void CreateControls();

    void OnListVariable1DoubleClicked( wxCommandEvent& event );
    void OnListVariable2DoubleClicked( wxCommandEvent& event );
    void OnCheckboxClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    wxListBox* pLB1;
    wxListBox* pLB2;
    wxCheckBox* pCheck;

	bool m_all_init;
    bool m_CheckDefault;
	wxString m_Var1;
	wxString m_Var2;
	double*	m_data1;
	double* m_data2;
	wxString varFile;
	bool IsU;

	DbfGridTableBase* grid_base;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed in wxGrid
	std::vector<int> col_id_map;	

protected:
	void InitFieldChoices();
	void InitData();
};

#endif
