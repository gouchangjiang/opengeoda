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
#include <wx/xrc/xmlres.h>  // XRC XML resouces
#include <wx/sizer.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../GeoDaConst.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../Project.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../mapview.h"
#include "../logger.h"
#include "BoxPlotView.h"
#include "HistView.h"
#include "ScatterPlotView.h"
#include "ConditionalView.h"


BEGIN_EVENT_TABLE(ConditionalViewFrame, wxFrame)
	EVT_ACTIVATE(ConditionalViewFrame::OnActivate)
    EVT_CLOSE(ConditionalViewFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), ConditionalViewFrame::OnMenuClose)
END_EVENT_TABLE()

inline void convert_rgb(float x, float y, float z, float* result);

MapSortElement::MapSortElement(double v, int i) {
    value = v;
    recordIndex = i;
}

bool MapSortElement::operator>(MapSortElement& a) {
    return (value > a.value);
}

bool MapSortElement::operator<(MapSortElement& a) {
    return (value < a.value);
}

MapSortElement& MapSortElement::operator=(const MapSortElement& a) {
    value = a.value;
    recordIndex = a.recordIndex;
    return *this;
}

ConditionalViewFrame::ConditionalViewFrame(wxFrame *parent,
										   Project* project,
										   double* v1, const wxString& v1_name,
										   double* v2, const wxString& v2_name,
										   double* v3, const wxString& v3_name,
										   double* v4, const wxString& v4_name,
										   const wxString& title,
										   const wxPoint& pos,
										   const wxSize& size,
										   const long style)
