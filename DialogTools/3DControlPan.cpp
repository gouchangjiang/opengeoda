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

#ifdef __GNUG__
#pragma implementation "3DControlPan.h"
#endif

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/splitter.h>
#include <wx/sizer.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../GeneralWxUtils.h"

#include "../Explore/3DPlotView.h"
#include "3DControlPan.h"

/*!
 * C3DControlPan type definition
 */

IMPLEMENT_CLASS( C3DControlPan, wxPanel )

/*!
 * C3DControlPan event table definition
 */

BEGIN_EVENT_TABLE( C3DControlPan, wxPanel )

////@begin C3DControlPan event table entries
    EVT_CHECKBOX( XRCID("IDC_DATAPOINT"), C3DControlPan::OnCDatapointClick )

    EVT_CHECKBOX( XRCID("IDC_TOX"), C3DControlPan::OnCToxClick )

    EVT_CHECKBOX( XRCID("IDC_TOY"), C3DControlPan::OnCToyClick )

    EVT_CHECKBOX( XRCID("IDC_TOZ"), C3DControlPan::OnCTozClick )

    EVT_CHECKBOX( XRCID("IDC_SELECT"), C3DControlPan::OnCSelectClick )

    EVT_SLIDER( XRCID("IDC_SLXP"), C3DControlPan::OnCSlxpUpdated )
	EVT_COMMAND_SCROLL( XRCID("IDC_SLXP"), C3DControlPan::OnCSlxpScroll )

    EVT_SLIDER( XRCID("IDC_SLXS"), C3DControlPan::OnCSlxsUpdated )
	EVT_COMMAND_SCROLL( XRCID("IDC_SLXS"), C3DControlPan::OnCSlxsScroll )

    EVT_SLIDER( XRCID("IDC_SLYP"), C3DControlPan::OnCSlypUpdated )
    EVT_COMMAND_SCROLL( XRCID("IDC_SLYP"), C3DControlPan::OnCSlypScroll )

    EVT_SLIDER( XRCID("IDC_SLYS"), C3DControlPan::OnCSlysUpdated )
    EVT_COMMAND_SCROLL( XRCID("IDC_SLYS"), C3DControlPan::OnCSlysScroll )

    EVT_SLIDER( XRCID("IDC_SLZP"), C3DControlPan::OnCSlzpUpdated )
    EVT_COMMAND_SCROLL( XRCID("IDC_SLZP"), C3DControlPan::OnCSlzpScroll )

    EVT_SLIDER( XRCID("IDC_SLZS"), C3DControlPan::OnCSlzsUpdated )
    EVT_COMMAND_SCROLL( XRCID("IDC_SLZS"), C3DControlPan::OnCSlzsScroll )

////@end C3DControlPan event table entries

END_EVENT_TABLE()

/*!
 * C3DControlPan constructors
 */

C3DControlPan::C3DControlPan( )
{
}

C3DControlPan::C3DControlPan( wxWindow* parent,
							 wxWindowID id,
							 const wxPoint& pos,
							 const wxSize& size,
							 long style,
							 const wxString& x3d_l,
							 const wxString& y3d_l,
							 const wxString& z3d_l )
{
	Create(parent, id, pos, size, style, x3d_l, y3d_l, z3d_l);

	m_xp->SetRange(1,20000);
	m_xp->SetValue(10000);	
	m_xs->SetRange(1,10000);
	m_xs->SetValue(1000);	

	m_yp->SetRange(1,20000);
	m_yp->SetValue(10000);	
	m_ys->SetRange(1,10000);
	m_ys->SetValue(1000);	

	m_zp->SetRange(1,20000);
	m_zp->SetValue(10000);	
	m_zs->SetRange(1,10000);
	m_zs->SetValue(1000);	

}

/*!
 * C3DControlPan creator
 */

bool C3DControlPan::Create( wxWindow* parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxString& x3d_l,
						   const wxString& y3d_l,
						   const wxString& z3d_l )
{
////@begin C3DControlPan member initialisation
    m_data = NULL;
    m_prox = NULL;
    m_proy = NULL;
    m_proz = NULL;
    m_select = NULL;
	m_static_text_x = NULL;
	m_static_text_y = NULL;
	m_static_text_z = NULL;
	x3d_label = x3d_l;
	y3d_label = y3d_l;
	z3d_label = z3d_l;
    m_xp = NULL;
    m_xs = NULL;
    m_yp = NULL;
    m_ys = NULL;
    m_zp = NULL;
    m_zs = NULL;
////@end C3DControlPan member initialisation

////@begin C3DControlPan creation
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end C3DControlPan creation
    return true;
}

/*!
 * Control creation for C3DControlPan
 */

