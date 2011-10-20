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
#include <wx/dcmemory.h>
#include <wx/xrc/xmlres.h>
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../Thiessen/VorDataType.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "PCPView.h"

extern MyFrame* frame;
extern	Selection gSelection;
extern	GeoDaEventType	gEvent;

BEGIN_EVENT_TABLE(PCPFrame, TemplateFrame)
	EVT_ACTIVATE(PCPFrame::OnActivate)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(PCPCanvas, wxScrolledWindow)
    EVT_SIZE(PCPCanvas::OnSize)
    EVT_MOUSE_EVENTS(PCPCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(PCPCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()



// ---------------------------------------------------------------------------
// PCP Frame
// ---------------------------------------------------------------------------

PCPFrame::PCPFrame(const std::vector<int>& pcp_col_ids,
				   wxFrame *parent, Project* project,
				   const wxString& title, const wxPoint& pos,
				   const wxSize& size, const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
	my_children.Append(this);
    SetSizeHints(100, 100);
	
	int width, height;
	GetClientSize(&width, &height);
	canvas = new PCPCanvas(pcp_col_ids, project->GetGridBase(), this,
						   wxPoint(0, 0), wxSize(width, height));
	template_canvas = canvas;
	template_canvas->template_frame = this;
	
	Show(true);
}

PCPFrame::~PCPFrame()
{
	LOG_MSG("In PCPFrame::~PCPFrame");
	DeregisterAsActive();
	my_children.DeleteObject(this);
}

void PCPFrame::Update()
{
	wxClientDC dc(canvas);
    PrepareDC(dc);
	canvas->Selection(&dc);
}

void PCPFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In PCPFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("PCPFrame", GetTitle());
	}
    if ( event.GetActive() && canvas ) canvas->SetFocus();
}

/*
void PCPFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In PCPFrame::OnClose");
	
 
	Destroy();
}

void PCPFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In PCPFrame::OnMenuClose");
	Close();
}
*/
 