: TemplateFrame(parent, project, title, pos, size, style ),
num_obs(project->GetNumRecords())
{
	old_style = true;
	my_children.Append(this);
    SetSize(800, 600);
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	switch(Conditionable::cViewType) {
	case 1:
		// SetMenuBar(wxXmlResource::Get()->
		//    LoadMenuBar(""));
		break;
	case 2:
		// SetMenuBar(wxXmlResource::Get()->
		//    LoadMenuBar(""));
		break;
	case 3:
		// SetMenuBar(wxXmlResource::Get()->
		//    LoadMenuBar(""));
		break;
	case 4:
		// SetMenuBar(wxXmlResource::Get()->
		//    LoadMenuBar(""));
		break;
	default:
		break;

	}

    flags = new int[num_obs];
	RawDataX = new double[num_obs];
	RawDataY = new double[num_obs];
	for (int i=0; i<num_obs; i++) {
		RawDataX[i] = v1[i];
		RawDataY[i] = v2[i];
	}

	xMin = RawDataX[0];
	xMax = RawDataX[0];
    yMin = RawDataY[0];
	yMax = RawDataY[0];
	
	for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
		if(xMin > RawDataX[i]) xMin = RawDataX[i];
		if(xMax < RawDataX[i]) xMax = RawDataX[i];
		if(yMin > RawDataY[i]) yMin = RawDataY[i];
		if(yMax < RawDataY[i]) yMax = RawDataY[i];
	}

	XCategory();
	YCategory();

	mViewType = Conditionable::cViewType;

	wxFlexGridSizer* sizer_3x2;
	wxGridSizer* sizer_3x3;
	sizer_3x2 = new wxFlexGridSizer(2,  // 2 column flex grid sizer
									0,  // horizontal pixels between windows
									0); // vertical pixels between windows
	sizer_3x3 = new wxGridSizer(3,  // 3 column grid sizer
								0,  // horizontal pixels between windows
								0); // vertical pixels between windows
	win_array.resize(9, 0);

	//   win_00_null     win_01_hdr_or_null
	//   win_10_y_axis   sizer_3x3
	//   win_20_null     win_21_x_axis	
	
	const int left_width = 95;
	const int right_width = 200;
	const int top_height = 70;
	const int middle_height = 200;
	const int bottom_height = 80;

	win_00_null = new ConditionalVariableNULLView(this, wxDefaultPosition,
												   wxSize(left_width,
														  top_height));
	if(mViewType == 1) {
		win_01_hdr_or_null =
		new ConditionalVariableHeaderView(this, grid_base,
										   v3, v3_name, wxDefaultPosition,
										   wxSize(right_width,
												  top_height));
	} else {
		win_01_hdr_or_null =
		new ConditionalVariableNULLView(this, wxDefaultPosition,
										 wxSize(right_width,
												top_height));
	}
	win_10_y_axis = new ConditionalVariableYView(v2_name, RawDataY,
												  this, num_obs,
												  wxDefaultPosition,
												  wxSize(left_width,
														 middle_height));
	// win_11 doesn't exist, but will be sizer_3x3
	win_20_null = new ConditionalVariableNULLView(this, wxDefaultPosition,
												   wxSize(left_width,
														  bottom_height));
	win_21_x_axis = new ConditionalVariableXView(v1_name, RawDataX,
												  this, num_obs,
												  wxDefaultPosition,
												  wxSize(right_width,
														 bottom_height));

	win_10_y_axis->pFrame = this;
	win_10_y_axis->yy[0] = y4;
	win_10_y_axis->yy[1] = y3;
	win_10_y_axis->yy[2] = y2;
	win_10_y_axis->yy[3] = y1;

	win_21_x_axis->pFrame = this; 
	win_21_x_axis->xx[0] = x1;
	win_21_x_axis->xx[1] = x2;
	win_21_x_axis->xx[2] = x3;
	win_21_x_axis->xx[3] = x4;
		
    switch(mViewType) {
		case 1:
            for(int i=0; i<9; i++) {
				Conditionable::cLocator = i+1;
				TemplateCanvas* tc =
					new MapMovieCanvas(this, num_obs,
									   wxPoint(0,0), wxSize(70,70), true);
				tc->SetSelectableOutlineVisible(false);
				((MapMovieCanvas*) tc)->AddMap(project->shp_fname.GetFullPath(),
											   v3, v3_name);
				win_array[i] = (wxWindow*) tc;
            }
            break;
		case 2:
            for(int i=0; i<9; i++) {
				Conditionable::cLocator = i+1;
				TemplateCanvas* tc =
					new BoxPlotCanvas(this,
									  v3, num_obs, v3_name,
									  wxPoint(0,0), wxSize(0,0), true);
				win_array[i] = (wxWindow*) tc;
            }
            break;
		case 3:
            for(int i=0; i<9; i++) {
				Conditionable::cLocator = i+1;
				TemplateCanvas* tc =
					new HistCanvas(true, this,
								   v3, num_obs, v3_name,
								   wxPoint(0,0), wxSize(0,0), true);
				win_array[i] = (wxWindow*) tc;
            }
            break;
		case 4:
            for(int i=0; i<9; i++) {	
				Conditionable::cLocator = i+1;
				TemplateCanvas* tc =
					new ScatterPlotCanvas(this, v3, v4,
										  num_obs, v3_name, v4_name,
										  wxPoint(0,0), wxSize(0,0), true);
				win_array[i] = (wxWindow*) tc;
            }
            break;
		default:
			wxMessageBox("Error: Bad choice");		
	}
	
	//   win_00_null     win_01_hdr_or_null
	//   win_10_y_axis   sizer_3x3
	//   win_20_null     win_21_x_axis
	
	wxSizerFlags win_flags(1);
	win_flags.Expand().Border(wxALL, 0);
	sizer_3x2->Add(win_00_null, win_flags);
	sizer_3x2->Add(win_01_hdr_or_null, win_flags);
	sizer_3x2->Add(win_10_y_axis, win_flags);
	for (int i=0; i<9; i++) {
		wxWindow* win_tmp = win_array[i];
		wxASSERT( win_tmp );
		sizer_3x3->Add(win_tmp, 1, wxEXPAND | wxALL, 0);
	}
	sizer_3x2->Add(sizer_3x3, win_flags); // sizer_3x3 occupies position 1,1
	sizer_3x2->Add(win_20_null, win_flags);
	sizer_3x2->Add(win_21_x_axis, win_flags);

	sizer_3x2->AddGrowableCol(1);
	sizer_3x2->AddGrowableRow(1);

	SetSizerAndFit(sizer_3x2);
	
	UpdateViews(); 
	Show(true);	
}

void ConditionalViewFrame::QuickSortFieldValT(MapSortElement* array,
											  int low, int high)
{
	int i, j; 
	SortT(&array[low], &array[(low+high)/2], &array[high]);	
	if ((high - low) > 2) {
		SwapT(&array[low + 1], &array[(low + high)/2]);	
		i = low + 1;
		j = high;
		while (i < j) {
			i++;
			while (array[i] < array[low + 1]) i++;
			j--;
			while (array[j] > array[low + 1]) j--;
			SwapT(&array[i], &array[j]);
		}
		SwapT(&array[i], &array[j]);
		SwapT(&array[low + 1], &array[j]);
		QuickSortFieldValT(array, low, j - 1);
		QuickSortFieldValT(array, j + 1, high);
	}
}

void ConditionalViewFrame::SortT(MapSortElement *i, MapSortElement *j,
								 MapSortElement *k)
{
	if (*i > *j) {
		SwapT (i, j);
		if (*j > *k) SwapT(j, k);
		if (*i > *j) SwapT(i, j);
	} else  {
		if (*j > *k) SwapT(j, k);
		if (*i > *j) SwapT(i, j);
	}
}

