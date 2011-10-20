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
#include <wx/xrc/xmlres.h>              // XRC XML resouces

#include "../DialogTools/SaveToTableDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../Project.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../ShapeOperations/GalWeight.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../ShapeOperations/Randik.h"
#include "MoranGView.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../Thiessen/VorDataType.h"
#include "../mapview.h"
#include "../GeneralWxUtils.h"
#include "../GeoDaConst.h"
#include "../logger.h"
 
#include "../Explore/BoxPlotView.h"
#include "../Explore/ScatterPlotView.h"
#include "../Explore/MoranScatterPlotView.h"
#include "../Explore/MoranGView.h"
#include "../DialogTools/RandomizationDlg.h"

extern	Selection gSelection;
extern	GeoDaEventType	gEvent;
extern	int gObservation;
extern	double *m_gX, *m_gY;
extern	wxString m_gVar1, m_gVar2;
extern	int Mesh(double & rMin, double & rMax, int lower = 3, int upper=6); 
extern MyFrame* frame;

BEGIN_EVENT_TABLE(MoranGFrame, wxFrame)

	EVT_ACTIVATE(MoranGFrame::OnActivate)
    EVT_CLOSE(MoranGFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), MoranGFrame::OnMenuClose)

	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_99PERMUTATION"),
			 MoranGFrame::OnRan99Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_199PERMUTATION"),
			 MoranGFrame::OnRan199Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_499PERMUTATION"),
			 MoranGFrame::OnRan499Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_999PERMUTATION"),
			 MoranGFrame::OnRan999Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_OTHER"),
			 MoranGFrame::OnRanOtherPer)
	
	EVT_MENU(XRCID("ID_OPTION_ENVELOPESLOPES"),
			 MoranGFrame::OnEnvelopeSlopes)

	EVT_MENU(XRCID("ID_SAVE_MORANI"), MoranGFrame::OnSaveMoranI)
	
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MoranGCanvas, wxScrolledWindow)
    EVT_SIZE(MoranGCanvas::OnSize)
    EVT_MOUSE_EVENTS(MoranGCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(MoranGCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()



// ---------------------------------------------------------------------------
// MoranG Frame
// ---------------------------------------------------------------------------

MoranGFrame::MoranGFrame(wxFrame *parent,
						 Project* project,
						 GalWeight* gal,
						 const wxString& title,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
	my_children.Append(this);

    // this should work for MDI frames as well as for normal ones
    SetSizeHints(100, 100);

	int width, height;
	GetClientSize(&width, &height);
	canvas = new MoranGCanvas(project, gal, this, wxPoint(0, 0),
							  wxSize(width, height));
	template_canvas = canvas;
	template_canvas->template_frame = this;
	
	Show(true);
}

MoranGFrame::~MoranGFrame()
{
	my_children.DeleteObject(this);
}

void MoranGFrame::Update()
{
	wxClientDC dc(canvas);
    PrepareDC(dc);
	canvas->Selection(&dc);
}

void MoranGFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MoranGFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In MoranGFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("MoranGFrame", GetTitle());
	}
    if ( event.GetActive() && canvas ) canvas->SetFocus();
}

void MoranGFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In MoranGFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void MoranGFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In MoranGFrame::OnMenuClose");
	Close();
}

void MoranGFrame::MapMenus()
{
	LOG_MSG("In MoranGFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_SCATTER_M_PLOT_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
}


void MoranGFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	canvas->ViewRegressionSelectedExcluded();
}

void MoranGFrame::OnRan99Per(wxCommandEvent& event)
{
	RandomizationP(99);
}

void MoranGFrame::OnRan199Per(wxCommandEvent& event)
{
	RandomizationP(199);
}

void MoranGFrame::OnRan499Per(wxCommandEvent& event)
{
	RandomizationP(499);
}

void MoranGFrame::OnRan999Per(wxCommandEvent& event)
{
	RandomizationP(999);
}

void MoranGFrame::OnRanOtherPer(wxCommandEvent& event)
{
	CPermutationCounterDlg PC(this);
	if (PC.ShowModal() != wxID_OK) return;

	long P;
	(PC.m_number->GetValue()).ToLong(&P);
	RandomizationP(P);
}

