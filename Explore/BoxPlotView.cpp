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
#include <wx/clipbrd.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"

#include "../OpenGeoDa.h"
#include "../GeneralWxUtils.h"
#include "../GeoDaConst.h"
#include "../logger.h"

#include "BoxPlotView.h"

extern Selection gSelection;
extern GeoDaEventType gEvent;
extern int gObservation;
extern wxString m_gVar1;
extern double* m_gX;
extern MyFrame* frame;

BEGIN_EVENT_TABLE(BoxPlotFrame, wxFrame)
	EVT_ACTIVATE(BoxPlotFrame::OnActivate)
	EVT_CLOSE(BoxPlotFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), BoxPlotFrame::OnMenuClose)
	EVT_MENU(XRCID("ID_OPTIONS_HINGE_15"), BoxPlotFrame::OnHinge15)
	EVT_MENU(XRCID("ID_OPTIONS_HINGE_30"), BoxPlotFrame::OnHinge30)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(BoxPlotCanvas, wxScrolledWindow)
	EVT_SIZE(BoxPlotCanvas::OnSize)
	EVT_MOUSE_EVENTS(BoxPlotCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(BoxPlotCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()


// ---------------------------------------------------------------------------
// BoxPlot Frame
// ---------------------------------------------------------------------------

BoxPlotFrame::BoxPlotFrame(wxFrame *parent, Project* project,
    const wxString& title,
    const wxPoint& pos, const wxSize& size,
    const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
    old_style = true;
    SetSizeHints(100, 100);
    int width, height;
    GetClientSize(&width, &height);
    canvas = new BoxPlotCanvas(this, wxPoint(0, 0), wxSize(width, height));
    template_canvas = canvas;
    template_canvas->template_frame = this;

    m_bpvarnm = m_gVar1;
    Show(true);
}

BoxPlotFrame::~BoxPlotFrame()
{
}

/** Update is called from myFrame::UpdateWholeView(wxFrame* caller) which
 * notifies all client frames that the highlighted object set has been
 * changed. */
void BoxPlotFrame::Update() {
    wxClientDC dc(canvas);
    PrepareDC(dc);
    canvas->Selection(&dc);
}

void BoxPlotFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void BoxPlotFrame::OnActivate(wxActivateEvent& event)
{
    LOG_MSG("In BoxPlotFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
    if (event.GetActive()) {
        RegisterAsActive("BoxPlotFrame", GetTitle());
    }
    if (event.GetActive() && canvas) canvas->SetFocus();
}

void BoxPlotFrame::UpdateMenuBarCheckMarks(wxMenuBar* menuBar)
{
    UpdateMenuCheckMarks(menuBar->GetMenu(menuBar->FindMenu("Options")));
}

void BoxPlotFrame::UpdateMenuCheckMarks(wxMenu* menu)
{
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_15"),
								  !canvas->hinge3);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_30"),
								  canvas->hinge3);
}

void BoxPlotFrame::OnClose(wxCloseEvent& event)
{
    LOG_MSG("In BoxPlotFrame::OnClose");
    DeregisterAsActive();
    Destroy();
}

void BoxPlotFrame::OnMenuClose(wxCommandEvent& event)
{
    LOG_MSG("In BoxPlotFrame::OnMenuClose");
    Close();
}

void BoxPlotFrame::MapMenus()
{
    LOG_MSG("In BoxPlotFrame::MapMenus");

    wxMenuBar* mb = frame->GetMenuBar();
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
        true);
    // Map Options Menus
    wxMenu* optMenu =
        wxXmlResource::Get()->LoadMenu("ID_BOXPLOT_VIEW_MENU_OPTIONS");
    GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
    UpdateMenuCheckMarks(optMenu);
}

void BoxPlotFrame::OnHinge15(wxCommandEvent& event)
{
    LOG_MSG("In BoxPlotFrame::OnHinge15");
    SetTitle("Box Plot (Hinge = 1.5) - " + m_bpvarnm);
    canvas->OnOptionsHinge15();
    UpdateMenuBarCheckMarks(frame->GetMenuBar());
}

void BoxPlotFrame::OnHinge30(wxCommandEvent& event)
{
    LOG_MSG("In BoxPlotFrame::OnHinge30");
    SetTitle("Box Plot (Hinge = 3.0) - " + m_bpvarnm);
    canvas->OnOptionsHinge30();
    UpdateMenuBarCheckMarks(frame->GetMenuBar());
}

// ---------------------------------------------------------------------------
// BoxPlot Canvas
// ---------------------------------------------------------------------------