void ConditionalViewFrame::SwapT(MapSortElement *x, MapSortElement *y)
{
	MapSortElement temp(0, 0);
	temp= *x;
	*x = *y;
	*y = temp;
}

void ConditionalViewFrame::XCategory()
{ 
	int i;
	MapSortElement* fieldValue; 
	fieldValue    = new MapSortElement[num_obs+1];

	for(i=0;i<num_obs;i++) { 
		fieldValue[i].value = RawDataX[i]; 
		fieldValue[i].recordIndex = i;
	}
		  
	QuickSortFieldValT(fieldValue, 0, num_obs-1);

	bool bUnique = false;
  
	int index = 0;
	int numClasses = 1;
	float uval[5];
	uval[0] = fieldValue[0].value;
	for(index = 0; index<num_obs-1; index++)
	{
		if(fieldValue[index].value != fieldValue[index+1].value)
		{
			uval[numClasses] = fieldValue[index+1].value;
			numClasses++;
		}
		if(numClasses > 3)
			break;
	}

	if(numClasses == 2) {
		Xbin = 2;
	} else if(numClasses == 3) {
		Xbin = 3;
		bUnique = true;
	} else {
		Xbin = 3;
	}
	
	if(bUnique) {
		x1 = uval[0];
		x4 = uval[2];
		x2 = (uval[0]+uval[1])/2.0;
		x3 = (uval[1]+uval[2])/2.0;
	} else if(Xbin == 3) {
		x1 = xMin;
		x4 = xMax;
		x2 = (xMax-xMin)/3.0 + xMin;
		x3 = 2.0*(xMax-xMin)/3.0 + xMin;
	} else if(Xbin == 2) {
		x1 = xMin;
		x4 = xMax;
		x2 = (xMax-xMin)/2.0 + xMin;
		x3 = xMax;
	}

	delete [] fieldValue; fieldValue = NULL;
}

void ConditionalViewFrame::YCategory()
{
	int i;

	MapSortElement* fieldValue; 
	fieldValue    = new MapSortElement[num_obs+1];

	for(i=0;i<num_obs;i++) { 
		fieldValue[i].value = RawDataY[i]; 
		fieldValue[i].recordIndex = i;
	}

	QuickSortFieldValT(fieldValue, 0, num_obs-1);

	bool bUnique = false;

	int index = 0;
	int numClasses = 1;

	float uval[5];
	uval[0] = fieldValue[0].value;

	for(index = 0; index<num_obs-1; index++) {
		if(fieldValue[index].value != fieldValue[index+1].value) {
			uval[numClasses] = fieldValue[index+1].value;
			numClasses++;
		}

		if(numClasses > 3)
			break;
	}

	if(numClasses == 2) {
		Ybin = 2;
	} else if (numClasses == 3) {
		Ybin = 3;
		bUnique = true;
	} else {
		Ybin = 3;
	}

	if(bUnique) {
		y1 = uval[0];
		y4 = uval[2];
		y2 = (uval[0]+uval[1])/2.0;
		y3 = (uval[1]+uval[2])/2.0;

	} else if(Ybin == 3) {
		y1 = yMin;
		y4 = yMax;
		y2 = (yMax-yMin)/3.0 + yMin;
		y3 = 2.0*(yMax-yMin)/3.0 + yMin;
	} else if(Ybin == 2) {
		y1 = yMin;
		y4 = yMax;
		y2 = (yMax-yMin)/2.0 + yMin;
		y3 = yMax;
	}

	delete [] fieldValue; fieldValue = NULL;
}

ConditionalViewFrame::~ConditionalViewFrame()
{
	if (RawDataX) delete [] RawDataX; RawDataX = NULL;
	if (RawDataY) delete [] RawDataY; RawDataY = NULL;
	if (flags) delete [] flags; flags = NULL;
	my_children.DeleteObject(this);
}
 
void ConditionalViewFrame::UpdateViews()
{
	for (int i=0; i<num_obs; i++) {  
		int l,m;
		l=-1; m=-1;
		if (RawDataX[i] < x2) {
            l = 0;
		} else if ( (RawDataX[i] >= x2) && (RawDataX[i] <= x3) ) {
            l = 1; 
		} else {
            l = 2;
		}
 
		if( RawDataY[i] < y2 ) {
			m = 2;
		} else if ( (RawDataY[i] >= y2) && (RawDataY[i] <= y3) ) {
			m = 1;
		} else {
			m = 0;
		}
		
		
		if( (l < 0) || (m < 0) ) {
			flags[i] = 0;
		} else {
 		    flags[i] = l+m*3+1;
		}
	}

    MapMovieCanvas* view1;
    BoxPlotCanvas* view2;
    HistCanvas* view3;
    ScatterPlotCanvas* view4;
	
    switch(mViewType)
	{
		case 1:
            for (int i=0; i<9; i++) {
				view1 = (MapMovieCanvas*) win_array[i];
				view1->UpdateCondition(flags);
			}
            break;

		case 2:
            for (int i=0; i<9; i++) {
				view2 = (BoxPlotCanvas*) win_array[i];
				view2->UpdateCondition(flags);
			}
            break;

		case 3:
            for (int i=0; i<9; i++) {
				view3 = (HistCanvas*) win_array[i];
				view3->UpdateCondition(flags);
			}
            break;

		case 4:
            for (int i=0; i<9; i++) {
				view4 = (ScatterPlotCanvas*) win_array[i];
				view4->UpdateCondition(flags);
			}
            break;

		default:
			break;
	}
}