void MoranGFrame::RandomizationP(int numPermutations)
{
	if (canvas->weightsCGal == NULL) return;
	if (numPermutations < 9) numPermutations = 9;
	if (numPermutations > 49999) numPermutations = 49999;
	
	canvas->setNumberPermutations(numPermutations);
	
	CRandomizationDlg dlg(2,canvas, numPermutations, canvas->slope, NULL);
	dlg.ShowModal();
	/*
	 CRandomizationDlg* dlg;
	 dlg = new CRandomizationDlg(this);
	 dlg->Create(this);
	 dlg->ShowModal();
	 */
	//  Randomization* my_Random;
	
	//  my_Random = new Randomization(this, numPermutations, slope, NULL);
	//  my_Random->Create(IDD_RANDOMIZATION);
	//  my_Random->ShowWindow(SW_SHOW);
	
	canvas->EmptyClick();
	return;
}


void MoranGFrame::OnEnvelopeSlopes(wxCommandEvent& event)
{
	canvas->DrawEnvelope();
}

void MoranGFrame::OnSaveMoranI(wxCommandEvent& event)
{
	canvas->SaveMoranI();
}

// ---------------------------------------------------------------------------
// MoranG Canvas 
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
MoranGCanvas::MoranGCanvas( Project* project, GalWeight* gal,
						   wxWindow *parent,
						   const wxPoint& pos, const wxSize& size)
          : TemplateCanvas(parent, pos, size), project_p(project)
{
	flick = true;
	oldSlope = 0.0;
	oldIntercept = 0.0;
	location.resize(gObservation + 1);
	RawData = new DataPoint [ gObservation +1 ];

	Horizontal = m_gVar1;
	Vertical = m_gVar2;

	SpbackColor = wxColour(255,255,255);

	regressionUnselected = false;
	excludeSelected = false;
	symmetricflag = true;
	obs = gObservation;

	for (int i=0; i<obs; i++) {
		RawData[i] = DataPoint(m_gX[i], m_gY[i]);
	}

	weightsCGal = gal;

	makeLocalMoran= true;
	envelope = false; 
	totFrequency = 0;
	uEnvSlope =0.0; lEnvSlope = 0.0;
	bool makeLM=true;

	Init();
    SetBackgroundColour(wxColour("WHITE"));
    CheckSize();
    Refresh();
}

MoranGCanvas::~MoranGCanvas()
{
	if (RawData) delete [] RawData; RawData = 0;
}


// Define the repainting behaviour
void MoranGCanvas::OnDraw(wxDC& dc)
{
    selection_outline_visible = false;
	Draw(&dc);
}


void MoranGCanvas::OnEvent(wxMouseEvent& event)
{
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if (event.RightUp()) {
	    PopupMenu(wxXmlResource::Get()->
            LoadMenu("ID_SCATTER_M_PLOT_VIEW_MENU_CONTEXT"),
            event.GetPosition().x, event.GetPosition().y);
		return;
	}

	TemplateCanvas::OnEvent(event);
    event.Skip();
}

void MoranGCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void MoranGCanvas::CheckSize() {
    wxSize size2 = GetClientSize();

    int width = size2.x;
    int height = size2.y;

    Left = 10;
    Right = 10;
    Top = 15;
    Bottom = 15;
    Width = 40;
    Height = 40;

    int res = width - Left - Right - Width;
    if (res < 0) res = 0;
    int rata = res / 16;
    Left += rata;
    Right += rata;
    Width = width - Left - Right;
    res = height - Top - Bottom - Height;
    if (res < 0) res = 0;
    rata = res / 32;
    Top += rata;
    Bottom += 4 * rata;
    Height = height - Top - Bottom;
    // This is a way to make square
    if (Width > Height)
        Width = Height;
    else
        Height = Width;
    int cnt = 0;
    xDensity = Width / (xMax - xMin);
    yDensity = Height / (yMax - yMin);
    for (cnt = 0; cnt < gObservation; ++cnt) {
        location.at(cnt).x = (long) (
            (RawData[cnt].horizontal - xMin) * xDensity + Left);
        location.at(cnt).y = (long) (
            Top + Height - (RawData[cnt].vertical - yMin) * yDensity);
    }
    starSize = (int) (log10((double) Width + (double) Height)
        - log10((double) gObservation)
        + ((double) Width + (double) Height) / 256);
    if (starSize < 0)
        starSize = 0;
    else if (starSize > 4)
        starSize = 4;
}

