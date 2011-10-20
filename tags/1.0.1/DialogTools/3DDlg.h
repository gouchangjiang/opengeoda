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

#ifndef __GEODA_CENTER_3DDLG_H__
#define __GEODA_CENTER_3DDLG_H__

#include <vector>
#include <wx/listbox.h>
#include <wx/dialog.h>

class DbfGridTableBase;

class C3DDlg: public wxDialog
{
    DECLARE_EVENT_TABLE()

	wxListBox *pLBx;
	wxListBox *pLBy;
	wxListBox *pLBz;

public:
    C3DDlg(DbfGridTableBase* grid_base, wxWindow* parent, wxWindowID id = -1,
		   const wxString& caption = "Axis Selection",
		   const wxPoint& pos = wxDefaultPosition,
		   const wxSize& size = wxDefaultSize,
		   long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Axis Selection",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();

    void OnOkClick( wxCommandEvent& event );

    void OnCancelClick( wxCommandEvent& event );

	std::vector<int> col_id_map;
	int x_col_id;
	int y_col_id;
	int z_col_id;
};

#endif