void ConditionalViewFrame::Update()
{
    MapMovieCanvas* view1;
    BoxPlotCanvas* view2;
    HistCanvas* view3;
    ScatterPlotCanvas* view4;

    switch(mViewType)
	{
		case 1:
            for(int i=0; i<9; i++) {
				view1 = (MapMovieCanvas*) win_array[i];
				wxClientDC dc(view1);
			    PrepareDC(dc);
				view1->Selection(&dc);
				view1->Refresh();
			}
            break;
		case 2:
            for(int i=0; i<9; i++) {
				view2 = (BoxPlotCanvas*) win_array[i];
				wxClientDC dc(view2);
			    PrepareDC(dc);
				view2->Selection(&dc);
			}
            break;
		case 3:
            for(int i=0; i<9; i++) {
				view3 = (HistCanvas*) win_array[i];
				wxClientDC dc(view3);
			    PrepareDC(dc);
				view3->Selection(&dc);
			}
            break;
		case 4:
            for(int i=0; i<9; i++) {
				view4 = (ScatterPlotCanvas*) win_array[i];
				wxClientDC dc(view4);
			    PrepareDC(dc);
				view4->Selection(&dc);
			}
            break;
		default:
			break;
	}
}

void ConditionalViewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ConditionalViewFrame::OnActivate");
	if (event.GetActive()) RegisterAsActive("ConditionalViewFrame", GetTitle());
}

void ConditionalViewFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In ConditionalViewFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void ConditionalViewFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In ConditionalViewFrame::OnMenuClose");
	Close();
}

void ConditionalViewFrame::MapMenus()
{
	LOG_MSG("In ConditionalViewFrame::MapMenus");
	//MMM: unfinished
}

BEGIN_EVENT_TABLE(ConditionalVariableXView, wxScrolledWindow)
    EVT_MOUSE_EVENTS(ConditionalVariableXView::OnEvent)
    EVT_SIZE(ConditionalVariableXView::OnSize)
END_EVENT_TABLE()


	

ConditionalVariableXView::ConditionalVariableXView(const wxString& varXname,
													 const double* xVals,
													 wxWindow *parent,
													 int num_obs_s,
													 const wxPoint& pos,
													 const wxSize& size)
	: TemplateCanvas(parent, pos, size), num_obs(num_obs_s)
{

	xMin = 1e20;
	xMax = -1e20;
	for(int i=0; i<num_obs; i++) {
		if(xMin > xVals[i])
			xMin = 	xVals[i];
		if(xMax < xVals[i])
			xMax = 	xVals[i];	
	}
	xrMax = xMax;
	xrMin = xMin;

	xMax = xMax + (xrMax - xrMin)/10000000.0;
	xMin = xMin - (xrMax - xrMin)/10000000.0;

	xx[0] = xMin;
	xx[3] = xMax;
	xx[1] = (xMax-xMin)/3.0 + xMin;
	xx[2] = 2.0*(xMax-xMin)/3.0 + xMin;
 

	varX = varXname;

    SetBackgroundColour(*wxWHITE);

	CheckSize();
    Refresh();
}

void ConditionalVariableXView::OnDraw(wxDC& dc)
{
    DrawAxes(&dc);
    DrawLegend(&dc);
}
void ConditionalVariableXView::SetCuts()
{
	pFrame->x1 = xx[0];
	pFrame->x2 = xx[1];
	pFrame->x3 = xx[2];
	pFrame->x4 = xx[3];

    pFrame->UpdateViews();
}

void ConditionalVariableXView::CheckSize()
{
	
	Bottom= 35;
	Top= 0; 	
	Left = 30 ;
	Right= 10;
	Width= 0;
	Height= 40;
	
	wxSize size2 = GetSize();
	
	int width = size2.x;
	int height = size2.y;
	
	int res = height - Top - Bottom-Height;
	if( res < 0) res =0;
	int rata = res/16;
	Top += rata;
	Bottom += rata;
	Height = height - Top - Bottom;
	res = width - Left - Right - Width;
	if(res<0) res = 0;
	rata = res/32;
	Left += rata;
	Right += 4*rata;
	Width = width - Left - Right;
	
	for(int i=0; i<4; i++) {
		coordinate_handle[i] = Left+Width*(xx[i]-xMin)/(xMax-xMin);
	}
}

