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

class CWeightCharacterDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CWeightCharacterDlg( );
    CWeightCharacterDlg( wxWindow* parent, wxWindowID id = -1,
						const wxString& caption = "WEIGHT CHARACTERISTICS",
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "WEIGHT CHARACTERISTICS",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

    void OnCOpenFileweightClick( wxCommandEvent& event );
	void OnOkClick( wxCommandEvent& event );

    static bool ShowToolTips();

    wxTextCtrl* m_name;
	long	m_obs;
	GalElement* m_gal;
	GwtElement* m_gwt;
	wxString	m_WeightFile;
	long	*m_freq;
};

#endif