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

#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/splitter.h>

#include "../DialogTools/UserConfigDlg.h"
#include "../ShapeOperations/shp.h"
#include "../OpenGeoDa.h"
#include "../GenUtils.h"
#include "../GeneralWxUtils.h"
#include "ConditionalView.h"
#include "../TemplateCanvas.h"
#include "../mapview.h"
#include "../logger.h"

#include "HistView.h"

extern	Selection gSelection;
extern	GeoDaEventType	gEvent;
extern	int gObservation;
extern	wxString m_gVar1;
extern	double* m_gX;
extern MyFrame* frame;
 
BEGIN_EVENT_TABLE(HistFrame, wxFrame)
	EVT_ACTIVATE(HistFrame::OnActivate)
    EVT_CLOSE(HistFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), HistFrame::OnMenuClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(HistCanvas, wxScrolledWindow)
	EVT_SIZE(HistCanvas::OnSize)
    EVT_MOUSE_EVENTS(HistCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(HistCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Hist Frame
// ---------------------------------------------------------------------------

HistFrame::HistFrame(wxFrame *parent, Project* project, const wxString& title,
					 const wxPoint& pos, const wxSize& size,
					 const long style, wxString hist_weight_file,
					 long* hist_weight_freq)
	:TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
    //SetSizeHints(100, 100);
	
	int width, height;
	GetClientSize(&width, &height);
	canvas = new HistCanvas(false, this, wxPoint(0, 0), wxSize(width, height),
							false, hist_weight_file, hist_weight_freq);
	template_canvas = canvas;
	template_canvas->template_frame = this;

	if (hist_weight_file != wxEmptyString) {
		SetTitle( "Connectivity of " + hist_weight_file);
	}

	Show(true);
}

HistFrame::~HistFrame()
{
}

void HistFrame::Update()
{
	LOG_MSG("Entering HistFrame::Update");
	wxClientDC dc(canvas);
    PrepareDC(dc);
	canvas->Selection(&dc);
	LOG_MSG("Exiting HistFrame::Update");
}

void HistFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void HistFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In HistFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("HistFrame", GetTitle());
	}
    if ( event.GetActive() && canvas ) canvas->SetFocus();
}

void HistFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In HistFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void HistFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In HistFrame::OnMenuClose");
	Close();
}

void HistFrame::MapMenus()
{
	LOG_MSG("In HistFrame::MapMenus");

	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_HISTOGRAM_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	
}

void HistFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	canvas->HistogramIntervals();
}



// ---------------------------------------------------------------------------
// Hist Canvas
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
HistCanvas::HistCanvas(bool ViewCC, wxWindow *parent, const wxPoint& pos,
					   const wxSize& size, bool conditional_view,
					   wxString hist_weight_file,
					   long* hist_weight_freq)
: TemplateCanvas(parent, pos, size), Conditionable(conditional_view)
{
	LOG_MSG("Entering HistCanvas::HistCanvas");
	m_ViewCC = ViewCC;
	Ranges = 7;
	RawData = new double[ gObservation+1 ];
	Range = new unsigned char[ gObservation+1 ];
	Grouping = new int[ gObservation+1 ];
	if (!RawData || !Range || !Grouping)  gObservation = 0;
	
	if (hist_weight_file != wxEmptyString) {
		if (hist_weight_freq == 0) {
			wxMessageBox("Error: there was a problem displaying the weights "
						 "characteristics.");
			return;
		}
		intData = true;
		isWeightsHistogram = true;
		FieldName = "Connectivity";
		int numIsolates = 0;
		for (int i=0; i<gObservation; i++) {
			if (hist_weight_freq[i] == 0) numIsolates++;
			RawData[i] = (double) hist_weight_freq[i];
		}
		isolates.resize(numIsolates);
		int cnt=0;
		if (numIsolates > 0) {
			for (int i=0; i<gObservation; i++) {
				if (hist_weight_freq[i] == 0) isolates[cnt++] = i;
			}
		}
	} else {
		intData = false;
		isWeightsHistogram = false;
		FieldName = m_gVar1;
		for (int i=0; i<gObservation; i++) RawData[i] = m_gX[i];
	}
	gSelection.Reset(false); // set Buffer not to empty

	LOG(Ranges);
	if (Ranges < 1) return;
	SetRanges = false;
	Init();
	Selection();
    SetBackgroundColour(wxColour("WHITE"));
	CheckSize();
    Refresh();
	LOG_MSG("Finished calling HistCanvas::Refresh()");
	LOG_MSG("Exiting HistCanvas::HistCanvas");
}

