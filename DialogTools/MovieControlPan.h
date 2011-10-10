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

class CMovieControlPan: public wxPanel
{    
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CMovieControlPan( );
    CMovieControlPan( wxWindow* parent, wxWindowID id = -1,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CMovieControlPan event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON1
    void OnCButton1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON2
    void OnCButton2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON3
    void OnCButton3Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON4
    void OnCButton4Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON5
    void OnCButton5Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
    void OnCheckbox1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLIDER1
    void OnCSlider1Updated( wxCommandEvent& event );

////@end CMovieControlPan event handler declarations

////@begin CMovieControlPan member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CMovieControlPan member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CMovieControlPan member variables
    wxCheckBox* m_reverse;
    wxSlider* m_slider;
    wxStaticText* m_label;
////@end CMovieControlPan member variables

	 MapMovieCanvas *myB;
};

#endif
    // _MOVIECONTROLPAN_H_
