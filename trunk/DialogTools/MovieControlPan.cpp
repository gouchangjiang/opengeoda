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
#include <wx/xrc/xmlres.h>
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../mapview.h"
#include "MovieControlPan.h"

extern GeoDaEventType gEvent;
extern Selection gSelection;
extern MyFrame *frame;

BEGIN_EVENT_TABLE( MovieControlPan, wxPanel )
    EVT_BUTTON( XRCID("IDC_BUTTON1"), MovieControlPan::OnCButton1Click )
    EVT_BUTTON( XRCID("IDC_BUTTON2"), MovieControlPan::OnCButton2Click )
    EVT_BUTTON( XRCID("IDC_BUTTON3"), MovieControlPan::OnCButton3Click )
    EVT_BUTTON( XRCID("IDC_BUTTON4"), MovieControlPan::OnCButton4Click )
    EVT_BUTTON( XRCID("IDC_BUTTON5"), MovieControlPan::OnCButton5Click )
    EVT_CHECKBOX( XRCID("ID_CHECKBOX1"), MovieControlPan::OnCheckbox1Click )
    EVT_SLIDER( XRCID("IDC_SLIDER1"), MovieControlPan::OnCSlider1Updated )
END_EVENT_TABLE()

MovieControlPan::MovieControlPan( wxWindow* parent, int num_obs_s,
								   wxWindowID id,
								   const wxPoint& pos, const wxSize& size,
								   long style )
: num_obs(num_obs_s)
{
    Create(parent, id, pos, size, style);
}

bool MovieControlPan::Create( wxWindow* parent, wxWindowID id,
							  const wxPoint& pos, const wxSize& size,
							  long style )
{
    m_reverse = NULL;
    m_slider = NULL;
    m_label = NULL;
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

	FindWindow(XRCID("IDC_BUTTON2"))->Enable(false);

    return true;
}

void MovieControlPan::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_MAPMOVIE");
    m_reverse = XRCCTRL(*this, "ID_CHECKBOX1", wxCheckBox);
    m_slider = XRCCTRL(*this, "IDC_SLIDER1", wxSlider);
    m_label = XRCCTRL(*this, "IDC_STATIC1", wxStaticText);
}

void MovieControlPan::OnCButton2Click( wxCommandEvent& event )
{
	myB->myP->timer->Stop();
	FindWindow(XRCID("IDC_BUTTON1"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON2"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(true);
}

void MovieControlPan::OnCSlider1Updated( wxCommandEvent& event )
{
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

void MovieControlPan::OnCButton3Click( wxCommandEvent& event )
{
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

void MovieControlPan::OnCButton4Click( wxCommandEvent& event )
{
	myB->myP->timer->Start(m_slider->GetValue()*10);
	FindWindow(XRCID("IDC_BUTTON1"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON2"))->Enable(true);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(false);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(false);
}

void MovieControlPan::OnCButton1Click( wxCommandEvent& event )
{
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

void MovieControlPan::OnCButton5Click( wxCommandEvent& event )
{
	if(myB->current_frame >= num_obs-1)
	{
		FindWindow(XRCID("IDC_BUTTON5"))->Enable(false);
		myB->current_frame = num_obs-1;
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

void MovieControlPan::OnCheckbox1Click( wxCommandEvent& event )
{
	OnCButton2Click(event);
    OnCButton1Click(event);

	int i;
	for(i=0; i<num_obs; i++)
	{
		myB->RawData[i] *= -1;
	}
	IndexSortD(myB->RawData, myB->hidx, 0, num_obs-1);
	myB->Refresh();

}