void ConditionalVariableXView::DrawLegend(wxDC *pDC)
{
	char  buf[64];  
	double  scale= 1;
	
	int sz= (int)(0.25 * Left), szy= (int)(0.75 * Bottom);
	if (sz > 10) sz = 10;
	if (szy > 10) szy = 10;
	sz = (sz + szy) / 2;
	
	wxFont m_font(*wxSMALL_FONT);
	m_font.SetPointSize(sz);
	m_font.SetWeight(wxFONTWEIGHT_NORMAL); 
	pDC->SetFont(m_font);
	
	wxBrush brush;
	brush.SetColour(wxColour(255,255,255));
	pDC->SetBackground(brush);
	
	
	int len;
	if (sz > 5) {
		for(int i=0; i<4; i++) {
			if( (pFrame->Xbin == 2) && (i == 2)) continue;
			
			double a = xx[i];
			if (xx[i]<0) a = fabs(a);
			GenUtils::ggcvt(a, 4, buf);
			
			len = strlen(buf);
			if (buf[len-1] == '.')  buf[--len]= '\x0';
			
			if (xx[i] >=0)
				pDC->DrawText(wxString::Format("%s",buf),
							  (int)coordinate_handle[i],
							  (int)(Height-Bottom+15+sz*(i%2)));
			else
				pDC->DrawText(wxString::Format("-%s",buf),
							  (int)coordinate_handle[i],
							  (int)(Height-Bottom+15+sz*(i%2)));
			
		}
		
		m_font.SetPointSize((int)(sz));
		pDC->DrawText(varX, 20, (int)(Height-Bottom+18+2*sz));
	}
}

void ConditionalVariableXView::DrawAxes(wxDC *pDC)
{
	wxColour color(128,128,0);
	wxPen dPen;
	dPen.SetWidth(2);
	dPen.SetColour(color);
	pDC->SetPen(dPen);
	
	
	for(int i=0; i<4; i++) {
		if( (i>=1) && (i<=2) ) {
			if((pFrame->Xbin == 2) && (i == 2)) continue;
			
			pDC->DrawLine((int)coordinate_handle[i],
						  (int)(Height-Bottom+10-5*i),
						  (int)coordinate_handle[i], (int)Top);
			DrawCircles(pDC, (int)coordinate_handle[i],
						(int)(Height-Bottom+10-5*i), 3, wxColour(0,0,0));
		} else {
			pDC->DrawLine((int)coordinate_handle[i],
						  (int)(Height-Bottom+10),(int)coordinate_handle[i],
						  (int)Top);
		}
	}
}  

void ConditionalVariableXView::DragLinePlot(wxPoint point)
{
	if (gRegime == NO_SELECT) return;       
	ClearLineFrame();
	if (gRegime == LINE_SELECT) {                      
		gSelect1.x = point.x;
		gSelect1.y = Height-Bottom+10-5*sel;
		gSelect2.y = Top;
		gSelect2.x = gSelect1.x;
	}
	ClearLineFrame();    // draw the new one
}

inline void ConditionalVariableXView::ClearLineFrame()
{
    wxClientDC dc(this);
    PrepareDC(dc);

	wxColour color(79,79,79);
	wxPen dPen;
	dPen.SetWidth(2);
	dPen.SetColour(color);
	dc.SetPen(dPen);

	dc.SetLogicalFunction(wxINVERT);

	dc.DrawLine(gSelect1,gSelect2);
	DrawCircles(&dc, gSelect1.x, gSelect1.y, 3, wxColour(0,0,0));
}

int ConditionalVariableXView::CheckHandle(wxPoint p)
{
	for(int i=1; i<3; i++) {
		if( (pFrame->Xbin == 2) && (i == 2)) continue;

		double y = Height-Bottom+10-5*i;
		double const x = coordinate_handle[i];
		if (fabs(p.x-x) + fabs(p.y-y) <= 5.0) return i;
	}
	return -1;
}

inline void ConditionalVariableXView::DrawCircles(wxDC *s, const int x,
												   const int y,
												   const double circleSize,
												   const wxColour color)
{
	wxBrush brush;
	brush.SetColour(color);
	s->SetBrush(brush);
	s->DrawCircle((int)x, (int)y, (int)circleSize);
}