HistCanvas::~HistCanvas()
{
    if (RawData) delete [] RawData; RawData = 0;
    if (Grouping) delete [] Grouping; Grouping = 0;
    if (Range) delete [] Range; Range = 0;
}

void HistCanvas::clear()
{
	// MMM: this is never called, but probably should
	// be in the destructor.
    if (Counts) delete [] Counts; Counts = 0;
    if (Selected) delete [] Selected; Selected = 0;
    if (Groups) delete [] Groups; Groups = 0;
    if (LegendItems) delete [] LegendItems; LegendItems = 0;
    if (RangeColor) delete [] RangeColor; RangeColor = 0;
    if (BreakVals) delete [] BreakVals; BreakVals = 0;
}

// Define the repainting behaviour
void HistCanvas::OnDraw(wxDC& dc)
{
	LOG_MSG("Entering HistCanvas::OnDraw(wxDC&)");
    selection_outline_visible = false;
	if (Ranges < 1) Ranges = 1;
	// Selection();
	Draw(&dc);
	LOG_MSG("Exiting HistCanvas::OnDraw(wxDC&)");
}

void HistCanvas::Draw(wxDC* dc)
{
	LOG_MSG("Entering HistCanvas::Draw");
	DrawLegend(dc);
	DrawHisto(dc);
	LOG_MSG("Exiting HistCanvas::Draw");
}

void HistCanvas::CheckSize()
{
	LOG_MSG("Entering HistCanvas::CheckSize");
	wxSize size2 = GetClientSize();;
	
	int width = size2.x;
	int height = size2.y;
	
    Left= 0;  Right= 30;
	Top= 10;  Bottom= 10;
    Width= Ranges * 4;
    Height= 20;
	
	int res= width - Left-Right-Width;
	if (res < 0) res= 0;
	int rata= (int)(res / (Ranges + 2.5));
	Left += rata/4;
	Width += Ranges * rata;
	Right = width - Width - Left;
	int    column= Width / Ranges;
	if (!hasLegend()) {
		while (Right > column)  {
			Right -= column;
			Width += column;
		}
	}
	
	res= height - Top - Bottom - Height;
	if (res < 0) res= 0;
	rata= res / 16;
	Top +=  column/2;
	Bottom += rata;
	Height = height - Top - Bottom;
	Density= 1.0 * MaxCount/Height;
	LOG_MSG("Exiting HistCanvas::CheckSize");
}

void HistCanvas::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
    event.Skip();
}

int	HistCanvas::SelectByRect(wxMouseEvent& event)
{
	return 0;
}

