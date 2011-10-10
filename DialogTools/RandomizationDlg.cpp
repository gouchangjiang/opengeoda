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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces
#include <wx/dcbuffer.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../GeoDaConst.h"

#include "../mapview.h"
#include "../GeoDaConst.h"
#include "../logger.h"
 
#include "../Explore/BoxPlotView.h"
#include "../Explore/ScatterPlotView.h"
#include "../Explore/MoranScatterPlotView.h"
#include "../Explore/MoranGView.h"

#include "RandomizationDlg.h"

IMPLEMENT_CLASS( CRandomizationDlg, wxDialog )

BEGIN_EVENT_TABLE( CRandomizationDlg, wxDialog )
    EVT_PAINT( CRandomizationDlg::OnPaint )
    EVT_BUTTON( XRCID("ID_CLOSE"), CRandomizationDlg::OnCloseClick )
    EVT_BUTTON( XRCID("ID_OK"), CRandomizationDlg::OnOkClick )
END_EVENT_TABLE()

CRandomizationDlg::CRandomizationDlg( int type, wxWindow* win,
									 const int NumPermutations,
									 const double initMoran,
									 wxWindow* parent, wxWindowID id,
									 const wxString& caption, 
									 const wxPoint& pos, const wxSize& size,
									 long style )
  : start(-1), stop(1), Moran(initMoran), Permutations(NumPermutations)
// : wxDialog(parent, id, caption, pos, size, style, caption)
{
	LOG_MSG("In CRandomizationDlg::CRandomizationDlg");

	SetParent(parent);
    CreateControls();
    Centre();

	this->type = type; // type = 1 for moran, 2 for moranG
	if(type == 1) MoranPtr = (MoranScatterPlotCanvas *) win;
	else MoranGPtr = (MoranGCanvas *) win;
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);

	Init();
}

void CRandomizationDlg::Init()
{		
	MoranI = new double[Permutations];
	
	if (Permutations <= 10) bins = 10;
	else if (Permutations <= 100) bins = 20;
	else if (Permutations <= 1000) bins = (Permutations+1)/4;
	else bins = 300;
	
	freq = new int[bins];
	
	range = (stop-start) / bins;
	// find the bin for the original Moran's I
	thresholdBin = (int)floor( (Moran-start)/range );
	// leave a room for those who smaller than I
	if (thresholdBin <= 0) thresholdBin = 1;
	// take the last bin
	else if (thresholdBin >= bins) thresholdBin = bins-1;	
	
	experiment_run_once = false;
}

CRandomizationDlg::~CRandomizationDlg()
{
	if (MoranI) delete [] MoranI; MoranI = 0;
    if (freq) delete [] freq; freq = 0;
}

void CRandomizationDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_RANDOMIZATION");
}

void CRandomizationDlg::OnCloseClick( wxCommandEvent& event )
{	
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}

void CRandomizationDlg::OnOkClick( wxCommandEvent& event )
{
	wxRect rcClient = GetClientRect();
	CheckSize(rcClient.GetWidth(), rcClient.GetHeight());

    RunPermutations();

	Refresh();
	FindWindow(XRCID("ID_CLOSE"))->SetLabel("Done");
}

bool CRandomizationDlg::ShowToolTips()
{
    return true;
}
 
void CRandomizationDlg::DrawRectangle(wxDC* dc, int left, int top,
									  int right, int bottom,
									  const wxColour color)
{
	wxPen Pen;
	Pen.SetColour(color);
	dc->SetPen(Pen);
	wxBrush Brush; 
	Brush.SetColour(color);
	dc->SetBrush(Brush); 

	if (left >= right) right = left+1;
	if (top >= bottom) bottom = top+1;
	
    dc->DrawRectangle(wxRect(left, bottom, right-left, top-bottom)); 
}

void CRandomizationDlg::RunPermutations()
{
	totFrequency = 0;
	signFrequency = 1;
	for (int i=0; i<bins; i++) freq[i]= 0;
	// thresholdBin bin already has one permutation
	freq[ thresholdBin ] = 1;
	// leftmost and the righmost are the same so far
	minBin = thresholdBin; 
	maxBin = thresholdBin;
	
	while (totFrequency < Permutations) SinglePermute();
	UpdateStatistics();
}

void CRandomizationDlg::SinglePermute()
{
	int* perm = 0;
	perm = rng.Perm( gObservation ) ;  // create a random permutation  
	if (perm == 0) return;
		
	double newMoran;
	if(type == 1) {
		newMoran = MoranPtr->OnePermute( perm );   // compute new Moran's I
	} else {
		newMoran = MoranGPtr->OnePermute( perm );  // compute new Moran's I
	}
	
	delete [] perm; perm = 0;
	
	MoranI[ totFrequency++ ] = newMoran;  // find its place in the distribution
	int newBin = (int)floor( (newMoran - start)/range );
	if (newBin < 0) newBin = 0;
	else if (newBin >= bins) newBin = bins-1;
	
	freq[newBin] = freq[newBin] + 1;
	if (newMoran > Moran) signFrequency++;
	if (newBin < minBin) minBin = newBin;
	if (newBin > maxBin) maxBin = newBin;
}