void MoranGCanvas::OnSize(wxSizeEvent& event)
{
    CheckSize();
    Refresh();
    event.Skip();
}

// the function name "fraction" is already used by a constant in the Carbon API.
inline double _fraction(const double x)  {
    return x - floor(x);
}


void MoranGCanvas::BuildMesh(const bool asymmetric)  
{
     int cnt;
    if (asymmetric)  {
       xMin= xMax= RawData[0].horizontal;
       yMin= yMax= RawData[0].vertical;
       for (cnt= 0; cnt < gObservation; ++cnt)  {
         if (xMin > RawData[cnt].horizontal) xMin= RawData[cnt].horizontal;
           else if (xMax < RawData[cnt].horizontal) xMax= RawData[cnt].horizontal;
         if (yMin > RawData[cnt].vertical) yMin= RawData[cnt].vertical;
           else if (yMax < RawData[cnt].vertical) yMax= RawData[cnt].vertical;
       };
       xUnits= Mesh(xMin, xMax);
       yUnits= Mesh(yMin, yMax);
    }  else  {
       double   min, max;
       min= max= RawData[0].horizontal;
       for (cnt= 0; cnt < gObservation; ++cnt)  {
         if (min > RawData[cnt].horizontal) min= RawData[cnt].horizontal;
           else if (max < RawData[cnt].horizontal) max= RawData[cnt].horizontal;
         if (min > RawData[cnt].vertical) min= RawData[cnt].vertical;
           else if (max < RawData[cnt].vertical) max= RawData[cnt].vertical;
       };
      for (cnt= 0; cnt < gObservation; ++cnt)  {
        if (min > -RawData[cnt].horizontal) min= -RawData[cnt].horizontal;
          else if (max < -RawData[cnt].horizontal) max= -RawData[cnt].horizontal;
        if (min > RawData[cnt].vertical) min= RawData[cnt].vertical;
          else if (max < RawData[cnt].vertical) max= RawData[cnt].vertical;
      };
      xUnits= yUnits= Mesh(min, max);
      xMin= yMin= min;
      xMax= yMax= max;
    };
    return;
}

bool MoranGCanvas::Init()  
{
	int cnt;
	double sum= 0, mean, dev;// xx[1000];
	
	var1 = new double [gObservation];
	var2 = new double [gObservation];
	for (cnt= 0; cnt < gObservation; ++cnt) {
		sum += RawData[cnt].horizontal;
 		var2[cnt] = RawData[cnt].vertical;
	}
	
	mean= sum / gObservation;
	if (mean != 0)  {         // remove the mean
		for (cnt= 0; cnt < gObservation; ++cnt) RawData[cnt].horizontal -= mean;
	}
	sum= 0;                   // accumulate the sum of squares
	for (cnt= 0; cnt < gObservation; ++cnt)
		sum += geoda_sqr(RawData[cnt].horizontal);
	dev= sqrt(sum / (gObservation-1));     // std. deviation
	if (dev != 0)
		for (cnt= 0; cnt < gObservation; ++cnt)
			RawData[cnt].horizontal /= dev;
	//================================================ 2nd Var
	
	sum = 0;
	for (cnt= 0; cnt < gObservation; ++cnt) sum += var2[cnt];
	
	
	mean= sum / gObservation;
	if (mean != 0) {         // remove the mean
		for (cnt= 0; cnt < gObservation; ++cnt)
			var2[cnt] -= mean;
	}
	sum = 0;
	for (cnt= 0; cnt < gObservation; ++cnt) sum += geoda_sqr(var2[cnt]);
	dev= sqrt(sum / (gObservation-1));     // std. deviation
	if (dev != 0) {
		for (cnt= 0; cnt < gObservation; ++cnt) var2[cnt] /= dev;
	}
	for (cnt = 0; cnt < gObservation; cnt++) {
		var1[cnt] = RawData[cnt].horizontal;
		RawData[cnt].vertical = weightsCGal->gal[cnt].SpatialLag(var2, true);
	}
	
	setNumberPermutations(499); // permDefault = 499
	BuildMesh(false); // false = build symmetric mesh
	
	ComputeRegression( true );
	
	return true;
}