void HistCanvas::SelectByPoint(wxMouseEvent& event) {
	if (isWeightsHistogram && isolates.size() > 0
		&& gSelect1.y >= 2 && gSelect1.x >= 2
		&& gSelect1.y <= isolates_button_size.GetHeight()+4
		&& gSelect1.x <= isolates_button_size.GetWidth()+4) {
		gSelection.Reset(true);
		gEvent = NEW_SELECTION;
		for (int i=0, iend = isolates.size(); i<iend; i++) {
			gSelection.Push(isolates[i]);
		}
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
		return;
	} else if (gSelect1.x < Left || gSelect1.y < Top
			 || gSelect1.y > Top + Height) {
        EmptyClick();
        return;
    }
    int rge = 0;

    if (gSelect1.x > Left + Width) {
        if (!hasLegend()) {
            EmptyClick();
            gSelection.Reset(true);
            gSelection.Update();
            frame->UpdateWholeView(NULL);
			gSelection.Reset(true);
            return;
        }
        int ht = Height / (Ranges + 1);
        if (ht < 10) {
            EmptyClick();
            gSelection.Reset(true);
            gSelection.Update();
            frame->UpdateWholeView(NULL);
			gSelection.Reset(true);
            return;
        }

        ht = (int) floor((double) ht / 2);
        int leftAlign = Left + Width + 5;

        for (rge = 0; rge < Ranges; ++rge) {

            if ((gSelect1.x > leftAlign) &&
                (gSelect1.x < leftAlign + Right / 5) &&
                (gSelect1.y > Top + 2 * ht * (rge + 1)) &&
                (gSelect1.y < (int) (Top + 2 * ht * (rge + 1) + 1.5 * ht))) {
                //			ClickOnSelected = true;
                break;
            } else if ((gSelect1.x >= leftAlign + Right / 5) &&
                (gSelect1.y > Top + 2 * ht * (rge + 1)) &&
                (gSelect1.y < (int) (Top + 2 * ht * (rge + 1) + 1.5 * ht))) {
                if (rge == 0)
                    continue;
                else if (rge == Ranges - 1)
                    continue;

                if (intData) {
                    const int stepData = (int) RangeData;
                    if (stepData <= 1) {
                        wxMessageBox("Unique values, No ranges!");
                        return;
                    }
                }

                CUserConfigDlg dlg(this);

                wxString str;
                str << wxEmptyString;
                str << "Category " << rge + 1;
                dlg.m_label->SetLabel(str);

                str = wxEmptyString;
                str << BreakVals[rge];
                dlg.s_int = str;
                str = wxEmptyString;
                str << BreakVals[rge + 1];
                dlg.s_int2 = str;

                if (dlg.ShowModal() == wxID_OK) {

                    double val1, val2;
                    (dlg.m_min->GetValue()).ToDouble(&val1);
                    (dlg.m_max->GetValue()).ToDouble(&val2);

                    if (val1 >= val2) {
                        wxMessageBox("Wrong values!");
                        return;
                    }

                    if (val1 < BreakVals[rge - 1]) {
                        wxMessageBox("Wrong values!");
                        return;
                    }

                    if (val2 > BreakVals[rge + 2]) {
                        wxMessageBox("Wrong values!");
                        return;
                    }

                    BreakVals[rge] = val1;
                    BreakVals[rge + 1] = val2;

                    UpdateBins();
                    MakeHistoLegend(LegendItems, BreakVals, Counts, minData,
                        RangeData, Ranges, intData);

                    gSelection.Reset(false);
                    Selection();

                    CheckSize();
                    LOG_MSG("Calling HistCanvas::Refresh()");
                    Refresh();
                    LOG_MSG("Finished calling HistCanvas::Refresh()");
                }
                return;
            }
        }
        if (rge == Ranges) return;

    } else {
        int left = Left;

        for (rge = 0; rge < Ranges; ++rge) {
            int column =
                (int) ((BreakVals[rge + 1] -
                BreakVals[rge]) * Width / (BreakVals[Ranges] - BreakVals[0]));
            int top = (int) (Top + (MaxCount - Counts[rge]) / Density),
                top1 = (int) (Top + (MaxCount - Selected[rge]) / Density + 1);
            if ((gSelect1.x > left) && (gSelect1.x < left + column))
                break;

            left += column;
        }

        if ((rge == Ranges) ||
            (gSelect1.y < Top + (MaxCount - Counts[rge]) / Density)) {
            EmptyClick();
            gSelection.Reset(true);
            gSelection.Update();
            frame->UpdateWholeView(NULL);
			gSelection.Reset(true);
            return;
        }
    }
    int cnt = Groups[rge];

    gSelection.Reset(true);

    { // click on unselected area
        if (event.ShiftDown()) {
            gEvent = ADD_SELECTION;
            for (; cnt != GeoDaConst::EMPTY; cnt = Grouping[cnt])
                if (!gSelection.selected(cnt)) gSelection.Push(cnt);
        }
        else {
            gEvent = NEW_SELECTION;
            for (; cnt != GeoDaConst::EMPTY; cnt = Grouping[cnt]) {
				gSelection.Push(cnt);
			}
        }
    }
    gSelection.Update();
    frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}

