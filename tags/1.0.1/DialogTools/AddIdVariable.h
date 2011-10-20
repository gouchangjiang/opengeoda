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

#ifndef __GEODA_CENTER_ADD_ID_VARIABLE_H__
#define __GEODA_CENTER_ADD_ID_VARIABLE_H__

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <vector>
#include "../ShapeOperations/DbfFile.h"

class DbfGridTableBase;

class AddIdVariable: public wxDialog {
public:
	AddIdVariable(const wxString& dbf_fnm,
				   wxWindow* parent, wxWindowID id = -1,
				   const wxString& caption = "Add New ID Variable",
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxCAPTION|wxSYSTEM_MENU );
	
	bool Create(const wxString& dbf_fnm,
				wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Add New ID Variable",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );
	
	void CreateControls();
	
	void OnOkClick( wxCommandEvent& event );
	
	void OnCancelClick( wxCommandEvent& event );
	
	static bool ShowToolTips();
	
	wxString GetIdVarName();
	
private:
	wxString new_id_var_name;
	wxTextCtrl *new_id_var;
	wxListBox *existing_vars_list;
	wxString dbf_fname;
	std::vector<DbfFieldDesc> fields;
	
	DECLARE_EVENT_TABLE();
};

#endif