BoxPlotCanvas::BoxPlotCanvas(wxWindow *parent, const wxPoint& pos,
							 const wxSize& size, bool conditional_view)
: TemplateCanvas(parent, pos, size), Conditionable(conditional_view)
{
    LOG_MSG("Entering BoxPlotCanvas::BoxPlotCanvas");

    mapper = new int[gObservation];
    RawData = new double [ gObservation ];
    RRawData = new double [ gObservation ];
    Index = new int [ gObservation ];
    Invert = new int [ gObservation ];
	hinge3 = false;

    for (int i=0; i<gObservation; i++) {
		Index[i] = i;
		RRawData[i] = m_gX[i];
	}
    FieldName = m_gVar1;
	
    Init();
    SetBackgroundColour(wxColour("WHITE"));
    CheckSize();
    Refresh();
    LOG_MSG("Exiting BoxPlotCanvas::BoxPlotCanvas");
}

BoxPlotCanvas::~BoxPlotCanvas()
{
	if (RawData) delete [] RawData; RawData = 0;
	if (RRawData) delete [] RRawData; RRawData = 0;
	if (Index) delete [] Index; Index = 0;
	if (Invert) delete [] Invert; Invert = 0;
	if (mapper) delete [] mapper; mapper = 0;
}

bool BoxPlotCanvas::Init()
{
    gcObs = 0;
    if (!mapper || !RRawData || !RawData || !Index || !Invert) {
        gObservation = 0;
        return false;
    }

    for (int i = 0; i < gObservation; i++) {
        if (!conditionFlag[i]) continue;
        mapper[i] = gcObs;
		RawData[gcObs] = RRawData[i];
		Index[gcObs] = gcObs;
        gcObs++;
    }

    if (gcObs < 1) return false;

    IndexSortD(RawData, Index, 0, gcObs-1);
    for (int i=0; i < gcObs; i++) Invert[Index[i]] = i;
	
	MinVal = RawData[Index[0]];
	MaxVal = RawData[Index[gcObs-1]];
	if (gcObs % 2 == 1) { // odd
		MedianVal = RawData[Index[(gcObs+1)/2 - 1]];
	} else { // even
		MedianVal = (RawData[Index[gcObs/2 - 1]]
					 + RawData[Index[gcObs/2]])/2.0;
	}
	
	if (gcObs >= 5) {
		Q1Ind = (gcObs+1)/4 - 1;
		Q3Ind =	3*(gcObs+1)/4 - 1;
	} else {
		Q1Ind = 0;
		Q3Ind = gcObs-1;
	}
	Q1Val = RawData[Index[Q1Ind]];
	Q3Val = RawData[Index[Q3Ind]];
	IQR = Q3Val - Q1Val;
	Mean = 0;
	for (int i=0; i<gcObs; i++) Mean += RawData[i];
	Mean /= gcObs;
	
    CheckHinge(hinge3);

    return true;
}

void BoxPlotCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
    // we must give up the mouse capture when we receive this event
    if (HasCapture()) ReleaseMouse();
}

void BoxPlotCanvas::OnEvent(wxMouseEvent& event)
{
    if (isConditional) return;

    if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
    if (event.LeftUp() && HasCapture()) ReleaseMouse();

    if (event.RightUp()) {
        wxMenu* optMenu = wxXmlResource::Get()->
            LoadMenu("ID_BOXPLOT_VIEW_MENU_CONTEXT");
		((BoxPlotFrame*) template_frame)->UpdateMenuCheckMarks(optMenu);
        PopupMenu(optMenu, event.GetPosition());
        return;
    }

    TemplateCanvas::OnEvent(event);
    event.Skip();
}

/** Called by BoxPlotFrame::Update() which is called when the set of
 highlighted observations has changed in another view. */
void BoxPlotCanvas::Selection(wxDC* pDC)
{
    if (gcObs < 5) return;

	int cnt;
    switch (gEvent) {
        case NEW_SELECTION:
            LOG_MSG("gEvent == NEW_SELECTION");
			DrawBox(pDC);
			for (int i=0; i<gObservation; i++) {
				if (conditionFlag[i]) DrawPoint(pDC, i);
			}
            break;

        case DEL_SELECTION:
            LOG_MSG("gEvent == DEL_SELECTION");
			while ((cnt = gSelection.Pop()) != GeoDaConst::EMPTY) {
				if (!conditionFlag[cnt]) continue;
				DrawPoint(pDC, cnt); // redraw only newly deleted points
			}			
            break;

        case ADD_SELECTION:
            LOG_MSG("gEvent == ADD_SELECTION");
			while ((cnt = gSelection.Pop()) != GeoDaConst::EMPTY) {
				if (!conditionFlag[cnt]) continue;
				DrawPoint(pDC, cnt); // draw only newly added points
			}			
            break;

        default:
            break;
    }
    DrawMedian(pDC); // indicate the median
    gSelection.Reset();
}