void HistCanvas::Selection(wxDC* pDC)
{
	LOG_MSG("Entering HistCanvas::Selection");
	int  cnt, rge;
	
	switch(gEvent)  {
		case NEW_SELECTION :
			LOG_MSG("gEvent == NEW_SELECTION");
			for (rge= 0; rge < Ranges; ++rge) Selected[ rge ]= 0;
			for (cnt=0 ; cnt < gObservation; cnt++) {
				if(!conditionFlag[cnt]) continue;
				
				if (gSelection.selected(cnt))
					++Selected[ Range[ cnt ] ];
			}
			break;
			
		case ADD_SELECTION :
			LOG_MSG("gEvent == ADD_SELECTION");
			while ((cnt= gSelection.Pop()) != GeoDaConst::EMPTY) {
				if(!conditionFlag[cnt]) continue;
				
				if (Counts[Range[cnt]] > Selected[ Range[ cnt ] ])
					++Selected[ Range[ cnt ] ];
			}
			break;
			
		case DEL_SELECTION :
			LOG_MSG("gEvent == DEL_SELECTION");
			while ((cnt= gSelection.Pop()) != GeoDaConst::EMPTY){
				if(!conditionFlag[cnt]) continue;
				
				if (Selected[ Range[ cnt ]]> 0) --Selected[ Range[ cnt ] ];
			}
			break;
			
		default :
			break;
	}
	
	if (!pDC) {
		wxClientDC tempDC(this);
		DrawHisto(&tempDC);
	} else {
		DrawHisto(pDC);
	}
	gSelection.Reset();
	LOG_MSG("Exiting HistCanvas::Selection");
}

void HistCanvas::OnEvent(wxMouseEvent &event)
{
	if (isConditional) return;

	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();
	
    wxClientDC dc(this);
    PrepareDC(dc);

    wxPoint point(event.GetLogicalPosition(dc));

	if(event.RightUp()) {
	    PopupMenu(wxXmlResource::Get()->
            LoadMenu("ID_HISTOGRAM_VIEW_MENU_CONTEXT"),
				  event.GetPosition().x, event.GetPosition().y);
		return;
	}

	if(event.LeftDClick()) {
		OnLButtonDblClk(event, point);
        return;
	}
	if(event.LeftDown()) {
		OnLButtonDown(event, point);
        return;
	}
	if(event.LeftUp()) {
		OnLButtonUp(event, point);
        return;
	}
	if(event.Dragging()) {
		OnMouseMove(event, point);
        return;
	}

	event.Skip();
}

void HistCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

/////////////////////////////////////////////////////////////////////////////
// CBrushView message handlers

void HistCanvas::OnMouseMove(wxMouseEvent& event, wxPoint& point) 
{
}

void HistCanvas::OnLButtonDblClk(wxMouseEvent& event, wxPoint& point) 
{
	gSelection.Reset(true);
	gSelection.Invert();
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (gSelection.selected(cnt)) {
			gSelection.Push(cnt);
		}
	}
	gEvent= NEW_SELECTION;
	btnDown= false;
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}


void HistCanvas::OnLButtonDown(wxMouseEvent& event, wxPoint& point) 
{
   CaptureMouse();

   gSelect1 = point;
   gSelect2= gSelect1;
   gRegime= RECT_SELECT;        // working on creating a rectangle
   // CLOCK FClickClock= clock();
   btnDown = true;
	
}