void MoranGCanvas::Draw(wxDC* pDC)
{
    DrawLegend(pDC);
    DrawAllPoints(pDC);
    DrawAxes(pDC);
    SlopeReport(pDC, "Moran's I ");
}

void MoranGCanvas::DrawAllPoints(wxDC* pDC)
{
    for (int cnt= 0; cnt < gObservation; ++cnt)  {
        // if(!conditionFlag[cnt]) continue;
        wxColour color=gSelection.selected(cnt) ?
			highlight_color : GeoDaConst::textColor;
        GenUtils::DrawSmallCirc(pDC,
								(int)(location.at(cnt).x),
								(int)(location.at(cnt).y),
            starSize, color);
    }
}

inline bool MoranGCanvas::isGreater(const double standard, const bool checkX)
{
  if (checkX)
      return  (xMin == 0 || fabs(xMin) >= standard) &&
              (xMax == 0 || fabs(xMax) >= standard);
    else
      return  (yMin == 0 || fabs(yMin) >= standard) &&
              (yMax == 0 || fabs(yMax) >= standard);
}

inline bool MoranGCanvas::isLess(const double standard, const bool checkX)
{
  if (checkX)
      return (fabs(xMin) <= standard && fabs(xMax) <= standard);
    else
      return (fabs(xMin) <= standard && fabs(xMax) <= standard);
}

