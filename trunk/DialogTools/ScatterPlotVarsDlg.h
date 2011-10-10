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

#ifndef __GEODA_CENTER_SCATTER_PLOT_VARS_DLG_H__
#define __GEODA_CENTER_SCATTER_PLOT_VARS_DLG_H__

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/string.h>

class DbfGridTableBase;

class ScatterPlotVarsDlg: public wxDialog
{
public:
	ScatterPlotVarsDlg(wxString& g_def_var_X,
						wxString& g_def_var_Y,
						wxString& g_def_var_Z,
						DbfGridTableBase* grid_base,
						wxWindow* parent = 0,
						wxWindowID id = -1,
						const wxString& title = "Scatter Plot Variables", 
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						long style = wxCAPTION|wxSYSTEM_MENU,
						const wxString& name = "dialogBox");

	void OnSetVarsAsDefaultClick( wxCommandEvent& event );
    void OnIncludeBubbleSizeClick( wxCommandEvent& event );
	void OnLBsValsChanged( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
	
	void UpdateButtons();

    wxListBox* p_LB_X;
    wxListBox* p_LB_Y;
	wxListBox* p_LB_Z;
    wxCheckBox* p_set_vars_as_default;
    wxCheckBox* p_include_bubble_size;
	
	bool is_set_vars_as_new_defaults;
	wxString var_X;
	wxString var_Y;
	wxString var_Z;
	wxString& global_default_X;
	wxString& global_default_Y;
	wxString& global_default_Z;
	bool is_include_bubble_size;

	DECLARE_EVENT_TABLE()
};

#endif
