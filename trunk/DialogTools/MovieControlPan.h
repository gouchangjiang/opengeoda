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

#ifndef __GEODA_CENTER_MOVIE_CONTROL_PAN_H__
#define __GEODA_CENTER_MOVIE_CONTROL_PAN_H__

#include <wx/slider.h>
#include <wx/checkbox.h>

class MapMovieFrmae;
class MapMovieCanvas;

class MovieControlPan: public wxPanel
{    
    DECLARE_EVENT_TABLE()

public:
    MovieControlPan( wxWindow* parent,
					 int num_obs_s, wxWindowID id = -1,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();

    void OnCButton1Click( wxCommandEvent& event );
    void OnCButton2Click( wxCommandEvent& event );
    void OnCButton3Click( wxCommandEvent& event );
    void OnCButton4Click( wxCommandEvent& event );
    void OnCButton5Click( wxCommandEvent& event );
    void OnCheckbox1Click( wxCommandEvent& event );
    void OnCSlider1Updated( wxCommandEvent& event );

	int num_obs;
    wxCheckBox* m_reverse;
    wxSlider* m_slider;
    wxStaticText* m_label;

	MapMovieCanvas *myB;
};

#endif