void MoranGCanvas::DrawLegend(wxDC* dc)  {

	char  buf[64];  // char   buf2[16];<--- useless
	double    scale= 1;

	int       sz= 7, szy=7, len;
	
	sz= Bottom/8 + Width/xUnits/16 + 1;
	double    dval= (xMax-xMin)/xUnits/scale;
	if (sz > 15) sz = 15;
	if (sz > 6)  
	{
		wxFont m_font(*wxNORMAL_FONT);
		m_font.SetPointSize(sz);
		dc->SetFont(m_font);

		wxString xLabel = Horizontal;

		if (isGreater(1000000, 1)) {  scale= 1000000;  xLabel = Horizontal + " (in 10^6)";  }
		else if (isGreater(1000, 1)) {  scale = 1000;  xLabel = Horizontal + " (in 1000)";  }
		else if (isLess(0.001, 1))  {  scale= 0.001; xLabel = Horizontal + " (in 0.001)";  };


		len = xLabel.Length();
		dc->DrawText(xLabel, Left+Width/2-len*sz/8, Top+Height+Bottom/3+4);



		double val= xMin/scale;
		dval= (xMax-xMin)/xUnits/scale;
		for (int cnt= 0; cnt <= xUnits; ++cnt)  
		{
			if (fabs(val) < 1e-15) val = 0;
			//_gcvt(val, 8, buf);
			ggcvt(val, 8, buf);

			sprintf ( buf, "%.*f", 1, val );

			len= strlen(buf);
			if (buf[len-1] == '.')  buf[--len]= '\x0';
			dc->DrawText(wxString::Format("%s",buf),
						 Left+Width*cnt/xUnits-len*sz/6,
						 Top+Height+1+Bottom/16);
			val += dval;
		};
	};

	szy= Left/8 + Height/yUnits/16 + 1;
	if (szy > 15) szy = 15;
	if (szy > 6)  
	{  
		wxFont m_font2(*wxNORMAL_FONT);
		m_font2.SetPointSize(szy);
		dc->SetFont(m_font2);

        wxString yLabel = Vertical;
		scale= 1;
		if (isGreater(1000000, 0)) {  scale= 1000000;  yLabel = Vertical + " (in 10^6)";  }
		else if (isGreater(1000, 0)) {  scale = 1000;  yLabel = Vertical + " (in 1000)";  }
		else if (isLess(0.001, 0))  {  scale= 0.001; yLabel = Vertical + " (in 0.001)";  };

		dval= (yMax-yMin)/yUnits/scale;

		len= yLabel.Length();
		dc->DrawRotatedText(yLabel, Left/8-2, Top+Height/2+len*szy/6, 90);
 
		double val= yMin/scale + dval;
		for (int cnt= 1; cnt < yUnits; ++cnt)  
		{
			if (fabs(val) < 1e-15) val = 0;
			//_gcvt(val, 8, buf);
			ggcvt(val, 8, buf);

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

void MoranGCanvas::ComputeRegression( const bool all )  
{
  double    sumX= 0, sumY= 0, sumX2= 0, sumXY= 0;
  int       sampleSize = 1;

  flick = true;
  for (int cnt= 0; cnt < gObservation; ++cnt) {
    if (all || !gSelection.selected(cnt))  
		{
      sumX += RawData[cnt].horizontal;
      sumY += RawData[cnt].vertical;
      sumX2 += geoda_sqr(RawData[cnt].horizontal);
      sumXY += RawData[cnt].horizontal * RawData[cnt].vertical;
      ++sampleSize;
	  }
  };
  
  const double    denominator = sampleSize ? sumX2 - geoda_sqr(sumX)/sampleSize : 0;

  if (fabs(denominator) < 1.0e-12)
     if (all) hasRegression= REGRESS_NO;
       else regressionUnselected= false;
  else  
	{
     const double  newSlope=  (sumXY - sumX * sumY / sampleSize) / denominator;
     const double  newIntercept= (sumY - newSlope * sumX)/sampleSize;
	   if ((newSlope == oldSlope) && (newIntercept == oldIntercept)) flick = false;
      
     if (all)   
		 {
       hasRegression= REGRESS_LINEAR;
       slope= newSlope;
		   slopeX = slope;
       intercept= newIntercept;
		   interceptX = intercept;
     }
     else   
		 {
	     oldSlope = slopeX; 
	     oldIntercept = interceptX;
       regressionUnselected= true;
       slopeX= newSlope;
       interceptX = newIntercept;
     }
  }

}


void MoranGCanvas::DrawRegression(wxDC* dc, const double rSlope, 
							   const double rIntercept, wxColour color, int PS)  {

	wxPen pen;
	pen.SetColour(color);
	pen.SetWidth((starSize >> 2)+1);
	dc->SetPen(pen);


    int x1, y1, x2, y2;
    double  y;
    if (hasRegression == REGRESS_NO) {
	}
	else {

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
    };
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
}


void MoranGCanvas::DrawAxes(wxDC* dc)
{
  double    steps;
  int   coord;

  if (hasRegression == REGRESS_LINEAR)  {
    DrawRegression(dc, slope, intercept, GeoDaConst::outliers_colour, wxSOLID );
	if (envelope) {

		DrawRegression(dc, uEnvSlope, intercept, *wxRED, wxDOT );
		DrawRegression(dc, lEnvSlope, intercept, *wxRED, wxDOT );
	};
    if (excludeSelected && regressionUnselected) {
		wxColour bpColor = GetBackgroundColour(); 
		//DrawRegression(dc, oldSlope, oldIntercept, bpColor, wxSOLID );
		DrawRegression(dc, slopeX, interceptX, GeoDaConst::textColor, wxSOLID );
	};
  }

	wxPen pen;
	pen.SetColour(GeoDaConst::textColor);
	dc->SetPen(pen);


  if (yMin < 0 && yMax > 0)  {
	
    steps= -yMin/(yMax-yMin);
    coord= (int)(Top+Height*(1.0-steps));
    dc->DrawLine(Left, coord, Left+Width, coord);
  };
  if (xMin < 0 && xMax > 0)  {
    steps= -xMin/(xMax-xMin);
    coord= (int)(Left+Width*steps);
    dc->DrawLine(coord, Top, coord, Top+Height);
  }
  dc->DrawLine(Left, Top, Left, Top+Height);
  dc->DrawLine(Left, Top+Height, Left+Width, Top+Height);
  dc->DrawLine(Left+Width, Top+Height, Left+Width, Top);
  dc->DrawLine(Left+Width, Top, Left, Top);

  int   posX, posY, len= Bottom/16 + 1, cnt;
  for (cnt= 0; cnt <= xUnits; ++cnt)  {
    posX= Left + cnt * Width / xUnits;
    dc->DrawLine(posX, Top+Height, posX, Top+Height+len);
  };
  len= Left /16 + 1;
  for (cnt= 1; cnt <= yUnits; ++cnt)  {
    posY= Top + Height - cnt * Height / yUnits;
    dc->DrawLine(Left, posY, Left-len, posY);
  }
}


void MoranGCanvas::SlopeReport(wxDC* dc,const char * header)  
{

	char bf[40];
	wxString buf;
	int sz= Left/8 + (Height/yUnits/16 + Width/xUnits/16)/2 + 1;
	if (sz > 15) sz = 15;

  if (sz > 5)  
  {                        // report Moran's I for the whole data set
    dc->SetTextForeground(GeoDaConst::textColor );
 
    wxFont m_font(*wxNORMAL_FONT);
	m_font.SetPointSize(sz);

	dc->SetFont(m_font);


    dc->SetTextForeground(GeoDaConst::outliers_colour );

    const int bufLen0 = sprintf(bf, "%s","                                ");
    dc->DrawText(wxString::Format("%s",bf), Left+Width/2-bufLen0*sz/4, Top/8-1);


    const int bufLen= sprintf(bf, "%s= %-7.4f %s", header, slope,"          ");
    int displ= regressionUnselected ? 0 : Width/2-bufLen*sz/4;
    dc->DrawText(wxString::Format("%s",bf), Left+displ, Top/8-1);


    dc->SetTextForeground(GeoDaConst::textColor );
  }

  if (sz > 5 && regressionUnselected)  {    // report MI for a subsample

    wxFont m_font(*wxNORMAL_FONT);
	m_font.SetPointSize(sz);

	dc->SetFont(m_font);


	wxColour bpk = GetBackgroundColour();
	dc->SetTextForeground(bpk);
    sprintf(bf, "%s= %-7.4f", " I ", oldSlope);
    dc->DrawText(wxString::Format("%s",bf), 40+Left+Width/2, Top/8-1);

    dc->SetTextForeground(GeoDaConst::textColor );
    sprintf(bf, "%s= %-7.4f", " I ", slopeX);
    dc->DrawText(wxString::Format("%s",bf), 40+Left+Width/2, Top/8-1);
  }
}


void MoranGCanvas::SelectByPoint(wxMouseEvent& event)  
{
	int  Id=GeoDaConst::EMPTY, distId= GeoDaConst::EMPTY, dist;
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (fabs(location.at(cnt).x - gSelect1.x) < 3 &&
			fabs(location.at(cnt).y - gSelect2.y) < 3)
		{
			dist= (int)(geoda_sqr( location.at(cnt).x-gSelect1.x) +
			geoda_sqr( location.at(cnt).y-gSelect1.y));
			if (Id == GeoDaConst::EMPTY || distId > dist) {
				Id= cnt;
				distId= dist;
			}
		}
	}

	if (Id == GeoDaConst::EMPTY) {
		gEvent = NEW_SELECTION;
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

int MoranGCanvas::SelectByRect(wxMouseEvent& event)  
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
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (location.at(cnt).x >= p1.x && location.at(cnt).x <= p2.x &&
			location.at(cnt).y >= p1.y && location.at(cnt).y <= p2.y &&
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
	}

	return mCnt;
}

void MoranGCanvas::Selection(wxDC* dc)
{
	int cnt;	
	bool  regressUpdate = excludeSelected && (hasRegression== REGRESS_LINEAR);
    switch(gEvent)  {
        case NEW_SELECTION :
            DrawAllPoints(dc);
            DrawAxes(dc);
            break;
        case ADD_SELECTION :
            cnt= gSelection.Pop();
            if (cnt == GeoDaConst::EMPTY) regressUpdate= false;
            while (cnt != GeoDaConst::EMPTY)  {
                GenUtils::DrawSmallCirc(dc,(int)(location.at(cnt).x),
                    (int)(location.at(cnt).y), starSize, highlight_color);
                cnt= gSelection.Pop();
            }
            break;
        case DEL_SELECTION :
            while ((cnt= gSelection.Pop()) != GeoDaConst::EMPTY){
                GenUtils::DrawSmallCirc(dc,(int)(location.at(cnt).x),
                    (int)(location.at(cnt).y), starSize, GeoDaConst::textColor);
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
			LOG_MSG("Calling Draw from within MoranGCanvas::Selection");
			dc->Clear();
			Draw(dc);
			LOG_MSG("Called Draw from within MoranGCanvas::Selection");
			if (sel_curr_shown) DrawSelectionOutline();
        }
    }
    gSelection.Reset();
}

void MoranGCanvas::DrawEnvelope()
{
	envelope = !envelope;
	const int nPer = numberPermutations;

	if (envelope) 
	{
		MoranI = new double [ nPer];

		OnePermuteY(nPer);

		int * Ix = new int [ nPer ];
		for (int i=0; i < nPer; i++) Ix[i]=i;

		IndexSortD(MoranI,Ix,0,nPer-1);

		int LE =0,UE=0; char buf[50]; wxString x1;
		LE = (int) (0.05 * nPer);
		UE = (int) (0.95 * nPer);

		sprintf(buf,"Moran's I   : %7.4f\n",slope);
		x1 = wxString::Format("%s",buf);
		sprintf(buf,"%s CI\n   Upper: %7.4f\n", "95%", MoranI[Ix[UE]]);
		x1 = x1 + wxString::Format("%s",buf);
		sprintf(buf,"   Lower: %7.4f\n", MoranI[Ix[LE]]);
		x1 = x1 + wxString::Format("%s",buf);
		
		uEnvSlope = MoranI[Ix[UE]];
		lEnvSlope = MoranI[Ix[LE]];

	}

	wxClientDC dc(this);
	Selection(&dc);
}

void MoranGCanvas::UpdateRegressionOption()  {
  regressionUnselected= false;
  if (hasRegression == REGRESS_LINEAR) {
		// make sure it exists
    if (slope == 0 && intercept == 0)  ComputeRegression( true ); 
    if (excludeSelected) ComputeRegression( false );
  }
}

void MoranGCanvas::ViewRegressionSelectedExcluded()
{
	// TODO: Add your command handler code here
	excludeSelected = !excludeSelected;
	regressionUnselected = 	!regressionUnselected;

	wxClientDC dc(this);
	Selection(&dc);
	CheckSize();
	Refresh();
}

void MoranGCanvas::OnePermuteY(int nPermute)  
{
	Randik rng;
	int * perm = new int [obs];
	double newMoran;
   
	for (int i= 0; i < nPermute; i++)
	{
		rng.PermG(obs, perm) ;
		newMoran = OnePermute( perm );
		MoranI[ i ] = newMoran;
	}

	if (perm) delete [] perm; perm = 0;
}

double MoranGCanvas::OnePermute(const int * Permutation)  
{
	double  newMoran = 0;

	for (int cnt = 0; cnt < gObservation; ++cnt) {
		newMoran += weightsCGal->gal[cnt].SpatialLag(RawData, Permutation, true)
			* RawData[Permutation[cnt]].horizontal;
	}

	newMoran /= gObservation - 1;
	return newMoran;
}


void MoranGCanvas::SaveMoranI()
{
	wxString title = "Save Results: Moran's I";
	std::vector<double> std_data(gObservation);
	std::vector<double> lag(gObservation);
	for (int i=0; i<gObservation; i++) {
		std_data[i] = RawData[i].horizontal;
		lag[i] = RawData[i].vertical;
	}
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &std_data;
	data[0].label = "Standardized Data";
	data[0].field_default = "MORAN_STD";
	data[0].type = GeoDaConst::double_type;
	data[1].d_val = &lag;
	data[1].label = "Spatial Lag";
	data[1].field_default = "MORAN_LAG";
	data[1].type = GeoDaConst::double_type;
	
	SaveToTableDlg dlg(project_p->GetGridBase(), this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}