/** Called when Refresh is called or a Paint event received. */
void BoxPlotCanvas::OnDraw(wxDC& dc)
{
    selection_outline_visible = false;
    Draw(&dc);
}

/** Assumes a blank canvas and draws everything. */
void BoxPlotCanvas::Draw(wxDC* dc)
{
    if (gcObs < 3) {
        int fs1 = (int) (Bottom * 0.75 - 1);
		if (fs1 < 12) fs1 = 12;
		if (fs1 > 40) fs1 = 40;
        wxPen BoxPen;
        BoxPen.SetColour(GeoDaConst::textColor);
        dc->SetPen(BoxPen);
        wxFont m_font(*wxSMALL_FONT);
        m_font.SetPointSize(max(10,fs1/2));
        dc->SetFont(m_font);
        wxString no_obs;
        no_obs.Printf("(%d)", gcObs);
		int w, h;
		GetClientSize(&w, &h);
        dc->DrawText(no_obs, w/2, 10);
        return;
    }
	

    wxPen pen;
    pen.SetColour(GeoDaConst::textColor);

	if (gcObs >= 5) {
		int upper = (int) (Top + (Upper - UpperWhisker) / Density);
		int lower = (int) (Top + (Upper - LowerWhisker) / Density);

		int whisker_width = (2*mainBox)/3;
		// Draw Upper Whisker
		dc->DrawLine(Left + Width / 2 - whisker_width, upper,
					 Left + Width / 2 + whisker_width, upper);

		// Draw Lower Whisker
		dc->DrawLine(Left + Width / 2 - whisker_width, lower,
					 Left + Width / 2 + whisker_width, lower);
	
		// Draw Center Line between Whiskers
		dc->DrawLine(Left + Width / 2, upper,
					 Left + Width / 2, lower);	
	}
		
    int fs = (int) (Bottom * 0.75 - 3);
    if (fs > 15) fs = 15;
    wxFont m_font(*wxNORMAL_FONT);
    m_font.SetPointSize(max(6,fs));
    dc->SetFont(m_font);
    dc->DrawText(FieldName, Left + Width / 2 - FieldName.Len() * fs / 4,
        Top + Height + Bottom / 8 + 1);

	DrawBox(dc);
	for (int i=0; i<gObservation; i++) if (conditionFlag[i]) DrawPoint(dc, i);
    DrawMedian(dc);
	
    if (gcObs < gObservation && gcObs >= 3) {
        wxFont m_font(*wxNORMAL_FONT);
        m_font.SetPointSize(max(10, (fs*9)/10));
        dc->SetFont(m_font);
        wxString no_obs;
        no_obs.Printf("(%d)", gcObs);
        dc->DrawText(no_obs, Left + Width / 2 + mainBox + 3, Top);
    }
}

void BoxPlotCanvas::DrawBox(wxDC* pDC)
{
    if (gcObs >= 5) {
		// Draw the box which represents data between Q1 and Q3
		int boxtop = Top + (Upper - Q3Val) / Density;
		int boxbottom = Top + (Upper - Q1Val) / Density;

		// For Box Plot we don't draw the points betwen Q1 and Q3,
		// we draw a box instead.
		DrawRectangle(pDC, Left + Width / 2 - mainBox, boxtop,
					  Left + Width / 2 + mainBox, boxbottom,
					  GeoDaConst::textColor, false);
	}
}

void BoxPlotCanvas::DrawMedian(wxDC* pDC)
{
    if (gcObs < 3) return;

    int diameter = circRadius + 2;
    int point = 0;
    point = (int) (Top + (Upper - Mean) / Density);
	
    wxBrush brush;
    brush.SetColour(wxColour(20, 200, 20));
    pDC->SetBrush(brush);

	//draw the mean as a circle.
	pDC->SetPen(*wxBLUE_PEN);
	pDC->DrawCircle(Left + Width/2, point, diameter);

    int location = (int) (Top + (Upper - MedianVal) / Density);
	
	pDC->SetPen(*wxTRANSPARENT_PEN);
    brush.SetColour(wxColour(219, 99, 28)); // orange
    pDC->SetBrush(brush);
	// Draw Median
	pDC->DrawRectangle((Left + Width / 2) - mainBox - 5, location-1,
					   2*(mainBox+5), Height < 150 ? 1 : 3);
	pDC->SetPen(*wxBLACK_PEN);
}

