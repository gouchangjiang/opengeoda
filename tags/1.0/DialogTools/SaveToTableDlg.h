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

#ifndef __GEODA_CENTER_SAVE_TO_TALBE_DLG_H__
#define __GEODA_CENTER_SAVE_TO_TALBE_DLG_H__

#include <vector>
#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include "../GeoDaConst.h"

class DbfGridTableBase;

struct SaveToTableEntry
{
	SaveToTableEntry(): d_val(0), l_val(0) {}
	std::vector<double>* d_val;
	std::vector<wxInt64>* l_val;
	wxString label;
	wxString field_default;
	// if type is double, then only show or allow double fields.  If type
	// is long64 then show both fields, and create a long64 field if a new
	// field is specified.
	GeoDaConst::FieldType type;
};

class SaveToTableDlg: public wxDialog
{    
public:
    SaveToTableDlg( DbfGridTableBase* grid_base, wxWindow* parent,
				   const std::vector<SaveToTableEntry>& data,
				   const wxString& title = "Save Results", 
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    void CreateControls();
    void OnOkClick( wxCommandEvent& event );
	void OnCloseClick( wxCommandEvent& event );
	void Init();

	std::vector<wxCheckBox*> m_check;
	std::vector<wxComboBox*> m_cb;
	const std::vector<SaveToTableEntry>& data;
		
	void AddColumn(wxString colname, GeoDaConst::FieldType dtype);

	DbfGridTableBase* grid_base;
	
	DECLARE_EVENT_TABLE()
};

#endif