void PCPFrame::MapMenus()
{
	LOG_MSG("In PCPFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_PCP_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
}

void PCPFrame::OnViewStandardizedData(wxCommandEvent& WXUNUSED(event))
{
	canvas->ViewStandardizedData();
}

void PCPFrame::OnViewOriginalData(wxCommandEvent& WXUNUSED(event))
{
	canvas->ViewOriginalData();
}


// ---------------------------------------------------------------------------
// PCP Canvas
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
PCPCanvas::PCPCanvas(const std::vector<int>& pcp_col_ids,
					 DbfGridTableBase* grid_base,
					 wxWindow *parent,
					 const wxPoint& pos, const wxSize& size)
	:TemplateCanvas(parent, pos, size), pcp_mode(pcp_mode_default),
	pcp_selected_ctrl_point(0), is_line_drag_visible(false),
	line_drag_y_pos(0), n_obs(grid_base->GetNumberRows())
{
    SetBackgroundColour(wxColour("WHITE"));

	n_plp_sel = pcp_col_ids.size();
	plp_sel.clear();
	plp_sel.resize(n_plp_sel);

	for(int i=0; i<n_plp_sel; i++) {
		plp_sel[i] = grid_base->col_data[pcp_col_ids[i]]->name;
	}
	
	l_map = new int[n_plp_sel];
	data = new p_double[n_plp_sel];
	sdata = new p_double[n_plp_sel];
	rdata = new p_double[n_plp_sel];
	coordinate_handle = new double[n_plp_sel];

	std::vector<double> grid_vec;
	
	// read data from table
    for (int i=0; i<n_plp_sel; i++) {
		l_map[i] = i;
		data[i] = new double[n_obs];
		rdata[i] = new double[n_obs];
		sdata[i] = new double[n_obs];
		
		grid_base->col_data[pcp_col_ids[i]]->GetVec(grid_vec);
		for (int row=0; row<n_obs; row++) {
			data[i][row]  = grid_vec[row];
			rdata[i][row] = grid_vec[row];
		}

		GenUtils::StandardizeData(grid_vec);
		for (int row=0; row<n_obs; row++) {  
			sdata[i][row] = grid_vec[row];
		}
	}

	StandardizedData = false;

	mins = new double[n_plp_sel];
    maxs = new double[n_plp_sel];

	location = new p_point[n_plp_sel];
	for(int i=0; i<n_plp_sel; i++) location[i] = new BasePoint[n_obs];

	ShowFirst();
	CheckSize();
}


PCPCanvas::~PCPCanvas()
{
	if (data) delete [] data; data = NULL;
	if (sdata) delete [] sdata; sdata = NULL;
	if (rdata) delete [] rdata; rdata = NULL;
	if (mins) delete [] mins; mins = NULL;
	if (maxs) delete [] maxs; maxs = NULL;
	for(int i=0; i<n_plp_sel; i++) delete location[i];
	if (location) delete [] location; location = NULL;
	if (l_map) delete [] l_map; l_map = NULL;
	if (coordinate_handle) delete [] coordinate_handle;
	coordinate_handle = NULL; 
}

// Define the repainting behaviour
void PCPCanvas::OnDraw(wxDC& dc)
{
	LOG_MSG("Entering PCPCanvas::OnDraw");
    selection_outline_visible = false;
	Draw(&dc);
    LOG_MSG("Exiting PCPCanvas::OnDraw");
}

void PCPCanvas::Draw(wxDC* dc)
{
    DrawAxes(dc);
    DrawLegend(dc);
    DrawAllLines(dc);
}

void PCPCanvas::OnEvent(wxMouseEvent& event)
{
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if (event.RightUp()) {
	    PopupMenu(wxXmlResource::Get()->
			LoadMenu("ID_PCP_VIEW_MENU_CONTEXT"),
			event.GetPosition().x, event.GetPosition().y);
		return;
	}

	wxPoint m_pos(event.GetX(), event.GetY());
	// The left button has just been pressed, but is not dragging which
	// could be detected by LeftIsDown()
	if (event.LeftDown()) {
		LOG_MSG("PCPCanvas::OnEvent: LeftDown");
		// check to see if one of the control circles is selected.
		int h = Left-10; // horizontal position of control points
		bool found = false;
		for (int i=0; i<n_plp_sel && !found; i++) {
			// vertical position of i-th control point
			int v = Top + i*Height/(n_plp_sel-1);
			if (GenUtils::distance(m_pos, wxPoint(h,v)) <= 8) {
				found = true;
				pcp_selected_ctrl_point = i;
				LOG_MSG(wxString::Format("Close to control point %d", i));
			}
		}
		if (found) {
			pcp_mode = pcp_mode_ctrl_sel;
			return;
		} else {
			pcp_mode = pcp_mode_default;
		}
	}
	if (event.LeftUp() && pcp_mode == pcp_mode_ctrl_sel) {
		LOG_MSG("Left button released after selecting a control point.");
		ErasePCPTempLine(); // erase temporary line if it exists.
		// check to see where released
		bool found = false;
		if (m_pos.y <= Top) {
			LOG_MSG("Move control to first position");
			MoveBefore(pcp_selected_ctrl_point, 0);
		} else if (m_pos.y >= Top + Height) {
			LOG_MSG("Move control to last position");
			MoveToEnd(pcp_selected_ctrl_point);
		} else {
			for (int i=1; i<n_plp_sel && !found; i++) {
				// vertical position of i-th control point
				int v = Top + (i-1)*Height/(n_plp_sel-1);
				int v_next = Top + i*Height/(n_plp_sel-1);
				if (v < m_pos.y && m_pos.y <= v_next) {
					LOG_MSG(wxString::Format("Move control point between %d "
											 "and %d control points", i-1, i));
					MoveBefore(pcp_selected_ctrl_point, i);
				}
			}
		}
		if (StandardizedData) ViewStandardizedData();
		else ViewOriginalData();
				
		pcp_mode = pcp_mode_default;
		return;
	}
	if (event.Dragging() && pcp_mode == pcp_mode_ctrl_sel) {
		LOG_MSG("Dragging mouse after selecting a control point.");
		ErasePCPTempLine();
		line_drag_y_pos = m_pos.y;
		DrawPCPTempLine();
		
		return;
	} 
	
	TemplateCanvas::OnEvent(event);
    event.Skip();
}

void PCPCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void PCPCanvas::CheckSize()
{
	wxSize size2 = GetClientSize();

	int width = size2.x;
	int height = size2.y;


	int i = 0;
	Left= plp_sel[0].Length();
    for(size_t j=1; (int) j < n_plp_sel; j++) {
		if (Left < (int) plp_sel[l_map[j]].Length())
			Left = plp_sel[l_map[j]].Length();
	}

    if (Left < 7)
		Left = 70;
    else
		Left = 100 ;

	Right= 0;
	Top= 30; 
	Bottom= 10;
	Width= 40;
	Height= 0;


    int res= width - Left-Right-Width;
    if (res < 0) res= 0;
    int rata= res / 16;
    Left += rata;
    Right += rata;
    Width = width - Left - Right;
    res= height - Top - Bottom - Height;
    if (res < 0) res= 0;
    rata		 = res / 32;
    Top			+= rata;
    Bottom	+= 4*rata;
    Height	 = height - Top - Bottom;
  
    for(i=0; i<n_plp_sel; i++)
	{

		double xDensity;
		if(maxs[l_map[i]]-mins[l_map[i]] == 0.0)
			xDensity = 0.0;
		else
	    xDensity = Width/(maxs[l_map[i]]-mins[l_map[i]]);

        for (int cnt= 0; cnt < n_obs; ++cnt) {
			location[l_map[i]][cnt].x =
				(long)((data[l_map[i]][cnt] - mins[l_map[i]])
					   * xDensity + Left);
			location[l_map[i]][cnt].y = Top+i*Height/(n_plp_sel-1);

		}
      coordinate_handle[l_map[i]] = Top+i*Height/(n_plp_sel-1);
	  
	}
	yGap1 = location[l_map[0]][0].y;
	yGap2 = location[l_map[0]][0].y;
}

void PCPCanvas::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
    event.Skip();
}