void BoxPlotCanvas::DrawPoint(wxDC* pDC, const int eltt)
{
	if (gcObs < 3) return;
	int elt = mapper[eltt];
	int order = Invert[elt];
	double eValue = RawData[elt];
	wxColour color;
	
    if (gcObs < 5) {
		color = gSelection.selected(eltt) ?
			GeoDaConst::highlight_color : GeoDaConst::outliers_colour;
		GenUtils::DrawSmallCirc(pDC, Left + Width / 2,
								Top + (Upper - eValue) / Density,
								circRadius, color);
	} else {
		if (order > Q3Ind || order < Q1Ind) {
			color = gSelection.selected(eltt) ?
				GeoDaConst::highlight_color : GeoDaConst::outliers_colour;
			GenUtils::DrawSmallCirc(pDC, Left + Width / 2,
									Top + (Upper - eValue) / Density,
									circRadius, color);
		} else if (gSelection.selected(eltt)) {
			int uBound, lBound;
			uBound = Top +
				(Upper - (eValue + RawData[ Index[Invert[elt] + 1] ]) / 2)
					/ Density;
			lBound = Top +
				(Upper - (eValue + RawData[ Index[Invert[elt] - 1] ]) / 2)
					/ Density;
			if (order == Q3Ind) {
				uBound = Top + (Upper - Q3Val) / Density;
			}
			if (order == Q1Ind) {
				lBound = Top + (Upper - Q1Val) / Density;
			}
			color = gSelection.selected(eltt) ?
				GeoDaConst::highlight_color : GeoDaConst::textColor;
			
			// For Box Plot we don't draw the points inside of IQR,
			// we draw a box instead.
			DrawRectangle(pDC, Left + Width / 2 - mainBox, uBound,
						  Left + Width / 2 + mainBox, lBound,
						  color, false);
		}
	}
}

void BoxPlotCanvas::DrawRectangle(wxDC* dc, int left, int top, int right,
								  int bottom, const wxColour color,
								  bool frameOnly)
{
    wxBrush brush;
    brush.SetColour(color);
    dc->SetBrush(brush);

    if (left >= right) right = left + 1;
    if (top >= bottom) bottom = top + 1;

    if (frameOnly) {
		dc->SetPen(wxPen(color));
        dc->DrawLine(left, top, right, top);
        dc->DrawLine(left, bottom, right, bottom);
        dc->DrawLine(left, top, left, bottom);
        dc->DrawLine(right, top, right, bottom);

    } else {
		dc->SetPen(wxPen(color));
        dc->DrawRectangle(left, top, (right - left), bottom - top);
    }
}

void BoxPlotCanvas::CheckSize()
{
    if (gcObs < 3) return;
	
	int width, height;
	GetClientSize(&width, &height);

    Left = 10;  // desired left margin
    Right = 10;  // desired right margin
    Top = 10;  // desired top margin
    Bottom = 10; // desired bottom margin
    Width = 40;  // working screen width
    Height = 40;  // working screen height

    int res = width - Left - Right - Width;
    if (res < 0) res = 0;
    int rata = res / 16;
    Left += rata;
    Right += rata;
	
    Width = width - Left - Right;
    mainBox = Width / 8;  // main box width

    res = height - Top - Bottom - Height;
    if (res < 0) res = 0;
    rata = res / 16;
    Top += rata;
    Bottom += rata;
    Height = height - Top - Bottom;
	if (Height <= 0) Height = GenUtils::max<int>(height, 1);
	
    circRadius = (int) (log10((double) Width + (double) Height) -
        log10((double) gObservation) + (double) Height / 128);
    if (circRadius < 0) circRadius = 1;
    else if (circRadius > 4) circRadius = 4;
	
	CheckHinge(hinge3);
}

void BoxPlotCanvas::CheckHinge(bool hinge3_s)
{
    hinge3 = hinge3_s;
	double m = hinge3 ? 3.0 : 1.5;
	if (gcObs >= 5) {
		UpperWhisker = Q3Val + m * IQR;
		LowerWhisker = Q1Val - m * IQR;
	} else {
		UpperWhisker = MaxVal;
		LowerWhisker = MinVal;
	}
	Lower = GenUtils::min<double>(MinVal, LowerWhisker);
	Upper = GenUtils::max<double>(MaxVal, UpperWhisker);

    Density = (Upper - Lower) / Height;  // Height >= 1;

    Refresh();
}