void ConditionalVariableXView::OnEvent(wxMouseEvent& event)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxPoint point(event.GetLogicalPosition(dc));

	if(event.LeftDown()) {

	    CaptureMouse();

	    if (gRegime == LINE_SELECT) ClearLineFrame();
		
	    gSelect1 = point;
		gSelect2= gSelect1;
	    gRegime= RECT_SELECT;      

		const int i = CheckHandle(gSelect1); 
		if (i!= -1) {
			 sel = i;
			 gRegime= LINE_SELECT;
		}	

	} else if(event.LeftUp()) {
		
		if (HasCapture()) ReleaseMouse();
		
		if (gRegime==LINE_SELECT){
	      ClearLineFrame();
		  gRegime = NO_SELECT;

		  if(point.x < coordinate_handle[sel-1])
			  point.x = (int)coordinate_handle[sel-1];	
	      if(point.x > coordinate_handle[sel+1])
		      point.x = (int)coordinate_handle[sel+1];

		  double res = (double) ((point.x - coordinate_handle[0])/
								 (coordinate_handle[3]-coordinate_handle[0]));

		  res *= (xMax-xMin);
		  xx[sel] = (res + xMin);
			
		  CheckSize();
		  SetCuts();
		  Refresh();
		}	
	} else if(event.Dragging()) {
		if (gRegime == LINE_SELECT) {
			if(point.x < coordinate_handle[sel-1])
			    point.x = (int)coordinate_handle[sel-1];	
			if(point.x > coordinate_handle[sel+1])
				point.x = (int)coordinate_handle[sel+1];

			DragLinePlot(point);
		}
	}
}

void ConditionalVariableXView::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
	event.Skip();
}

BEGIN_EVENT_TABLE(ConditionalVariableYView, wxScrolledWindow)
    EVT_MOUSE_EVENTS(ConditionalVariableYView::OnEvent)
    EVT_SIZE(ConditionalVariableYView::OnSize)
END_EVENT_TABLE()
	

ConditionalVariableYView::ConditionalVariableYView(const wxString& varYname,
													 const double* yVals,
													 wxWindow *parent,
													 int num_obs_s,
													 const wxPoint& pos,
													 const wxSize& size)
: TemplateCanvas(parent, pos, size), num_obs(num_obs_s)
{

	yMin = 1e20;
	yMax = -1e20;
	for(int i=0; i<num_obs; i++) {
		if(yMin > yVals[i]) yMin = yVals[i];
		if(yMax < yVals[i]) yMax = yVals[i];	
	}
	yrMax = yMax;
	yrMin = yMin;

	yMax = yMax + (yrMax - yrMin)/10000000.0;
	yMin = yMin - (yrMax - yrMin)/10000000.0;

	yy[3] = yMin;
	yy[0] = yMax;
	yy[2] = (yMax-yMin)/3.0 + yMin;
	yy[1] = 2.0*(yMax-yMin)/3.0 + yMin;
 
	varY = varYname;

    SetBackgroundColour(*wxWHITE);

	CheckSize();
    Refresh();
}

void ConditionalVariableYView::OnDraw(wxDC& dc)
{
    DrawAxes(&dc);
    DrawLegend(&dc);
}

void ConditionalVariableYView::SetCuts()
{
	pFrame->y1 = yy[3];
	pFrame->y2 = yy[2];
	pFrame->y3 = yy[1];
	pFrame->y4 = yy[0];

	pFrame->UpdateViews();

}

void ConditionalVariableYView::CheckSize()
{
	Left = 85;
	
	Right= 0;
	Top= 30; 
	Bottom= 10;
	Width= 40;
	Height= 0;
	
	
	wxSize size2 = GetSize();
	
	int width = size2.x;
	int height = size2.y;
	
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
	
	for(int i=0; i<4; i++) {
		coordinate_handle[i] = Top+Height*(yMax-yy[i])/(yMax-yMin);
	}
}

void ConditionalVariableYView::DrawLegend(wxDC *pDC)
{
	char  buf[64];  
	double    scale= 1;
	
	int sz= (int)(0.25 * Bottom), szy= (int)(0.25 * Left);
	if (sz > 10) sz = 10;
	if (szy > 10) szy = 10;
	
	sz = (sz + szy) / 2;
	
	wxFont m_font(*wxNORMAL_FONT);
	m_font.SetPointSize(sz);
	m_font.SetWeight(wxFONTWEIGHT_NORMAL); 
	pDC->SetFont(m_font);
	
	wxBrush brush;
	brush.SetColour(wxColour(255,255,255));
	pDC->SetBackground(brush);
	
	int len;
	if (sz > 5) {
		for(int i=0; i<4; i++) {
			if( (pFrame->Ybin == 2) && (i == 1)) continue;
			
			double a = yy[i];
			if (a <0) a = fabs(a);
			GenUtils::ggcvt(a, 4, buf);
			
			len = strlen(buf);
			if (buf[len-1] == '.')  buf[--len]= '\x0';
			if (yy[i]<0)
				pDC->DrawText(wxString::Format("-%s",buf), (int)(5),
							  (int)(coordinate_handle[i]-sz/2));
			else
				pDC->DrawText(wxString::Format("%s",buf), (int)(5),
							  (int)(coordinate_handle[i]-sz/2));
		}
		
		m_font.SetPointSize((int)(sz));
		pDC->DrawText(varY, 20, (int)(coordinate_handle[3]+2*sz));
		
	}
}