void HistCanvas::OnLButtonUp(wxMouseEvent& event, wxPoint& point) 
{
   if (HasCapture()) ReleaseMouse();

   if (!btnDown) return;
   btnDown= false;
   gSelect2 = point;
   if (abs(gSelect1.x-gSelect2.x) + abs(gSelect1.y-gSelect2.y) <= 3) {
      gRegime= NO_SELECT;
      SelectByPoint(event);
	  gSelection.Reset( true );

      return;
   }
}


//** generate color
void ColorGenerator(wxColour * Colors, int Num)  
{
	const int     Intensity[]= { 255, 0, 128, 64, 192, 32, 160, 96, 224 };
	int pt= 2, having= 0, red= 0, green= 0, blue= 1;
	while (pt*pt*pt-pt < Num) ++pt;

	for (having= 0; having < Num; ++having) {
		if (red == green && green == blue) ++blue;        // skip gray
		Colors[ having ] = wxColour(Intensity[red], 
									Intensity[green], Intensity[blue]);
		if (++blue == pt) {
			blue= 0;
			if (++green == pt) {
				green= 0;
				++red;
			}
		}
	}
	Colors[0]= wxColour(128, 128, 128);      // replace yellow with gray
	// replace yellowish with dark-gray
	if (pt > 2) Colors[ 1 ]= wxColour(64, 64, 64);
	if (Num > 4) {
		int lim= (Num-1)/2;
		having = (Num-2*lim == 2) ? 2 : 1;
		for ( ; having <= lim; having += 2) {
			wxColour tm= Colors[having];
			Colors[having]= Colors[having+lim];
			Colors[having+lim]= tm;
		}
	}
}

void HistCanvas::DrawRectangle(wxDC *pDC,
							   int left, 
							   int top, 
							   int right, 
							   int bottom, 
							   const wxColour color, 
							   const bool frameOnly)  
{
	//LOG_MSG("Entering HistCanvas::DrawRectangle");
	wxBrush brush;
	brush.SetColour(color);
	pDC->SetBrush(brush);

	if (left >= right) right= left+1;
	if (top >= bottom) bottom= top+1;

	if (frameOnly) 
	{
		pDC->DrawLine(left, top, right, top);
		pDC->DrawLine(left, bottom, right, bottom);
		pDC->DrawLine(left, top, left, bottom);
		pDC->DrawLine(right, top, right, bottom);
	}
    else
	{
		pDC->DrawRectangle(left, top, right-left, bottom-top);	
	}
	//LOG_MSG("Exiting HistCanvas::DrawRectangle");
}