int PCPCanvas::SelectByRect(wxMouseEvent& event) {
	wxPoint p1, p2;
    if (gSelect1.x <= gSelect2.x) {
		p1.x= gSelect1.x;  p2.x= gSelect2.x;
	} else {
		p2.x= gSelect1.x;  p1.x= gSelect2.x;
	}
	if (gSelect1.y <= gSelect2.y)  {
		p1.y= gSelect1.y;  p2.y= gSelect2.y;
	} else {
		p2.y= gSelect1.y;  p1.y= gSelect2.y;
	}

    gEvent = (event.ShiftDown()) ? ADD_SELECTION : NEW_SELECTION;

	int mCnt = 0;
	for (int cnt = 0; cnt < n_obs; cnt++) {
		bool inside = false;

		inside = BoxTest(cnt, p1, p2);
		if (inside && (gEvent == NEW_SELECTION || !gSelection.selected(cnt))) {
			gSelection.Push(cnt);
			++mCnt;
		}
	}
	gSelection.Update();
	if (mCnt > 0) {
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
	}

	return mCnt;
}

void PCPCanvas::SelectByPoint(wxMouseEvent& event) {
	int Id = GeoDaConst::EMPTY, distId = GeoDaConst::EMPTY, dist;

	for (int cnt = 0; cnt < n_obs; ++cnt) {
		double r_dist;
		r_dist = LineToPointDist(cnt, gSelect1);

		if (r_dist < 3) {
			dist = (int) r_dist;

			if (Id == GeoDaConst::EMPTY || distId > dist) {
				Id = cnt;
				distId = dist;
			}
		}
	}

	if (Id == GeoDaConst::EMPTY) {
		EmptyClick();
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
		return;
	}

	bool isSelected = gSelection.selected(Id);
	if (event.ShiftDown()) {
		gEvent = isSelected ? DEL_SELECTION : ADD_SELECTION;
	} else {
		gEvent = NEW_SELECTION;
	}
    
	gSelection.Push(Id);
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}

void PCPCanvas::Selection(wxDC* pDC)
{
	int cnt;

	switch (gEvent) {
		case NEW_SELECTION:
            DrawAllLines(pDC);
			DrawAxes(pDC);
			break;

		case ADD_SELECTION:
			cnt = gSelection.Pop();
			while (cnt != GeoDaConst::EMPTY) {
				DrawLine(pDC, cnt, highlight_color);
				cnt = gSelection.Pop();
			}
			break;

		case DEL_SELECTION:
			while ((cnt = gSelection.Pop()) != GeoDaConst::EMPTY) {
				DrawLine(pDC, cnt, GeoDaConst::textColor);
            }
			break;

		default:
			break;
	}
	gSelection.Reset();
}