void ConditionalVariableYView::DrawAxes(wxDC *pDC)
{
	wxColour color(128,128,0);
	wxPen dPen;
	dPen.SetWidth(2);
	dPen.SetColour(color);
	pDC->SetPen(dPen);

	for(int i=0; i<4; i++) {
		if( (i>=1) && (i<=2) ) {
			if( (pFrame->Ybin == 2) && (i == 1)) continue;

			pDC->DrawLine((int)(Left-10+5*i),(int)coordinate_handle[i],
						  (int)(Left+Width), (int)coordinate_handle[i]);
			DrawCircles(pDC, (int)(Left-10+5*i), (int)coordinate_handle[i],
						3, wxColour(0,0,0));
		} else {
			pDC->DrawLine((int)(Left-10),(int)coordinate_handle[i],
						  (int)(Left+Width), (int)coordinate_handle[i]);
		}
	}
}

void ConditionalVariableYView::DragLinePlot(wxPoint point)
{
   if (gRegime == NO_SELECT) return;       
   ClearLineFrame();
   if (gRegime == LINE_SELECT) {                      
		gSelect1.y = point.y;
		gSelect1.x = Left-10+5*sel;
		gSelect2.x = Left+Width;
		gSelect2.y = gSelect1.y;
	}
	ClearLineFrame();                        // draw the new one
}

inline void ConditionalVariableYView::ClearLineFrame()
{
    wxClientDC dc(this);
    PrepareDC(dc);

	wxColour color(79,79,79);
	wxPen dPen;
	dPen.SetWidth(2);
	dPen.SetColour(color);
	dc.SetPen(dPen);

	dc.SetLogicalFunction(wxINVERT);

	dc.DrawLine(gSelect1,gSelect2);
	DrawCircles(&dc, gSelect1.x, gSelect1.y, 3, wxColour(0,0,0));
}

int ConditionalVariableYView::CheckHandle(wxPoint p)
{
	for(int i=1; i<3; i++) {
		if( (pFrame->Ybin == 2) && (i == 1)) continue;

		double x = Left - 10 + 5*i;
		double const y = coordinate_handle[i];

		if (fabs(p.x-x) + fabs(p.y-y) <= 5.0) return i;
	}
	return -1;
}

inline void ConditionalVariableYView::DrawCircles(wxDC *s,
												   const int x, const int y,
												   const double circleSize,
												   const wxColour color)
{
	wxBrush brush;
	brush.SetColour(color);
	s->SetBrush(brush);
	s->DrawCircle((int)x, (int)y, (int)circleSize);
}

void ConditionalVariableYView::OnEvent(wxMouseEvent& event)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxPoint point(event.GetLogicalPosition(dc));

	if(event.LeftDown())
	{
	    CaptureMouse();

		if (gRegime == LINE_SELECT)
		 {
			ClearLineFrame();
		 }
		 gSelect1 = point;
		 gSelect2= gSelect1;
		 gRegime= RECT_SELECT;  

		 const int i = CheckHandle(gSelect1); 
		 if (i!= -1)
		 {
			 sel = i;
			 gRegime= LINE_SELECT;
		 }

	}
	else if(event.LeftUp())
	{
		
		if (HasCapture()) ReleaseMouse();

		if (gRegime==LINE_SELECT)
		{

		  ClearLineFrame();
		  gRegime = NO_SELECT;


		  if(point.y < coordinate_handle[sel-1])
			  point.y = (int)coordinate_handle[sel-1];
		  if(point.y > coordinate_handle[sel+1])
			  point.y = (int)coordinate_handle[sel+1];

		  double res  =  (double) (coordinate_handle[3]-point.y) /
			( coordinate_handle[3] - coordinate_handle[0]);

		  res *= (yMax-yMin);
		  yy[sel] = (res + yMin);			
				
		  SetCuts();
		  CheckSize();
		  Refresh();
		}	
	}
	else if(event.Dragging())
	{
		if (gRegime == LINE_SELECT)
		{
			if(point.y < coordinate_handle[sel-1])
				point.y = (int)coordinate_handle[sel-1];
			if(point.y > coordinate_handle[sel+1])
				point.y = (int)coordinate_handle[sel+1];

			DragLinePlot(point);
			return;
		}
	}
}

