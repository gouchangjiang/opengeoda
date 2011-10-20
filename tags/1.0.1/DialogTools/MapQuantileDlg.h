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

#ifndef __GEODA_CENTER_MAP_QUANTILE_DLG_H__
#define __GEODA_CENTER_MAP_QUANTILE_DLG_H__

#include <wx/spinctrl.h>

class CMapQuantileDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    CMapQuantileDlg( int max_classes, bool dup_val_warning = true,
					wxWindow* parent = NULL, wxWindowID id = -1,
					const wxString& caption = "Quantile Map",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxSYSTEM_MENU );
    void CreateControls();

    void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );

    static bool ShowToolTips();

	bool dup_val_warning;
	int max_classes;
    wxSpinCtrl* m_classes;
};

#endif