void HistCanvas::DrawLegend(wxDC *pDC)  
{
	if (isWeightsHistogram && isolates.size() > 0) {
		wxFont link_font(*GeoDaConst::small_font);
		link_font.SetUnderlined(true);
		pDC->SetFont(link_font);
		wxString text;
		if (isolates.size() == 1) {
			text = "Select the neighborless observation";
		} else {
			text = wxString::Format("Select all %d neighborless"
									" observations", (int) isolates.size());
		}
		isolates_button_size = pDC->GetTextExtent(text) + wxSize(2, 2);
		int bw = isolates_button_size.GetWidth();
		int bh = isolates_button_size.GetHeight();
		pDC->SetTextForeground(*wxBLUE);
		pDC->DrawText(text, 4, 4);
		pDC->SetTextForeground(*wxBLACK);
	}
	if (m_ViewCC) {
		wxString text = wxString::Format("(%d)", (int) gcObs);
		pDC->SetFont(*GeoDaConst::small_font);
		pDC->SetTextForeground(*wxBLACK);
		int w, h;
		GetClientSize(&w, &h);
		pDC->DrawText(text, w-30, 2);
	}
	
	if (!hasLegend()) return; 
	int   ht= Height/( Ranges + 1), rge= 0;
  
	int   fs= (int)(Bottom*0.75-3);
	fs = min(fs, 20);
	wxFont m_font2(*wxNORMAL_FONT);
	m_font2.SetPointSize(max(fs,6));
	pDC->SetFont(m_font2);
	pDC->DrawText(FieldName, Left+Width/2-FieldName.Len()*fs/2,
				  Top+Height+Bottom/8);
  
	ht= (int)floor((double) ht/2);
	int leftAlign= Left + Width + 5;
	DrawRectangle(pDC,leftAlign, Top+2*ht*rge,
				  leftAlign+Right/5, (int)(Top+2*ht*rge+1.5*ht),
				  highlight_color, 0);
	DrawRectangle(pDC,leftAlign, Top+2*ht*rge,
				  leftAlign+Right/5, (int)(Top+ht*rge+1.5*ht),
				  GeoDaConst::textColor, 1);
	
	for (rge= 0; rge < Ranges; ++rge) {
		DrawRectangle(pDC, leftAlign, Top+2*ht*(rge+1),
					  leftAlign+Right/5, (int)(Top+2*ht*(rge+1)+1.5*ht),
					  RangeColor[rge], 0);
		DrawRectangle(pDC, leftAlign, Top+2*ht*(rge+1),
					  leftAlign+Right/5, (int)(Top+2*ht*(rge+1)+1.5*ht),
					  GeoDaConst::textColor, 1);
	}
	
	int fsize= (int)(ht*0.75 + Right/20.0);
	wxFont m_font(*wxSMALL_FONT);
	m_font.SetPointSize(max((int)(ht*0.7),6));
	pDC->SetFont(m_font);

	rge= 0;
	leftAlign += Right/5 + 3;
	pDC->DrawText("selected features", leftAlign, Top+ht*2*rge+2);
	for (rge= 1; rge <= Ranges; ++rge) {
		char * legItem = LegendItems[rge-1];
		wxString text;
		//text << LegendItems[rge-1];
		text = wxEmptyString;
		int ct=0;
		while(LegendItems[rge-1][ct] != '\0') {
			text = text + wxString::Format("%c",LegendItems[rge-1][ct]);
			ct++;
		}

		pDC->DrawText(text,leftAlign, Top+ht*2*rge+2);
	}
}

void HistCanvas::DrawHisto(wxDC* pDC)  
{
	LOG_MSG("Entering HistCanvas::DrawHisto(wxDC*)");
	
	int left= Left;
	for (int rge= 0; rge < Ranges; ++rge) {
		int column =
			(int) ((BreakVals[rge+1]-
					BreakVals[rge])*Width/(BreakVals[Ranges]-BreakVals[0]));
		int top, top1;
		top  = (int)(Top + (MaxCount-Counts[rge])/Density);
		top1 = (int)(Top + (MaxCount-Selected[rge])/Density+1);
		if (Counts[rge]-Selected[rge]) 
			DrawRectangle(pDC,
						  left, top,
						  left+column, top1,
						  RangeColor[rge], 0);
		if (Selected[rge]) 
			DrawRectangle(pDC,
						  left, top1,
						  left+column, Top+Height+1,
						  highlight_color,0);
		if (Counts[rge])   
			DrawRectangle(pDC,
						  left, top,
						  left+column, Top+Height+1,
						  GeoDaConst::textColor, 1);
		left += column; 
	}
	
	pDC->DrawLine(Left, Top+Height+1,
				  Left+Width-2, Top+Height+1);
	
	LOG_MSG("Exiting HistCanvas::DrawHisto(wxDC* )");
}

