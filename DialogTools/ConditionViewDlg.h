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

#ifndef __GEODA_CENTER_CONDITION_VIEW_DLG_H__
#define __GEODA_CENTER_CONDITION_VIEW_DLG_H__

#include <wx/dialog.h>

class CConditionViewDlg: public wxDialog
{ 
public:
    CConditionViewDlg( wxWindow* parent, wxWindowID id = -1,
					  const wxString& caption = "Choose View Type",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Choose View Type",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();

    void OnCRadio1Selected( wxCommandEvent& event );
    void OnCRadio2Selected( wxCommandEvent& event );
    void OnCRadio3Selected( wxCommandEvent& event );
    void OnCRadio4Selected( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

#endif