void PCPCanvas::ErasePCPTempLine()
{
	LOG_MSG("In PCPCanvas::ErasePCPTempLine");
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	wxClientDC dc(this);
	PrepareDC(dc);
    wxDCOverlay overlaydc( canvas_overlay, &dc );
    LOG_MSG("Calling overlaydc.Clear()");
    overlaydc.Clear();
    LOG_MSG("Called overlaydc.Clear()");
#else
	if (!is_line_drag_visible) return;
	wxClientDC dc(this);
	PrepareDC(dc);
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(*wxBLACK_PEN);
	
	dc.DrawCircle(wxPoint(Left-10, line_drag_y_pos), 3);
	dc.DrawLine(Left-10, line_drag_y_pos, Left+Width, line_drag_y_pos);
#endif
	is_line_drag_visible = false;
}

void PCPCanvas::DrawPCPTempLine()
{
	LOG_MSG("In TemplateCanvas::DrawSelectionOutline");
	
	wxClientDC dc(this);
	PrepareDC(dc);
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	wxDCOverlay overlaydc( canvas_overlay, &dc );
	dc.SetPen(*wxBLUE_PEN);
#else
	dc.SetLogicalFunction(wxINVERT);
	dc.SetPen(*wxBLACK_PEN);
#endif

	dc.DrawCircle(wxPoint(Left-10, line_drag_y_pos), 3);
	dc.DrawLine(Left-10, line_drag_y_pos, Left+Width, line_drag_y_pos);
	is_line_drag_visible = true;
}

void PCPCanvas::DrawAxes(wxDC *dc)
{
    wxPen pen;
	pen.SetColour(wxColour(128,128,0));
	pen.SetWidth(2);
    dc->SetPen(pen);

    dc->DrawLine(Left-2, Top+Height,Left+Width, Top+Height);
    dc->DrawLine(Left+Width, Top,Left-10, Top);

    DrawCircles( dc, Left-10, Top, 3, *wxBLACK);

    for(int i=1; i<n_plp_sel; i++) {
		DrawCircles( dc, Left-10, Top+i*Height/(n_plp_sel-1),
					3, *wxBLACK);
	    dc->DrawLine(Left-10, Top+i*Height/(n_plp_sel-1),
					 Left+Width, Top+i*Height/(n_plp_sel-1));
	}

	if (StandardizedData) {
		double const xDensity = Width/10.0;		
		int middle;

		for (int i=0; i<4; i++) {
			middle = (int) ((5.0-i) * xDensity + Left); 
			dc->DrawLine(middle, Top,middle,Top+Height+5);
			
			middle = (int) ((5.0+i) * xDensity + Left); 
			dc->DrawLine(middle, Top,middle,Top+Height+5);
		}
	}
}  

void PCPCanvas::DrawLegend(wxDC *dc)
{
  double    scale= 1;
  int i = 0;
  int sz= (int)(0.25 * Bottom), szy= (int)(0.25 * Left);
  if (sz > 14) sz = 14;
  if (szy > 13) szy = 13;
	sz = (sz + szy) / 2;

  wxFont font(*wxNORMAL_FONT);
  font.SetPointSize(sz);
  dc->SetFont(font);

  wxPen pen;
  pen.SetColour(wxColour(1, 1, 1));
  dc->SetPen(pen);
	
  if (sz > 5)
  {
    for(i=0; i<n_plp_sel; i++)
	  {
           wxString t=plp_sel[l_map[i]]; 
		   t.Trim(false); t.Trim(true);
           dc->DrawText(t, Left-t.Length()*8-30,
						Top+i*Height/(n_plp_sel-1) - 10);

	  }

    wxFont m_font(*wxNORMAL_FONT);
    m_font.SetPointSize((int)(sz/1.3));
    dc->SetFont(m_font);

    for(i=0; i<n_plp_sel; i++)
	  {
		  wxString t = wxString::Format("(%4.2f,%4.2f)",
										mins[l_map[i]], maxs[l_map[i]]); 
		  t.Trim(true);
          dc->DrawText(t, Left-t.Length()*5-20, Top+i*Height/(n_plp_sel-1)+8);

	  }
  }
	if (StandardizedData)
	{
		double const xDensity = Width/10.0;
		int const middle = (int) (5.0 * xDensity + Left); 
		char buf[6] = "0.0"; wxString t = wxString::Format("%s",buf); 
		t.Trim(true);
        dc->DrawText(t, middle-6, Top+Height+8);
	}
}