void BoxPlotCanvas::UpdateCondition(int *flags)
{
    Conditionable::UpdateCondition(flags);
    Init();
	CheckSize();
    Refresh();
}

void BoxPlotCanvas::OnSize(wxSizeEvent& event)
{
    CheckSize();
    Refresh();
    event.Skip();
}

int BoxPlotCanvas::FindProxy(const double Proxy)
{
    if (Proxy >= MaxVal) return Index[gcObs-1];
    if (Proxy <= MinVal) return Index[0];
    int lower = 0, upper = gcObs - 1, current;
    double cValue;
    while (lower <= upper) {
        current = (lower + upper) >> 1;
        cValue = RawData[ Index[current] ];
        if (cValue < Proxy) lower = current + 1;
        else if (cValue > Proxy) upper = current - 1;
        else return Index[current];
    }
    if (cValue < Proxy) lower = current;
    else lower = current - 1;
    double Low = RawData[ Index[lower] ], Up = RawData[ Index[lower + 1] ];
    if (Proxy - Low < Up - Proxy) current = lower;
    else current = lower + 1;
    return Index[current];
}

void BoxPlotCanvas::SelectByPoint(wxMouseEvent& event)
{
    if (abs(Left + Width / 2 - gSelect1.x) > mainBox ||
        gSelect1.y < Top || gSelect1.y > Top + Height) {
        EmptyClick();
		gSelection.Reset(true);
        gSelection.Update();
        frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
        return;
    }
    double value = (Top + Height - gSelect1.y) * Density + Lower;
    int ix = FindProxy(value);

    bool isSelected = gSelection.selected(ix);
    if (event.ShiftDown()) {
        gEvent = isSelected ? DEL_SELECTION : ADD_SELECTION;
    } else {
        gEvent = NEW_SELECTION;
    }

	gSelection.Reset(true);
    gSelection.Push(ix);
    gSelection.Update();
    frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}

int BoxPlotCanvas::SelectByRect(wxMouseEvent& event)
{
	int curr_sel_cnt = 0;
	for (int i=0; i<gObservation; i++) {
		if (gSelection.selected(i)) curr_sel_cnt++;
	}
	
    wxPoint p1, p2;
    if (gSelect1.x <= gSelect2.x) {
        p1.x = gSelect1.x;  p2.x = gSelect2.x;
    } else {
        p2.x = gSelect1.x;  p1.x = gSelect2.x;
    }
    if (gSelect1.y <= gSelect2.y) {
        p1.y = gSelect1.y;  p2.y = gSelect2.y;
    } else {
        p2.y = gSelect1.y;  p1.y = gSelect2.y;
    }

    if (p1.x > Left + Width / 2 + mainBox ||
        p2.x < Left + Width / 2 - mainBox ||
        p1.y > Top + Height || p2.y < Top) {
        EmptyClick();
		if (curr_sel_cnt) {
			gSelection.Update();
			frame->UpdateWholeView(NULL);
			gSelection.Reset(true);
		}
        return 0;
    }

    // smaller y -- larger value
    double v1 = (Top + Height - p1.y) * Density + Lower;
    // larger y -- smaller value
    double v2 = (Top + Height - p2.y) * Density + Lower;
    int b1 = FindProxy(v1), b2 = FindProxy(v2), tx; // value(b1) > value(b2)
    if (RawData[b1] > v1) {
        tx = Invert[b1];
        if (tx != 0) b1 = Index[ tx - 1 ];
    }

    if (RawData[b2] < v2) {
        tx = Invert[ b2 ];
        if (tx < gObservation - 1) b2 = Index[ tx + 1 ];
    }

    gEvent = (event.ShiftDown()) ? ADD_SELECTION : NEW_SELECTION;

	int mCnt = 0;
    for (int ix = Invert[b2]; ix <= Invert[b1]; ++ix) {
        if (gEvent == NEW_SELECTION || !gSelection.selected(Index[ix])) {
            gSelection.Push(Index[ix]);
            ++mCnt;
        }
    }

    if (mCnt == 0) gEvent = NEW_SELECTION;
	if ( !(mCnt == 0 && curr_sel_cnt == 0) ) {
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
    }
	
    return mCnt;
}

void BoxPlotCanvas::OnOptionsHinge15()
{
    CheckHinge(false);
}

void BoxPlotCanvas::OnOptionsHinge30()
{
    CheckHinge(true);
}