void HistCanvas::UpdateBins()
{
	LOG_MSG("Entering HistCanvas::UpdateBins");
	int *tCount = new int[ Ranges+1];
	int cnt = 0; 
	
	for (cnt= 0; cnt < Ranges; ++cnt)  
	{ 
		tCount[cnt] = 0;
		
		Counts[ cnt ] =  0;
		Selected[ cnt ] = 0;        // counts & selected in each group are zero
		Groups[ cnt ] = GeoDaConst::EMPTY;                   // each group is empty
		
	}
	
	int rge = 0, bucket = 0;
	for (cnt= 0; cnt < gObservation; ++cnt) {
		for(bucket=0; bucket<Ranges; bucket++)
		{
			if( (RawData[cnt] < BreakVals[bucket+1] ) )
			{
				//			rge = bucket;
				break;
			}
		}
		rge = bucket;
		
		if(rge >= Ranges) rge = Ranges - 1;
		/*
		 int rge= (int)floor( (RawData[cnt] - minData)/RangeData);
		 if (rge < 0) rge= 0;
		 if (rge >= Ranges) rge = Ranges - 1;
		 */
		Range[cnt]= rge;
		
		tCount[rge]++;
		
		if(conditionFlag[cnt])
			++Counts[rge];
		Grouping[cnt]= Groups[rge];//save previous starting element of the group
		Groups[rge]= cnt;          // set a new starting element of the group
	}
	
	MaxCount = tCount[0];
	//	double initVal = minData;
	for (rge= 0; rge < Ranges; ++rge) {
		if (MaxCount < tCount[rge]) MaxCount= tCount[rge];
		
		RangeColor[rge] = GetColor(rge+1, intData);
		//		initVal += RangeData;
	}
	LOG_MSG("Exiting HistCanvas::UpdateBins");
}


void HistCanvas::MakeHistoLegend(char ** Legend, double* BreakVals,
								 const int* counts, const double minData,
								 const double rangeData,
								 const int ranges, const bool intData)  
{
	char  buf2[16], *lgd= Legend[0];
	if (intData) {
		const int  stepData= (int) rangeData; 
		for (int rge= 0; rge < ranges; ++rge) {
			Legend[rge] = lgd; 
			GenUtils::longToString(BreakVals[rge], lgd, 10);
			if (stepData > 1) {
				strcat(lgd, " : "); 
				GenUtils::longToString(BreakVals[rge+1], buf2, 10);
				strcat(lgd, buf2);
			}

			strcat(lgd, " (");
			GenUtils::longToString(counts[rge], buf2, 10);
			strcat(lgd, buf2);
			strcat(lgd, ")");

			while (*lgd) ++lgd;
			++lgd; 
		}
	} else { 
		for (int rge= 0; rge < ranges; ++rge) {
	        Legend[rge] = lgd;

			double a = fabs(BreakVals[rge]);
			if (BreakVals[rge]<0) {
				lgd[0]='\0';
				strcat(lgd,"-");						 
				ggcvt(a, 3, buf2);
				strcat(lgd, buf2);
			} else {
				ggcvt(a, 3, lgd);
			}

			strcat(lgd, " : ");
			
			a = fabs(BreakVals[rge+1]);
			if (BreakVals[rge+1]<0) strcat(lgd, "-");
			ggcvt(a, 3, buf2);
			strcat(lgd, buf2);

			strcat(lgd, " (");
			GenUtils::longToString(counts[rge], buf2, 10);
			strcat(lgd, buf2);
			strcat(lgd, ")");

		    while (*lgd) ++lgd;
			++lgd; 
		}
	}
}

bool HistCanvas::Init()  
{
	LOG_MSG("Entering HistCanvas::Init()");
	if (m_ViewCC) {
		gcObs = 0;
		for (int i=0; i<gObservation; i++) if (conditionFlag[i]) gcObs++;
	} else {
		gcObs = gObservation;
	}
	CheckSize();
	grouping();
	LOG_MSG("Exiting HistCanvas::Init()");
	return true;
}