void C3DControlPan::CreateControls()
{    
////@begin C3DControlPan content construction
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_3DCONTROL");
    m_data = XRCCTRL(*this, "IDC_DATAPOINT", wxCheckBox);
    m_prox = XRCCTRL(*this, "IDC_TOX", wxCheckBox);
    m_proy = XRCCTRL(*this, "IDC_TOY", wxCheckBox);
    m_proz = XRCCTRL(*this, "IDC_TOZ", wxCheckBox);
    m_select = XRCCTRL(*this, "IDC_SELECT", wxCheckBox);
	if ( GeneralWxUtils::isMac() ) {
		// change "CTRL" in label to "CMD" in label.
		m_select->SetLabel("Select, hold CMD for brushing");
	}
	m_static_text_x = XRCCTRL(*this, "ID_3D_STATICTEXT_X", wxStaticText);
	m_static_text_x->SetLabel("X (" + x3d_label + ")");
	m_static_text_y = XRCCTRL(*this, "ID_3D_STATICTEXT_Y", wxStaticText);
	m_static_text_y->SetLabel("Y (" + y3d_label + ")");
	m_static_text_z = XRCCTRL(*this, "ID_3D_STATICTEXT_Z", wxStaticText);
	m_static_text_z->SetLabel("Z (" + z3d_label + ")");
    m_xp = XRCCTRL(*this, "IDC_SLXP", wxSlider);
    m_xs = XRCCTRL(*this, "IDC_SLXS", wxSlider);
    m_yp = XRCCTRL(*this, "IDC_SLYP", wxSlider);
    m_ys = XRCCTRL(*this, "IDC_SLYS", wxSlider);
    m_zp = XRCCTRL(*this, "IDC_SLZP", wxSlider);
    m_zs = XRCCTRL(*this, "IDC_SLZS", wxSlider);
////@end C3DControlPan content construction

    // Create custom windows not generated automatically here.

////@begin C3DControlPan content initialisation

////@end C3DControlPan content initialisation
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_DATAPOINT
 */

void C3DControlPan::OnCDatapointClick( wxCommandEvent& event )
{
    // Insert custom code here
	pa->canvas->m_d = m_data->GetValue();
	pa->canvas->Refresh();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOX
 */

void C3DControlPan::OnCToxClick( wxCommandEvent& event )
{
    // Insert custom code here
	pa->canvas->m_x = m_prox->GetValue();
	pa->canvas->Refresh();

}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOY
 */

void C3DControlPan::OnCToyClick( wxCommandEvent& event )
{
    // Insert custom code here
	pa->canvas->m_y = m_proy->GetValue();
	pa->canvas->Refresh();

}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_TOZ
 */

void C3DControlPan::OnCTozClick( wxCommandEvent& event )
{
    // Insert custom code here
	pa->canvas->m_z = m_proz->GetValue();
	pa->canvas->Refresh();

}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for IDC_SELECT
 */

void C3DControlPan::OnCSelectClick( wxCommandEvent& event )
{
    // Insert custom code here
	pa->canvas->b_select = m_select->GetValue();
	pa->canvas->Refresh();

}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLXP
 */

void C3DControlPan::OnCSlxpUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->xp = ((double) m_xp->GetValue())/10000.0 - 1.0;

	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    

}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLXS
 */

void C3DControlPan::OnCSlxsUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->xs = ((double) m_xs->GetValue())/10000.0;

	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    
     
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLYS
 */

void C3DControlPan::OnCSlysUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->ys = ((double) m_ys->GetValue())/10000.0;

	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    
     
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLYP
 */

void C3DControlPan::OnCSlypUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->yp = ((double) m_yp->GetValue())/10000.0 - 1.0;
    
	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    
	 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLZP
 */

void C3DControlPan::OnCSlzpUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->zp = ((double) m_zp->GetValue())/10000.0 - 1.0;

	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    
     
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLZS
 */

void C3DControlPan::OnCSlzsUpdated( wxCommandEvent& event )
{
    // Insert custom code here

	pa->canvas->zs = ((double) m_zs->GetValue())/10000.0;

	if(this->m_select->GetValue())
		pa->canvas->UpdateSelect();
	pa->canvas->Refresh();    
	 
}

/*!
 * Should we show tooltips?
 */

bool C3DControlPan::ShowToolTips()
{
    return true;
}

/*!
 * wxEVT_SCROLL event handler for IDC_SLXP
 */

void C3DControlPan::OnCSlxpScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}


/*!
 * wxEVT_SCROLL event handler for IDC_SLXS
 */

void C3DControlPan::OnCSlxsScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}


/*!
 * wxEVT_SCROLL event handler for IDC_SLYP
 */

void C3DControlPan::OnCSlypScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}


/*!
 * wxEVT_SCROLL event handler for IDC_SLYS
 */

void C3DControlPan::OnCSlysScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}


/*!
 * wxEVT_SCROLL event handler for IDC_SLZP
 */

void C3DControlPan::OnCSlzpScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}


/*!
 * wxEVT_SCROLL event handler for IDC_SLZS
 */

void C3DControlPan::OnCSlzsScroll( wxScrollEvent& event )
{
    // Insert custom code here
    pa->canvas->UpdateViewsDlg();
}



/*!
 * Get bitmap resources
 */

wxBitmap C3DControlPan::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin C3DControlPan bitmap retrieval
    return wxNullBitmap;
////@end C3DControlPan bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon C3DControlPan::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin C3DControlPan icon retrieval
    return wxNullIcon;
////@end C3DControlPan icon retrieval
}
