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

#include <wx/utils.h> // for wxGetMouseState()
#include <wx/stopwatch.h>
#include <wx/dcbuffer.h> // for IMPLEMENT_DYNAMIC_CLASS
#include <wx/dcgraph.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces
#include <wx/sizer.h>
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../GeneralWxUtils.h"
#include "../GeoDaConst.h"
#include "../logger.h"
#include "ScatterPlotView.h"

extern Selection gSelection;
extern GeoDaEventType gEvent;
extern MyFrame *frame;

BEGIN_EVENT_TABLE(ScatterPlotFrame, wxFrame)
	EVT_ACTIVATE(ScatterPlotFrame::OnActivate)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ScatterPlotCanvas, wxScrolledWindow)
    EVT_SIZE(ScatterPlotCanvas::OnSize)
	EVT_PAINT(ScatterPlotCanvas::OnMyPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(ScatterPlotCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(ScatterPlotCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

// ---------------------------------------------------------------------------
// ScatterPlot Frame
// ---------------------------------------------------------------------------

ScatterPlotFrame::ScatterPlotFrame(wxFrame *parent,
								   double* v1, double* v2, int num_obs,
								   const wxString& v1_name,
								   const wxString& v2_name,
								   Project* project,
								   const wxString& title,
								   const wxPoint& pos, const wxSize& size,
								   const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
	my_children.Append(this);
	int width, height;
	GetClientSize(&width, &height);
	template_canvas = new ScatterPlotCanvas(this, v1, v2, num_obs,
											v1_name, v2_name, wxPoint(0, 0),
											wxSize(width, height));
	template_canvas->template_frame = this;
	
	Show(true);
}

ScatterPlotFrame::~ScatterPlotFrame()
{
	LOG_MSG("In ScatterPlotFrame::~ScatterPlotFrame");
	DeregisterAsActive();
	my_children.DeleteObject(this);
}

void ScatterPlotFrame::Update()
{
	LOG_MSG("Entering ScatterPlotFrame::Update()");
	wxClientDC cdc(template_canvas);
	wxBufferedDC dc(&cdc);
    //PrepareDC(dc);
	((ScatterPlotCanvas*) template_canvas)->Selection(&dc);
	LOG_MSG("Exiting ScatterPlotFrame::Update()");
}

void ScatterPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ScatterPlotFrame::OnActivate");
	template_canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("ScatterPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        ((ScatterPlotCanvas*) template_canvas)->SetFocus();
}

void ScatterPlotFrame::MapMenus()
{
	LOG_MSG("In ScatterPlotFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
						LoadMenu("ID_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
}

void ScatterPlotFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	((ScatterPlotCanvas*) template_canvas)->ViewStandardizedData();
}

void ScatterPlotFrame::OnViewOriginalData(wxCommandEvent& event)
{
	((ScatterPlotCanvas*) template_canvas)->ViewOriginalData();
}

void ScatterPlotFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	((ScatterPlotCanvas*) template_canvas)->ViewRegressionSelectedExcluded();
}


// ---------------------------------------------------------------------------
// ScatterPlot Canvas 
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
ScatterPlotCanvas::ScatterPlotCanvas(wxWindow *parent,
									 double* v1, double* v2, int num_obs_s,
									 const wxString& v1_name,
									 const wxString& v2_name,
									 const wxPoint& pos,
									 const wxSize& size, bool conditional_view)
: TemplateCanvas(parent, pos, size), Conditionable(conditional_view, num_obs_s),
num_obs(num_obs_s)
{
	flick = true;
	oldSlope = 0.0;
	oldIntercept = 0.0;

	location.resize(num_obs);
	
	RawData = new DataPoint[num_obs];
	Horizontal = v1_name;
	Vertical = v2_name;

	SpbackColor = wxColour(255,255,255);

	for (int i=0; i < num_obs; i++) {
		RawData[i] = DataPoint(v1[i], v2[i]);
	}
	
	regressionUnselected = false;
	excludeSelected = false;
	symmetricflag = true;

	Init();
    //SetBackgroundColour(*wxWHITE);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
    CheckSize();
    Refresh();
}

ScatterPlotCanvas::~ScatterPlotCanvas()
{
	if (RawData) delete [] RawData; RawData = 0;
}

// Redefine the wxScrolledWindow::OnDraw repainting behaviour
void ScatterPlotCanvas::OnDraw(wxDC& dc)
{
	//selection_outline_visible = false;
	//Draw(&dc);
}

void ScatterPlotCanvas::OnMyPaint(wxPaintEvent& event)
{
	LOG_MSG("Entering ScatterPlotCanvas::OnMyPaint");
	
	wxAutoBufferedPaintDC dc(this);
	//wxBufferedPaintDC dc(this);
	//DoPrepareDC(dc);  // We certainly need to call this for Mac otherwise
	// the image doesn't redraw when scrolling.
	//LOG_MSG("Painting");
	selection_outline_visible = false;
	
	Draw(&dc);
	
	LOG_MSG("Exiting ScatterPlotCanvas::OnMyPaint");
}

void ScatterPlotCanvas::OnEvent(wxMouseEvent& event)
{
	if (isConditional) return;

	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if (event.RightUp()) {
	    PopupMenu(wxXmlResource::Get()->
					LoadMenu("ID_SCATTER_PLOT_VIEW_MENU_CONTEXT"),
				  event.GetPosition().x, event.GetPosition().y);
		return;
	}

	TemplateCanvas::OnEvent(event);
	event.Skip();
}

void ScatterPlotCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void ScatterPlotCanvas::CheckSize()
{
	wxSize size2 = GetClientSize();;
	
	int width = size2.x;
	int height = size2.y;
	
    Left = 10;  Right = 10;
    Top= 15;   Bottom = 15;
    Width = 40;
    Height = 40;
	
    int res= width - Left-Right-Width;
    if (res < 0) res = 0;
    int rata = res / 16;
    Left += rata;
    Right += rata;
    Width = width - Left - Right;
    res = height - Top - Bottom - Height;
    if (res < 0) res= 0;
    rata = res / 32;
    Top += rata;
    Bottom += 4*rata;
    Height = height - Top - Bottom;
	
    xDensity= Width/(xMax-xMin);
    yDensity= Height/(yMax-yMin);
    for (int cnt= 0; cnt < num_obs; ++cnt) {
		location[cnt].x =
			(long) ((RawData[cnt].horizontal - xMin) * xDensity + Left);
		location[cnt].y =
			(long) (Top+Height- (RawData[cnt].vertical - yMin) * yDensity);
	}
    starSize = (int) (log10((double)Width+(double)Height)
					  - log10((double)num_obs)
					  + ((double)Width+(double)Height)/256);
	if (starSize < 0) 
		starSize = 0;
	else if (starSize > 4)
		starSize = 4;
}

void ScatterPlotCanvas::OnSize(wxSizeEvent& event)
{
	wxStopWatch sw;
    CheckSize();
    //Refresh();
    //event.Skip();
	LOG_MSG(wxString::Format("ScatterPlotCanvas::OnSize took %ld ms",
							 (long) sw.Time()));
}

inline double _fraction(const double x) 
{
    return x - floor(x);
}

int Mesh(double & rMin, double & rMax, int lower= 3, int upper= 6)  {
  const double  log2= log10((double)2);
  if (lower < 2) lower= 2;
  if (upper > 20) upper= 20;
  while (lower >= upper)  {
    lower >>= 1;            // double the value
    upper <<= 1;            // cut in half ( minus the remainder)
  };
  int Units= lower;
  double unit, LogRange= log10(rMax-rMin), power;
  do  {
    if (Units < lower) unit += log2;
      else if (Units > upper) unit -= log2;
        else unit= 0;
    power= _fraction (LogRange + unit );
    Units= (int) ceil( pow(10, power) );
  }
  while ( Units < lower || Units > upper);
  power= floor(LogRange+unit) - unit;
  unit = pow(10, power);
  int steps= (int) ceil(rMax / unit);
  rMax= steps * unit;
  steps= (int) floor(rMin / unit);
  rMin= steps * unit;
  Units = (int) floor( (rMax - rMin) / unit + 0.5);
  return Units;
}


void ScatterPlotCanvas::BuildMesh(const bool asymmetric)  
{
    int cnt;
    if (asymmetric) {
		xMin= xMax= RawData[0].horizontal;
		yMin= yMax= RawData[0].vertical;
		for (cnt= 0; cnt < num_obs; ++cnt)  {
			if (xMin > RawData[cnt].horizontal) xMin= RawData[cnt].horizontal;
			else if (xMax <RawData[cnt].horizontal) xMax=RawData[cnt].horizontal;
			if (yMin > RawData[cnt].vertical) yMin= RawData[cnt].vertical;
			else if (yMax < RawData[cnt].vertical) yMax= RawData[cnt].vertical;
		}
		if (xMin==xMax) xUnits = 4;
		else xUnits= Mesh(xMin, xMax);
		if (yMin==yMax) {
			yUnits = 4;
			yMin = yMax/2;
			yMax = yMax+yMin;
		}
		else yUnits= Mesh(yMin, yMax);
		
    } else {
		double   min, max;
		min= max= RawData[0].horizontal;
		for (cnt= 0; cnt < num_obs; ++cnt)  
		{
			if (min > RawData[cnt].horizontal) min= RawData[cnt].horizontal;
			else if (max < RawData[cnt].horizontal) max= RawData[cnt].horizontal;
			if (min > RawData[cnt].vertical) min= RawData[cnt].vertical;
			else if (max < RawData[cnt].vertical) max= RawData[cnt].vertical;
		}
		for (cnt= 0; cnt < num_obs; ++cnt) {
			if (min > -RawData[cnt].horizontal) min= -RawData[cnt].horizontal;
			else if (max < -RawData[cnt].horizontal) max=-RawData[cnt].horizontal;
			if (min > RawData[cnt].vertical) min= RawData[cnt].vertical;
			else if (max < RawData[cnt].vertical) max= RawData[cnt].vertical;
		}
		xMin= yMin= min;
		xMax= yMax= max;
		
		if (xMin==xMax || yMin==yMax) {
			xUnits = yUnits = 4;
		} else {
			xUnits= yUnits= Mesh(min, max);
		}
    }
}


bool ScatterPlotCanvas::Init()  
{
	Standardized();
	Destandardized();
	StandardizedFlag = false;
	BuildMesh(true);
	ComputeRegression( true );
	slopeX = slope;
    
	return true;
}

void ScatterPlotCanvas::Standardized() {
	int cnt;
	double sum = 0;

	// Standardized X
	for (cnt = 0; cnt < num_obs; ++cnt) {
		sum += RawData[cnt].horizontal;
	}
	meanX = sum / num_obs;
	if (meanX != 0) { // remove the mean
		for (cnt = 0; cnt < num_obs; ++cnt)
			RawData[cnt].horizontal -= meanX;
	}
	sum = 0; // accumulate the sum of squares
	for (cnt = 0; cnt < num_obs; ++cnt)
		sum += geoda_sqr(RawData[cnt].horizontal);
	sdevX = sqrt(sum / (num_obs - 1)); // std. deviation
	if (sdevX != 0)
		for (cnt = 0; cnt < num_obs; ++cnt)
			RawData[cnt].horizontal /= sdevX;

	// Standardized Y
	sum = 0;
	for (cnt = 0; cnt < num_obs; ++cnt) {
		sum += RawData[cnt].vertical;
	}
	meanY = sum / num_obs;
	if (meanY != 0) { // remove the mean
		for (cnt = 0; cnt < num_obs; ++cnt)
			RawData[cnt].vertical -= meanY;
	}
	sum = 0; // accumulate the sum of squares
	for (cnt = 0; cnt < num_obs; ++cnt)
		sum += geoda_sqr(RawData[cnt].vertical);
	sdevY = sqrt(sum / (num_obs - 1)); // std. deviation
	if (sdevY != 0)
		for (cnt = 0; cnt < num_obs; ++cnt)
			RawData[cnt].vertical /= sdevY;

	StandardizedFlag = true;
}

void ScatterPlotCanvas::Destandardized()
{
	for (int cnt=0; cnt < num_obs; cnt++)
	{
		RawData[cnt].vertical = RawData[cnt].vertical * sdevY + meanY; 
		RawData[cnt].horizontal = RawData[cnt].horizontal * sdevX + meanX;
	}
	StandardizedFlag = false;
}

void ScatterPlotCanvas::Draw(wxDC* pDC)
{
	LOG_MSG("Entering ScatterPlotCanvas::Draw");
	PaintBackground(*pDC);
	DrawLegend(pDC);
	DrawAllPoints(pDC);
	DrawAxes(pDC);
	SlopeReport(pDC, "Slope ");
	LOG_MSG("Exiting ScatterPlotCanvas::Draw");
}


void ScatterPlotCanvas::DrawLegend(wxDC* dc)
{
	dc->SetTextBackground(canvas_background_color);
	dc->SetTextForeground(*wxBLACK);
	char  buf[64];
	double    scale= 1;

	int       sz= 7, szy=7, len;

	sz= max(6, Bottom/8 + Width/xUnits/16 + 1);
	double    dval= (xMax-xMin)/xUnits/scale;
	if (sz > 15) sz = 15;
	if (sz > 5) {
		wxFont m_font(*wxNORMAL_FONT);
		m_font.SetPointSize(max(sz,6));
		dc->SetFont(m_font);

		wxString xLabel = Horizontal;

		if (isGreater(1000000, 1)) {
			scale= 1000000;  xLabel = Horizontal + " (in 10^6)";
		} else if (isGreater(1000, 1)) {
			scale = 1000;  xLabel = Horizontal + " (in 1000)";
		} else if (isLess(0.001, 1)) {
			scale= 0.001; xLabel = Horizontal + " (in 0.001)";
		}

		len = xLabel.Length();
		dc->DrawText(xLabel, Left+Width/2-len*sz/8, Top+Height+Bottom/3+4);

		double val= xMin/scale;
		dval= (xMax-xMin)/xUnits/scale;
		for (int cnt= 0; cnt <= xUnits; ++cnt)  {
			if (fabs(val) < 1e-15) val = 0;
			//_gcvt(val, 8, buf);
			GenUtils::ggcvt(val, 8, buf);

			sprintf ( buf, "%.*f", 1, val );

			len= strlen(buf);
			if (buf[len-1] == '.')  buf[--len]= '\x0';
			dc->DrawText(wxString::Format("%s",buf),
						 Left+Width*cnt/xUnits-len*sz/6,
						 Top+Height+1+Bottom/16);
			val += dval;
		}
	}

	szy = max(6, Left/8 + Height/yUnits/16 + 1);
	if (szy > 15) szy = 15;
	if (szy > 5) {
		wxFont m_font2(*wxNORMAL_FONT);
		m_font2.SetPointSize(max(szy,6));
		dc->SetFont(m_font2);

        wxString yLabel = Vertical;
		scale= 1;
		if (isGreater(1000000, 0)) {
			scale= 1000000;
			yLabel = Vertical + " (in 10^6)";
		} else if (isGreater(1000, 0)) {
			scale = 1000;
			yLabel = Vertical + " (in 1000)";
		} else if (isLess(0.001, 0)) {
			scale= 0.001;
			yLabel = Vertical + " (in 0.001)";
		}

		dval= (yMax-yMin)/yUnits/scale;

		len= yLabel.Length();
		dc->DrawRotatedText(yLabel, Left/8-2, Top+Height/2+len*szy/6, 90);

		double val= yMin/scale + dval;
		for (int cnt= 1; cnt < yUnits; ++cnt)
		{
			if (fabs(val) < 1e-15) val = 0;
			//_gcvt(val, 8, buf);
			GenUtils::ggcvt(val, 8, buf);

			sprintf ( buf, "%.*f", 1, val );

			len= strlen(buf);
			if (buf[len-1] == '.')  buf[--len]= '\x0';
			dc->DrawRotatedText(wxString::Format("%s",buf),
								Left-Left/8-szy-2,
								Top+Height*(yUnits-cnt)/yUnits+len*sz/6,90);
			val += dval;
		}
	}
}

inline void ScatterPlotCanvas::DrawAllPoints(wxDC* pDC)  
{
	wxStopWatch sw;
	wxPen unsel_pen;
	unsel_pen.SetColour(GeoDaConst::textColor);
	wxPen sel_pen;
	sel_pen.SetColour(*wxRED);
	
	for (int cnt=0; cnt<num_obs; cnt++) {
		if (!conditionFlag[cnt]) continue;
		
		if (gSelection.selected(cnt)) {
			GenUtils::DrawSmallCirc(pDC, (int)(location[cnt].x),
									(int)(location[cnt].y), starSize,
									GeoDaConst::highlight_color);
		} else {
			GenUtils::DrawSmallCirc(pDC, (int)(location[cnt].x),
									(int)(location[cnt].y), starSize,
									GeoDaConst::textColor);			
		}
	}
	LOG_MSG(wxString::Format("ScatterPlotCanvas::DrawAllPoints took %ld ms"
							 " using wxDC", (long) sw.Time()));
}

void ScatterPlotCanvas::DrawAxes(wxDC* dc)
{
	double steps;
	int coord;

	BPbackColor = canvas_background_color;
	if (yMin < 0 && yMax > 0) {

		steps = -yMin / (yMax - yMin);
		coord = (int) (Top + Height * (1.0 - steps));
		dc->DrawLine(Left, coord, Left + Width, coord);
	}
	if (xMin < 0 && xMax > 0) {
		steps = -xMin / (xMax - xMin);
		coord = (int) (Left + Width * steps);
		dc->DrawLine(coord, Top, coord, Top + Height);
	}

	if (hasRegression == REGRESS_LINEAR) {
		DrawRegression(dc, slope, intercept, GeoDaConst::outliers_colour, wxSOLID);
		if (excludeSelected && regressionUnselected) {
			DrawRegression(dc, oldSlope, oldIntercept, BPbackColor, wxSOLID);
			DrawRegression(dc, slopeX, interceptX, GeoDaConst::textColor, wxSOLID);
		}
	}

	wxPen pen;
	pen.SetColour(GeoDaConst::textColor);
	dc->SetPen(pen);

	dc->DrawLine(Left, Top, Left, Top + Height);
	dc->DrawLine(Left, Top + Height, Left + Width, Top + Height);
	dc->DrawLine(Left + Width, Top + Height, Left + Width, Top);
	dc->DrawLine(Left + Width, Top, Left, Top);

	int posX, posY, len = Bottom / 16 + 1, cnt;
	for (cnt = 0; cnt <= xUnits; ++cnt) {
		posX = Left + cnt * Width / xUnits;
		dc->DrawLine(posX, Top + Height, posX, Top + Height + len);
	}
	len = Left / 16 + 1;
	for (cnt = 1; cnt <= yUnits; ++cnt) {
		posY = Top + Height - cnt * Height / yUnits;
		dc->DrawLine(Left, posY, Left - len, posY);
	}

}

void ScatterPlotCanvas::DrawRegression(wxDC* dc, const double rSlope,
									   const double rIntercept, wxColour color,
									   int PS)
{
	wxPen pen;
	pen.SetColour(color);
	pen.SetWidth((starSize >> 2)+1);
	dc->SetPen(pen);

    int x1, y1, x2, y2;
    double  y;
    if (hasRegression == REGRESS_NO) return;
	
	y= rIntercept + rSlope * xMax;
    if (y <= yMax && y >= yMin)  {      // crossing right vertical border
      x1= Left + Width;
      y1= (int)(Top + (yMax-y)/(yMax-yMin)*Height);
    }  else  if (y > yMax)  {           // crossing upper horizontal border
      x1= (int)(Left + ((yMax-rIntercept)/rSlope-xMin)/(xMax-xMin)*Width);
      y1= Top;
    }  else                 {            // crossing lower horizontal border
      x1= (int)(Left + ((yMin-rIntercept)/rSlope-xMin)/(xMax-xMin)*Width);
      y1= Top+Height;
    }
    y= rIntercept + rSlope * xMin;
    if (y >= yMin && y <= yMax)  {      // crossing left vertical border
      x2= Left;
      y2= (int)(Top + (yMax-y)/(yMax-yMin)*Height);
    }  else if (y > yMax)  {            // crossing upper horizontal border
      x2= (int)(Left + ((yMax-rIntercept)/rSlope-xMin)/(xMax-xMin)*Width);
      y2= Top;
    }  else  {                          // crossing horizontal lower border
      x2= (int)(Left + ((yMin-rIntercept)/rSlope-xMin)/(xMax-xMin)*Width);
      y2= Top + Height;
    }
    dc->DrawLine(x1, y1, x2, y2);
}


void ScatterPlotCanvas::SlopeReport(wxDC* dc,const char * header)  
{
	int sz= max(6,Left/8 + (Height/yUnits/16 + Width/xUnits/16)/2 + 1);
	if (sz > 15) sz = 15;
	
	wxString buf;
	if (sz > 5) {
		// report Moran's I for the whole data set
		dc->SetTextForeground(GeoDaConst::textColor );
		
		wxFont m_font(*wxNORMAL_FONT);
		m_font.SetPointSize(max(sz,6));
		
		dc->SetFont(m_font);
		dc->SetPen(canvas_background_color);
		dc->SetBrush(canvas_background_color);
		dc->DrawRectangle(Left, Top/8-1,Width, sz+5);
		
		char bf[40];
		
		const int bufLen = sprintf(bf, "%s= %-7.4f %s", header, slope,"       ");
		int displ= regressionUnselected ? 0 : Width/2-bufLen*sz/4;
		
		dc->SetTextForeground(GeoDaConst::outliers_colour );
		buf.Printf("                   ");
		dc->DrawText(buf, Left+displ, Top/8-1);
		buf.Printf("%s= %7.4f         ", header, slope);
		dc->DrawText(buf, Left+displ, Top/8-1);
	}
	
	if (regressionUnselected) {
		// report MI for a subsample
		dc->SetTextForeground(GeoDaConst::textColor );
		wxFont m_font(*wxNORMAL_FONT);
		m_font.SetPointSize(max(sz,6));
		dc->SetFont(m_font);
		buf.Printf(wxT(" (%7.4f) " ), slopeX);
		dc->DrawText(buf, 10+Left+Width/2, Top/8-1);
	}
	
	if (gcObs < num_obs) {
		wxString text = wxString::Format("(%d)", (int) gcObs);
		dc->SetFont(*GeoDaConst::small_font);
		dc->SetTextForeground(*wxBLACK);
		int w, h;
		GetClientSize(&w, &h);
		dc->DrawText(text, w-35, 2);
	}	
}


void ScatterPlotCanvas::SelectByPoint(wxMouseEvent& event)  
{
	int  Id=GeoDaConst::EMPTY, distId= GeoDaConst::EMPTY, dist;
	for (int cnt= 0; cnt < num_obs; ++cnt) {
		if (fabs(location[cnt].x - gSelect1.x) < 3 &&
			fabs(location[cnt].y - gSelect2.y) < 3)
		{
			dist= (int)(geoda_sqr( location[cnt].x-gSelect1.x) +
			geoda_sqr( location[cnt].y-gSelect1.y));
			if (Id == GeoDaConst::EMPTY || distId > dist) {
				Id= cnt;  
				distId= dist;
			}
		}
	}

	if (Id == GeoDaConst::EMPTY) {  
		EmptyClick();
		gSelection.Reset(true);
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
		return;  
	}
	
	bool  isSelected = gSelection.selected(Id);
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

int ScatterPlotCanvas::SelectByRect(wxMouseEvent& event)  
{
	BasePoint   p1, p2;
	if (gSelect1.x <= gSelect2.x) {
		p1.x= gSelect1.x;  p2.x= gSelect2.x;
	}  else  {
		p2.x= gSelect1.x;  p1.x= gSelect2.x;
	}
	if (gSelect1.y <= gSelect2.y)  {
		p1.y= gSelect1.y;  p2.y= gSelect2.y;
	}  else  {
		p2.y= gSelect1.y;  p1.y= gSelect2.y;
	}

	gEvent = (event.ShiftDown()) ? ADD_SELECTION : NEW_SELECTION;

	int mCnt= 0;
	for (int cnt= 0; cnt < num_obs; ++cnt) {
		if (location[cnt].x >= p1.x && location[cnt].x <= p2.x &&
			location[cnt].y >= p1.y && location[cnt].y <= p2.y &&
			(gEvent == NEW_SELECTION || !gSelection.selected(cnt)))  
		{
			gSelection.Push( cnt );
			++mCnt;
		}
	}

	if (mCnt == 0) {
		gEvent = NEW_SELECTION;
	} else {
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
		gSelection.Reset(true);
	}
	
	return mCnt;
}

void ScatterPlotCanvas::Selection(wxDC* dc)  
{
	LOG_MSG("Entering ScatterPlotCanvas::Selection");
	wxStopWatch sw;
	wxPen unsel_pen;
	unsel_pen.SetColour(GeoDaConst::textColor);
	wxPen sel_pen;
	sel_pen.SetColour(*wxRED);
		
	int cnt;
	bool regressUpdate = excludeSelected && (hasRegression == REGRESS_LINEAR);
	switch(gEvent) {
		case NEW_SELECTION :
		{
			Draw(dc);
		}
			break;
		case ADD_SELECTION :
		{
			cnt = gSelection.Pop();
			if (cnt == GeoDaConst::EMPTY) regressUpdate= false;
			while (cnt != GeoDaConst::EMPTY) {
				if (!conditionFlag[cnt]) continue;
				GenUtils::DrawSmallCirc(dc,(int)(location[cnt].x),
										(int)(location[cnt].y), starSize,
										GeoDaConst::highlight_color);
				cnt = gSelection.Pop();
			}
		}
			break;
		case DEL_SELECTION :
		{
			while ((cnt= gSelection.Pop()) != GeoDaConst::EMPTY) {
				if (!conditionFlag[cnt]) continue;
				GenUtils::DrawSmallCirc(dc,(int)(location[cnt].x),
										(int)(location[cnt].y), starSize,
										GeoDaConst::textColor);
			}
		}
			break;
		default :
			break;
	}
	
	if (regressUpdate) {
		const bool saveRegressionUnselected= regressionUnselected;
		ComputeRegression( false );     // compute regression on unselected obs
		if (flick && (regressionUnselected ||
					  regressionUnselected != saveRegressionUnselected)) {
			bool sel_curr_shown = selection_outline_visible;
			selection_outline_visible = false;
			LOG_MSG("Calling Draw from within ScatterPlotCanvas::Selection");
			dc->Clear();
			Draw(dc);
			LOG_MSG("Called Draw from within ScatterPlotCanvas::Selection");
			if (sel_curr_shown) DrawSelectionOutline();
		}
	}
	gSelection.Reset();
	
	LOG_MSG(wxString::Format("ScatterPlotCanvas::Selection took %ld ms",
							 (long) sw.Time()));
	LOG_MSG("Exiting ScatterPlotCanvas::Selection");
}

inline bool ScatterPlotCanvas::isGreater(const double standard,
										 const bool checkX)
{
  if (checkX)
      return  (xMin == 0 || fabs(xMin) >= standard) &&
              (xMax == 0 || fabs(xMax) >= standard);
    else
      return  (yMin == 0 || fabs(yMin) >= standard) &&
              (yMax == 0 || fabs(yMax) >= standard);
}

inline bool ScatterPlotCanvas::isLess(const double standard, const bool checkX)
{
  if (checkX)
      return (fabs(xMin) <= standard && fabs(xMax) <= standard);
    else
      return (fabs(xMin) <= standard && fabs(xMax) <= standard);
}


void ScatterPlotCanvas::ComputeRegression( const bool all )
{

	double    sumX= 0, sumY= 0, sumX2= 0, sumXY= 0;
	int       sampleSize = 0;

	flick = true;
	if (!isConditional) {
		gcObs = num_obs;
		for (int cnt=0; cnt < num_obs; ++cnt) {
			if (all || !gSelection.selected(cnt)) {
				sumX += RawData[cnt].horizontal;
				sumY += RawData[cnt].vertical;
				sumX2 += geoda_sqr(RawData[cnt].horizontal);
				sumXY += RawData[cnt].horizontal * RawData[cnt].vertical;
				++sampleSize;
			}
		}
		
		const double denominator = sampleSize ? sumX2-geoda_sqr(sumX)/sampleSize : 0;
	
		if (fabs(denominator) < 1.0e-12) {
			if (all) {
				hasRegression = REGRESS_NO;
			} else {
				regressionUnselected= false;
			}
		} else {
			const double newSlope =
				(sumXY - sumX * sumY / sampleSize) / denominator;
			const double  newIntercept=
				(sumY - newSlope * sumX)/sampleSize;
			if ((newSlope == oldSlope) & (newIntercept == oldIntercept)) {
				flick = false;
			}
			if (all) {
				hasRegression= REGRESS_LINEAR;
				slope= newSlope;
				slopeX = slope;
				intercept= newIntercept;
				interceptX = intercept;
			} else {
				oldSlope = slopeX;
				oldIntercept = interceptX;
				regressionUnselected= true;
				slopeX= newSlope;
				interceptX = newIntercept;
			}
		}
	} else {
		gcObs = 0;
		for (int cnt=0; cnt < num_obs; ++cnt) {
			if(!conditionFlag[cnt]) continue;
			gcObs++;
			if (all || !gSelection.selected(cnt)) {
				sumX += RawData[cnt].horizontal;
				sumY += RawData[cnt].vertical;
				sumX2 += geoda_sqr(RawData[cnt].horizontal);
				sumXY += RawData[cnt].horizontal * RawData[cnt].vertical;
				++sampleSize;
			}
		}

		slope = 0.0;
		slopeX = 0.0;
		const double denominator = sampleSize ? sumX2 - geoda_sqr(sumX)/sampleSize : 0;

		if (fabs(denominator) < 1.0e-12) {
			if (all) {
				hasRegression = REGRESS_NO;
			} else {
				regressionUnselected = false;
			}
		} else {
			const double  newSlope=
				(sumXY - sumX * sumY / sampleSize) / denominator;
			const double  newIntercept= (sumY - newSlope * sumX)/sampleSize;
			if ((newSlope == oldSlope) && (newIntercept == oldIntercept)) {
				flick = false;
			}

			if (all) {
				hasRegression= REGRESS_LINEAR;
				slope= newSlope;
				slopeX = slope;
				intercept= newIntercept;
				interceptX = intercept;
			} else {
				oldSlope = slopeX;
				oldIntercept = interceptX;
				regressionUnselected= true;
				slopeX= newSlope;
				interceptX = newIntercept;
			}
		}
	}
}

void ScatterPlotCanvas::UpdateRegressionOption()  
{
	regressionUnselected= false;
	if (hasRegression == REGRESS_LINEAR) {
		// make sure it exists
		if (slope == 0 && intercept == 0)  ComputeRegression( true ); 
		if (excludeSelected) ComputeRegression( false );
	}
}

void ScatterPlotCanvas::ViewStandardizedData()
{
 	symmetricflag = false;
	if (StandardizedFlag) 
		return;
	Standardized();
    BuildMesh(symmetricflag);
    ComputeRegression( true );
    slopeX = slope;
 
	CheckSize();
	Refresh();
}

void ScatterPlotCanvas::ViewOriginalData()
{
	symmetricflag = true;
	if (!StandardizedFlag) 
		return;

	Destandardized();
    BuildMesh(symmetricflag);
    ComputeRegression( true );
    slopeX = slope;

	CheckSize();
	Refresh();
}

void ScatterPlotCanvas::ViewRegressionSelectedExcluded()
{
	// TODO: Add your command handler code here
	excludeSelected = !excludeSelected;
	regressionUnselected = 	!regressionUnselected;

	wxClientDC dc(this);
	Selection(&dc);

	CheckSize();
	Refresh();
}


void ScatterPlotCanvas::UpdateCondition(int *flags)
{
	Conditionable::UpdateCondition(flags);
    ComputeRegression(!excludeSelected);
	Refresh();
}