bool HistCanvas::grouping()  
{
	LOG_MSG("Entering HistCanvas::grouping");
	LOG(Ranges);
	LOG(intData);
	minData= RawData[0], maxData= minData;
	
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (RawData[cnt] < minData) {
			minData = RawData[cnt];
		} else if (RawData[cnt] > maxData) {
			maxData= RawData[cnt];
		}
	}
	
	LOG(SetRanges);
	LOG(m_ViewCC);
	if (!SetRanges) {
		if (Ranges < minGroups) Ranges= minGroups;
		if (Ranges > maxGroups) Ranges= maxGroups;
		
		if (intData) {
			Ranges= (int)(maxData-minData+1);
			if (Ranges > maxGroups) Ranges = maxGroups;
		}
	} else {
		if (intData) {
			if (Ranges > (maxData-minData+1))
			{
				Ranges = (int)(maxData-minData+1);
				if (Ranges > maxGroups) Ranges = maxGroups;
			}
		}
	}

	if (m_ViewCC && Ranges > 7) Ranges = 7;
	LOG(Ranges);
	
	BreakVals = new double[Ranges+1];
	
	Groups= new int [ Ranges +1];
	Counts= new int [ Ranges +1];
	Selected= new int [Ranges ];
	RangeColor= new wxColour [ Ranges +1];    // include color for seleted items
	LegendItems= new charPtr [ Ranges +1];
	if (LegendItems)
		LegendItems[0]= new char [ Ranges * 32 ];
	if (!Groups || !Counts || !Selected || !RangeColor
		|| !LegendItems || !LegendItems[0]) { 
		gObservation= 0;
		return false;
	}
	for (int cnt=0; cnt < Ranges; ++cnt) {
		Counts[ cnt ] =  0;
		Selected[ cnt ] = 0;      // counts & selected in each group are zero
		Groups[ cnt ] = GeoDaConst::EMPTY;                     // each group is empty
		
	}
	RangeData= (maxData - minData) / Ranges;
	
	if (intData) RangeData = ceil((maxData-minData+1)/Ranges);
	
	if (intData) {
		const int  stepData= (int) RangeData;
		int v1= (int)minData, v2= (int)(minData + stepData - 1);
		for (int rge= 0; rge < Ranges; ++rge) { 
			BreakVals[rge] = v1;
			
			v1 +=  stepData;
			v2 +=  stepData;
			
		}
		BreakVals[Ranges] = v2;
	} else { 
		double    v1= minData, v2= minData+RangeData;
		for (int rge= 0; rge < Ranges; ++rge) { 
			BreakVals[rge] = v1;
			
			v1 += RangeData;
			v2 += RangeData;
		}
		BreakVals[Ranges] = (v2 - RangeData*0.9999) < maxData ?
			(v2 - RangeData*0.9999) : maxData;
	}
	
	UpdateBins();
	
	MakeHistoLegend(LegendItems, BreakVals, Counts, minData,
					RangeData, Ranges, intData);
	

	LOG_MSG("Calling HistCanvas::Refresh()");
	Refresh();
	LOG_MSG("Finished calling HistCanvas::Refresh()");
	
	LOG_MSG("Exiting HistCanvas::grouping(const int)");
	return true;
}


#include "../DialogTools/HistIntervalDlg.h"

void HistCanvas::HistogramIntervals() 
{
	CHistIntervalDlg dlg(this);
	wxString text;
	text << Ranges;
	dlg.m_intervals->SetValue(text);

	if (dlg.ShowModal () == wxID_OK) {
		wxString text;
		text << dlg.m_intervals->GetValue();
		if(!text.IsNumber()) {
			wxMessageBox("Please type a number");
			HistogramIntervals();
			return;
		}
		
		long val;
		text.ToLong(&val);
		Ranges = val;
		SetRanges = true;
		Init();
		gSelection.Reset(false);  
		Selection();
		CheckSize();
		LOG_MSG("Calling HistCanvas::Refresh()");
		Refresh(true);
		LOG_MSG("Finished calling HistCanvas::Refresh()");
	}
}


wxColour HistCanvas::GetColor(float data, bool IntData)
{
	float col[3];
	convert_rgb(((int)(220.0-220.0*data/Ranges))%360,
				(float) 0.7, (float) 0.7, col);
	wxColour color = wxColour((int)(col[0]*255),
							  (int)(col[1]*255),(int)(col[2]*255));
	return color;
}


void HistCanvas::UpdateCondition(int *flags)
{
	Conditionable::UpdateCondition(flags);
	Init();
	gSelection.Reset(false);
	Selection();
	
	LOG_MSG("Calling HistCanvas::Refresh()");
	Refresh(true);  
	LOG_MSG("Finished calling HistCanvas::Refresh()");
}