void ConditionalVariableYView::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
	event.Skip();
}

BEGIN_EVENT_TABLE(ConditionalVariableNULLView, wxScrolledWindow)
	EVT_SIZE(ConditionalVariableNULLView::OnSize)
END_EVENT_TABLE()


ConditionalVariableNULLView::ConditionalVariableNULLView(wxWindow *parent,
														   const wxPoint& pos,
														   const wxSize& size)
          :TemplateCanvas(parent, pos, size)
{
	SetBackgroundColour(*wxWHITE);
}


BEGIN_EVENT_TABLE(ConditionalVariableHeaderView, wxScrolledWindow)
    EVT_SIZE(ConditionalVariableHeaderView::OnSize)
END_EVENT_TABLE()


ConditionalVariableHeaderView::ConditionalVariableHeaderView(
						wxWindow *parent, DbfGridTableBase* grid_base,
						double* v3, const wxString& v3_name,
						const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size), num_obs(grid_base->GetNumberRows())
{
	vMin = 1e20;
	vMax = -1e20;

	for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
		if (vMin > v3[i]) vMin = v3[i];
		if (vMax < v3[i]) vMax = v3[i];	
	}

	vv[0] = vMin;
	vv[3] = vMax;
	vv[1] = (vMax-vMin)/3.0 + vMin;
	vv[2] = 2.0*(vMax-vMin)/3.0 + vMin;

	var = v3_name;

    SetBackgroundColour(*wxWHITE);

	CheckSize();
    Refresh();
}    

void ConditionalVariableHeaderView::CheckSize()
{

	Bottom= 35;
	Top= 0; 	
	Left = 30 ;
	Right= 10;
	Width= 0;
	Height= 40;

	wxSize size2 = GetClientSize();

	int width = size2.x;
	int height = size2.y;

	int res = height - Top - Bottom-Height;
	if( res < 0) res =0;
	int rata = res/16;
	Top += rata;
	Bottom += rata;
	Height = height - Top - Bottom;
	res = width - Left - Right - Width;
	if(res<0) res = 0;
	rata = res/32;
	Left += rata;
	Right += 4*rata;
	Width = width - Left - Right;

	for(int i=0; i<4; i++) {
		coordinate_handle[i] = (int)(Left+Width*(vv[i]-vMin)/(vMax-vMin));
	}
}

void ConditionalVariableHeaderView::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
	event.Skip();
}


void ConditionalVariableHeaderView::DrawLegend(wxDC *pDC)
{
	double scale= 1;
	
	int sz= (int)(0.25 * Left), szy= (int)(0.75 * Bottom); 
	
	if (sz > 10) sz = 10;
	if (szy > 10) szy = 10;
	
	sz = (sz + szy) / 2;
	
	wxFont m_font(*wxNORMAL_FONT);
	m_font.SetPointSize(sz);
	m_font.SetWeight(wxFONTWEIGHT_NORMAL); 
	pDC->SetFont(m_font);
	
	wxBrush brush;
	brush.SetColour(wxColour(255,255,255));
	pDC->SetBackground(brush);
	
	if (sz > 5) {
		for(int i=0; i<4; i++) {
			if(i==1 || i==2) continue;
			pDC->DrawText(wxString::Format("%.5g", vv[i]),
						  (int)coordinate_handle[i],
						  (int)(Height-Bottom+32+sz*(i%2)));			
		}
		
		m_font.SetPointSize((int)(sz));
		pDC->SetFont(m_font);
		
		pDC->DrawText(var, 20, Height-Bottom+22+2*sz);
		
	}
}

void ConditionalVariableHeaderView::DrawAxes(wxDC *pDC)
{
	wxColour color(128,128,0);
	
	wxPen dPen;
	dPen.SetWidth(2);
	dPen.SetColour(color);
	pDC->SetPen(dPen);
	
	double len = coordinate_handle[3] - coordinate_handle[0];
	for(int j=(int)coordinate_handle[0]; j<(int)coordinate_handle[3]; j++) {
	    float col[3];
		
		convert_rgb(((int)(220.0-220.0*(j-coordinate_handle[0])/len))%360,
					(float) 0.7, (float) 0.7, col);
	    wxColour color = wxColour((int)(col[0]*255),
								  (int)(col[1]*255),
								  (int)(col[2]*255));
		
        dPen.SetWidth(1);
		dPen.SetColour(color);
		pDC->SetPen(dPen);
		
		pDC->DrawLine(j,Height-Bottom+30,j,Top);
	}
	
}

void ConditionalVariableHeaderView::OnDraw(wxDC& dc)
{
    DrawAxes(&dc);
    DrawLegend(&dc);
}
