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

#ifndef __GEODA_CENTER_WEIGHT_CHARACTER_DLG_H__
#define __GEODA_CENTER_WEIGHT_CHARACTER_DLG_H__

class DbfGridTableBase;

class WeightCharacterDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    WeightCharacterDlg( wxWindow* parent,
						DbfGridTableBase* grid_base,
						wxWindowID id = -1,
						const wxString& caption = "Weights Neighbors Histogram",
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Weights Neighbors Histogram",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

    void OnCOpenFileweightClick( wxCommandEvent& event );
	void OnOkClick( wxCommandEvent& event );

	DbfGridTableBase* grid_base;
    wxTextCtrl* m_name;
	wxString m_WeightFile;
	long* m_freq;
};

#endif