void PCPCanvas::DrawAllLines(wxDC *pDC)
{	
	bool _on = false;
	int cnt = 0;
    for (cnt= 0; cnt < n_obs; ++cnt) {
		wxColour color= gSelection.selected(cnt) ?
			highlight_color : GeoDaConst::textColor;
		if (!gSelection.selected(cnt))
			DrawLine(pDC, cnt, color);
		else
			_on = true;
	}
	if (_on) {
		for (cnt= 0; cnt < n_obs; ++cnt) {
			wxColour color= gSelection.selected(cnt) ?
				highlight_color : GeoDaConst::textColor;
			if (gSelection.selected(cnt))
				DrawLine(pDC, cnt, color);
		}
	}
}

void PCPCanvas::ShowFirst()
{
	int i = 0, j = 0;
	for (i=0; i<n_plp_sel; i++) {
		mins[l_map[i]] = 1e100;
	    maxs[l_map[i]] = -1e100;
	}

	for(i=0; i<n_plp_sel; i++) {
		for(j=0; j<n_obs; j++)
		{
			if(mins[l_map[i]] > data[l_map[i]][j])
				mins[l_map[i]] = data[l_map[i]][j];
			if(maxs[l_map[i]] < data[l_map[i]][j])
				maxs[l_map[i]] = data[l_map[i]][j];
		}
	}

	if (StandardizedData) {
		for(i=0; i<n_plp_sel; i++) {
				mins[l_map[i]] = -5.0;
				maxs[l_map[i]] = 5.0;
		}
	}

    Refresh();
}

void PCPCanvas::DrawLine(wxDC *s, const int cnt, const wxColour color)
{
	wxPen pen;
	pen.SetColour(color);
	pen.SetWidth(1);
	s->SetPen(pen);

	int dGap = abs(yGap2-yGap1);

	for(int i=0; i<n_plp_sel-1; i++) {
		s->DrawLine((int)(location[l_map[i]][cnt].x), 
					(int)(location[l_map[i]][cnt].y+dGap),
					(int)(location[l_map[i+1]][cnt].x),
					(int)(location[l_map[i+1]][cnt].y+dGap));
	}
}

inline double PCPCanvas::LineToPointDist(int cnt, wxPoint p)
{

	double dis = 1e100;
	for(int i=0; i<n_plp_sel-1; i++)
	{
		if( (p.y > (Top+i*Height/(n_plp_sel-1)))
		   && (p.y < (Top+(i+1)*Height/(n_plp_sel-1))) )
		{
			double x1 = location[l_map[i]][cnt].x; 
			double x2 = location[l_map[i+1]][cnt].x; 
			double y1 = location[l_map[i]][cnt].y; 
			double y2 = location[l_map[i+1]][cnt].y; 
  		double a = (y2-y1);
			double b = -(x2-x1);
			double c = -x1*(y2-y1) + y1*(x2-x1);

			dis = (int)(fabs(a*p.x+b*p.y+c)/sqrt(a*a+b*b));
     }
   }
	return dis;
}

bool PCPCanvas::BoxTest(int cnt, wxPoint p1, wxPoint p2)
{
	int i = 0;
	for(i=0; i<n_plp_sel-1; i++)
	{
		if (location[l_map[i]][cnt].x >= p1.x &&
			location[l_map[i]][cnt].x <= p2.x && 
			location[l_map[i]][cnt].y >= p1.y &&
			location[l_map[i]][cnt].y <= p2.y) return true;

		bool chk1 = false;
		bool chk2 = false;

		if( (p1.y > (Top+i*Height/(n_plp_sel-1))) && (p1.y < 
			(Top+(i+1)*Height/(n_plp_sel-1))) )
			chk1 = true;

		if( (p2.y > (Top+i*Height/(n_plp_sel-1))) && (p2.y < 
			(Top+(i+1)*Height/(n_plp_sel-1))) )
			chk2 = true;

		if(chk1 || chk2)
		{
			double x1 = location[l_map[i]][cnt].x;
			double x2 = location[l_map[i+1]][cnt].x;
			double y1 = location[l_map[i]][cnt].y;
			double y2 = location[l_map[i+1]][cnt].y;
			double a = (y2-y1);
			double b = -(x2-x1);
			double c = -x1*(y2-y1) + y1*(x2-x1);

			if( ((a*p1.x+b*p1.y+c) * (a*p2.x+b*p1.y+c) < 0.0) && chk1 )
				return true;
			if( ((a*p1.x+b*p2.y+c) * (a*p2.x+b*p2.y+c) < 0.0) && chk2 )
				return true;

			double min_x = (location[l_map[i]][cnt].x >
							location[l_map[i+1]][cnt].x) ? 
				location[l_map[i+1]][cnt].x : location[l_map[i]][cnt].x;
			double max_x = (location[l_map[i]][cnt].x >
							location[l_map[i+1]][cnt].x) ?
				location[l_map[i]][cnt].x : location[l_map[i+1]][cnt].x;
			if( ((a*p1.x+b*p1.y+c)*(a*p1.x+b*p2.y+c) < 0.0)
			   && (min_x < p1.x) && (max_x> p1.x) ) return true;
			if( ((a*p2.x+b*p1.y+c)*(a*p2.x+b*p2.y+c) < 0.0)
			   && (min_x < p2.x) && (max_x> p2.x) ) return true;
		}
	}

	if (location[l_map[i]][cnt].x >= p1.x &&
		location[l_map[i]][cnt].x <= p2.x &&
		location[l_map[i]][cnt].y >= p1.y &&
		location[l_map[i]][cnt].y <= p2.y) return true;

	return false;
}