void CRandomizationDlg::UpdateStatistics()
{
	double sMoran = 0;
	for (int i=0; i < totFrequency; i++) {
		sMoran = sMoran + MoranI[i];
	}
	MMean = sMoran / (totFrequency-1);
	double MM2 = 0;
	for (int i=0; i < totFrequency; i++) {
		MM2 = MM2 + ((MoranI[i]-MMean) * (MoranI[i]-MMean));
	}
	MSdev = sqrt(MM2/(totFrequency));
	
	signFrequency = 1;
	if (Moran < 0) {
		for (int i=0; i < totFrequency; i++) {
			if (MoranI[i] <= Moran) signFrequency++;
		} 
	} else {
		for (int i=0; i < totFrequency; i++) {
			if (MoranI[i] > Moran) signFrequency++;
		}
	}

	expected_val = (double) -1/(gObservation - 1);
	pseudo_p_val = (double) signFrequency/(totFrequency+1);
}

void CRandomizationDlg::Draw(wxDC* dc)
{
	int fMax = freq[0];
	for (int i=1; i<bins; i++) if (fMax < freq[i]) fMax = freq[i];

	for (int i=0; i < bins; i++) {
		double df = double (freq[i]* Height) / double (fMax);
		freq[i] = int(df);
	}

	wxColour color = (Moran > 0) ? GeoDaConst::textColor : GeoDaConst::outliers_colour;
	for (int i=0; i < thresholdBin; i++) {
		if (freq[i] > 0) {
			int xx = Top + Height - freq[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}

	color = (Moran > 0) ? GeoDaConst::outliers_colour : GeoDaConst::textColor;
	for (int i=thresholdBin+1; i < bins; i++) {
		if (freq[i] > 0) {
			int xx = Top + Height - freq[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}	
	
	DrawRectangle(dc, Left + thresholdBin*binX, Top,
				  Left + (thresholdBin+1)*binX, Top+  Height,
				  GeoDaConst::highlight_color );

	wxPen drawPen(*wxBLACK_PEN);
	drawPen.SetColour(GeoDaConst::textColor);
	dc->SetPen(drawPen);

	dc->DrawLine(Left, Top, Left, Top+Height); // Vertical axis
	dc->DrawLine(Left, Top+Height, Left+Width, Top+Height); // Horizontal axis

	drawPen.SetColour(wxColour(20, 20, 20));
	dc->SetPen(drawPen);
	dc->SetBrush(*wxWHITE_BRUSH);

	const int hZero= (int)(Left+(0-start)/(stop-start)*Width);
	dc->DrawRectangle(hZero, Top-2 , 3, Height+2);
  
	int fs = 4 + Bottom/4;
	wxFont nf(*wxSMALL_FONT);
	nf.SetPointSize(fs);
	dc->SetFont(nf);
	
	drawPen.SetColour(GeoDaConst::textColor);
	dc->SetPen(drawPen);
	 
	wxString text;
	dc->SetTextForeground(wxColour(0,0,0));	
	text = wxString::Format("I:%-7.4f E[I]:%-7.4f  Mean:%-7.4f   Sd:%-7.4f",
							Moran, expected_val, MMean, MSdev);
	dc->DrawText(text, Left, Top + Height + Bottom/2);
 
	text = wxString::Format("permutations: %d  ", Permutations);
	dc->DrawText(text, Left+5, 20);

	text = wxString::Format("pseudo p-value: %-7.6f", pseudo_p_val);
	dc->DrawText(text, Left+5, 35);
}


void CRandomizationDlg::OnPaint( wxPaintEvent& event )
{
	if(!experiment_run_once) {
		experiment_run_once = true;
		wxCommandEvent ev;
		OnOkClick(ev);
	}
    wxAutoBufferedPaintDC dc(this);
	dc.Clear();
	Paint(&dc);
}

void CRandomizationDlg::Paint(wxDC *dc)
{
	wxPen dPen(*wxBLACK_PEN);
	dPen.SetColour(GeoDaConst::textColor);
    dc->SetPen(dPen);

	wxRect rcClient = GetClientRect();
	CheckSize(rcClient.GetWidth(), rcClient.GetHeight());
	
    Draw(dc);
}

void CRandomizationDlg::CheckSize(const int width, const int height)
{
	Left = 10;    Bottom = 20;
	Right = 10;   Top = 10;
	Height = 40;   Width = 40;	
	
	int res = width - Width - Left - Right;
	if (res < 0) res = 0;
	int rata = (int)floor((double) res / (bins + 2));
	if (rata == 0) rata= 1;
	Left += rata;
	binX = rata;     //  vertical scale is under control of Format
	Right = width - Left - binX*bins;
	if (Right < 0) Right= 0;
	Width= width - Left - Right;
	res= height - Height - Top - Bottom;
	if  (res < 0) res = 0;
	rata= res / 16;
	Top += rata;
	Bottom += rata;
	Height = height - Top - Bottom;
}

