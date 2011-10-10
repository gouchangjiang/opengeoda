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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../DialogTools/MapQuantileDlg.h"
#include "../mapview.h"
#include "MovieControlPan.h"

extern GeoDaEventType	gEvent;


BEGIN_EVENT_TABLE( CMovieControlPan, wxPanel )

////@begin CMovieControlPan event table entries
    EVT_BUTTON( XRCID("IDC_BUTTON1"), CMovieControlPan::OnCButton1Click )

    EVT_BUTTON( XRCID("IDC_BUTTON2"), CMovieControlPan::OnCButton2Click )

    EVT_BUTTON( XRCID("IDC_BUTTON3"), CMovieControlPan::OnCButton3Click )

    EVT_BUTTON( XRCID("IDC_BUTTON4"), CMovieControlPan::OnCButton4Click )

    EVT_BUTTON( XRCID("IDC_BUTTON5"), CMovieControlPan::OnCButton5Click )

    EVT_CHECKBOX( XRCID("ID_CHECKBOX1"), CMovieControlPan::OnCheckbox1Click )

    EVT_SLIDER( XRCID("IDC_SLIDER1"), CMovieControlPan::OnCSlider1Updated )

////@end CMovieControlPan event table entries

END_EVENT_TABLE()

/*!
 * CMovieControlPan constructors
 */

CMovieControlPan::CMovieControlPan( )
{
}

CMovieControlPan::CMovieControlPan( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * CMovieControlPan creator
 */

bool CMovieControlPan::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CMovieControlPan member initialisation
    m_reverse = NULL;
    m_slider = NULL;
    m_label = NULL;
////@end CMovieControlPan member initialisation

////@begin CMovieControlPan creation
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CMovieControlPan creation

	FindWindow(XRCID("IDC_BUTTON2"))->Enable(false);

    return true;
}

/*!
 * Control creation for CMovieControlPan
 */

void CMovieControlPan::CreateControls()
{    
////@begin CMovieControlPan content construction
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_MAPMOVIE");
    m_reverse = XRCCTRL(*this, "ID_CHECKBOX1", wxCheckBox);
    m_slider = XRCCTRL(*this, "IDC_SLIDER1", wxSlider);
    m_label = XRCCTRL(*this, "IDC_STATIC1", wxStaticText);
////@end CMovieControlPan content construction

    // Create custom windows not generated automatically here.

////@begin CMovieControlPan content initialisation

////@end CMovieControlPan content initialisation
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON2
 */

void CMovieControlPan::OnCButton2Click( wxCommandEvent& event )
{
    // Insert custom code here

	myB->myP->timer->Stop();
	FindWindow(XRCID("IDC_BUTTON1"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON2"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(true);
}


/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for IDC_SLIDER1
 */

void CMovieControlPan::OnCSlider1Updated( wxCommandEvent& event )
{
    // Insert custom code here
	int val = m_slider->GetValue()*10;
	wxString label;
	label = wxEmptyString;
	label << val;
	m_label->SetLabel(label);

	if(myB->myP->timer->IsRunning())
	{
		myB->myP->timer->Stop();
		myB->myP->timer->Start(val);
	}
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON4
 */

void CMovieControlPan::OnCButton3Click( wxCommandEvent& event )
{
    // Insert custom code here
	if(myB->current_frame <= 0)
	{
		myB->current_frame = -1;
		FindWindow(XRCID("IDC_BUTTON3"))->Enable(false);
	    myB->InvalidatePolygon(0, false);
	}
	else
	{		
		myB->InvalidatePolygon(myB->current_frame, false);
		myB->current_frame--; 
	    myB->InvalidatePolygon(myB->current_frame, true);
	}
	
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(true);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON5
 */

void CMovieControlPan::OnCButton4Click( wxCommandEvent& event )
{
    // Insert custom code here
	myB->myP->timer->Start(m_slider->GetValue()*10);
	FindWindow(XRCID("IDC_BUTTON1"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON2"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(false);
}

/*!
 * Should we show tooltips?
 */

bool CMovieControlPan::ShowToolTips()
{
    return true;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON1
 */

void CMovieControlPan::OnCButton1Click( wxCommandEvent& event )
{
    // Insert custom code here
	myB->current_frame = -1;
	myB->Refresh(true);

	FindWindow(XRCID("IDC_BUTTON1"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON2"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(true);

	gEvent= NEW_SELECTION;
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_BUTTON5
 */

void CMovieControlPan::OnCButton5Click( wxCommandEvent& event )
{
    // Insert custom code here

	if(myB->current_frame >= gObservation-1)
	{
		FindWindow(XRCID("IDC_BUTTON5"))->Enable(false);
		myB->current_frame = gObservation-1;
	    myB->InvalidatePolygon(myB->current_frame, true);
	}
	else
	{
		if(myB->type == 1)
			myB->InvalidatePolygon(myB->current_frame, false);
		myB->current_frame++; 
	    myB->InvalidatePolygon(myB->current_frame, true);
	}

	FindWindow(XRCID("IDC_BUTTON3"))->Enable(true);
}



/*!
 * Get bitmap resources
 */

wxBitmap CMovieControlPan::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CMovieControlPan bitmap retrieval
    return wxNullBitmap;
////@end CMovieControlPan bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CMovieControlPan::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CMovieControlPan icon retrieval
    return wxNullIcon;
////@end CMovieControlPan icon retrieval
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
 */

void CMovieControlPan::OnCheckbox1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1 in CMovieControlPan.
    // Before editing this code, remove the block markers.
	OnCButton2Click(event);
    OnCButton1Click(event);

	int i;
	for(i=0; i<gObservation; i++)
	{
		myB->RawData[i] *= -1;
//		myB->hidx[i] = myB->hidx[gObservation-i-1];
	}
	IndexSortD(myB->RawData, myB->hidx, 0, gObservation-1);
	myB->Refresh();

}