inline void PCPCanvas::DrawCircles(wxDC *s, const int x, const int y,
								   const double circleSize,
								   const wxColour color)
{
    wxBrush Brush; 
    Brush.SetColour(color);
    s->SetBrush(Brush);

    s->DrawEllipse((int)(x-circleSize), (int)(y-circleSize), 
				   (int)(2*circleSize),  (int)(2*circleSize));

}

void PCPCanvas::ViewStandardizedData() 
{

	StandardizedData = true;
    for(int i=0; i<n_plp_sel; i++) {
		for(int row=0; row<n_obs; row++)  
			data[l_map[i]][row] = sdata[l_map[i]][row];
	}
	ShowFirst();
	CheckSize();
}

void PCPCanvas::ViewOriginalData() 
{
	StandardizedData = false;
	for(int i=0; i<n_plp_sel; i++) {
		for(int row=0; row<n_obs; row++)  
			data[l_map[i]][row] = rdata[l_map[i]][row];
	}
	ShowFirst();
	CheckSize();
}

/** Move line l1 before line l2.  Use MoveToEnd to move to last position */
void PCPCanvas::MoveBefore(int l1, int l2)
{
	if (l1 == l2 || l1 == l2-1) return;
	
	int* temp = new int[n_plp_sel];
	
	if (l1 < l2) {
		for (int i=0; i<l1; i++) temp[i] = l_map[i];
		temp[l2-1] = l_map[l1];
		for (int i=l1+1; i<l2; i++) temp[i-1] = l_map[i];
		for (int i=l2; i<n_plp_sel; i++) temp[i] = l_map[i];
	} else {
		for (int i=0; i<l2; i++) temp[i] = l_map[i];
		for (int i=l2; i<l1; i++) temp[i+1] = l_map[i];
		temp[l2] = l_map[l1];
		for (int i=l1+1; i<n_plp_sel; i++) temp[i] = l_map[i];
	}
	
	for (int i=0; i<n_plp_sel; i++) l_map[i] = temp[i];
	delete [] temp;
}

/** Move line l1 to the last position */
void PCPCanvas::MoveToEnd(int l1)
{
	if (l1 == n_plp_sel-1) return;
	
	int* temp = new int[n_plp_sel];
	
	for (int i=0; i<l1; i++) temp[i] = l_map[i];
	for (int i=l1+1; i<n_plp_sel; i++) temp[i-1] = l_map[i];
	temp[n_plp_sel-1] = l_map[l1];
	
	for (int i=0; i<n_plp_sel; i++) l_map[i] = temp[i];
	delete [] temp;
}

void PCPCanvas::SwitchLinePlot(int l1, int l2)
{
	int* temp = new int[n_plp_sel];
	temp[l2] = l_map[l1];
	if (l1 < l2) {
		for (int i=l1; i < l2;i++) temp[i] = l_map[i+1];
		for (int i=0; i<l1; i++) temp[i] = l_map[i];
		for (int i=l2+1; i<n_plp_sel; i++) temp[i] = l_map[i];
	} else {
		for (int i=l2; i < l1; i++) temp[i+1] = l_map[i];
		for (int i=0; i <l2; i++) temp[i] = l_map[i];
		for (int i=l1+1; i <n_plp_sel; i++) temp[i] = l_map[i];
	}

	for (int i=0;i<n_plp_sel;i++) l_map[i] = temp[i];
	
	delete [] temp;
}


