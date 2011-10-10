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

#ifndef __GEODA_CENTER_3D_CONTROL_PAN_H__
#define __GEODA_CENTER_3D_CONTROL_PAN_H__

#ifdef __GNUG__
#pragma interface "3DControlPan.cpp"
#endif


/*!
 * C3DControlPan class declaration
 */
class C3DPlotFrame;

class C3DControlPan: public wxPanel
{    
    DECLARE_CLASS( C3DControlPan )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    C3DControlPan( );
    C3DControlPan( wxWindow* parent, wxWindowID id = -1,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = wxCAPTION|wxSYSTEM_MENU,
				  const wxString& x3d_l = "X",
				  const wxString& y3d_l = "Y",
				  const wxString& z3d_l = "Z" );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU,
				const wxString& x3d_l = "X",
				const wxString& y3d_l = "Y",
				const wxString& z3d_l = "Z" );

    /// Creates the controls and sizers
    void CreateControls();

////@begin C3DControlPan event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_DATAPOINT
    void OnCDatapointClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOX
    void OnCToxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOY
    void OnCToyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOZ
    void OnCTozClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_SELECT
    void OnCSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLXP
    void OnCSlxpUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLXP
    void OnCSlxpScroll( wxScrollEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLXS
    void OnCSlxsUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLXS
    void OnCSlxsScroll( wxScrollEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLYP
    void OnCSlypUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLYP
    void OnCSlypScroll( wxScrollEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLYS
    void OnCSlysUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLYS
    void OnCSlysScroll( wxScrollEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLZP
    void OnCSlzpUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLZP
    void OnCSlzpScroll( wxScrollEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLZS
    void OnCSlzsUpdated( wxCommandEvent& event );

    /// wxEVT_SCROLL event handler for IDC_SLZS
    void OnCSlzsScroll( wxScrollEvent& event );

////@end C3DControlPan event handler declarations

////@begin C3DControlPan member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end C3DControlPan member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin C3DControlPan member variables
    wxCheckBox* m_data;
    wxCheckBox* m_prox;
    wxCheckBox* m_proy;
    wxCheckBox* m_proz;
    wxCheckBox* m_select;
	wxStaticText* m_static_text_x;
	wxStaticText* m_static_text_y;
	wxStaticText* m_static_text_z;
	wxString x3d_label;
	wxString y3d_label;
	wxString z3d_label;
    wxSlider* m_xp;
    wxSlider* m_xs;
    wxSlider* m_yp;
    wxSlider* m_ys;
    wxSlider* m_zp;
    wxSlider* m_zs;
////@end C3DControlPan member variables
	C3DPlotFrame* pa;
};

#endif
    // _3DCONTROLPAN_H_
