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
#include <wx/splitter.h>
#include <wx/sizer.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../Thiessen/VorDataType.h"
#include "../Thiessen/Thiessen.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../GenUtils.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "ConditionalView.h"
#include "CartogramView.h"


extern Selection gSelection;
extern GeoDaEventType gEvent;
extern int gObservation;
extern wxString m_gVar1, m_gVar2;

extern wxString	gCompleteFileName;
extern double *m_gX;

extern MyFrame* frame;

const double friction = 0.25;
const double ratio = 0.4;
const double pi = 3.14159265;
const double screen_scale = 0.001;

struct leaf
{
	int id; 
	double xpos;
	double ypos;
	int left;
	int right;
};

extern bool CreatePolygonShapeFile(char* otfl, char *infl, const int nPolygon,
								   myPolygon* Polygons, myBox B);

BEGIN_EVENT_TABLE(CartogramFrame, TemplateFrame)
	EVT_ACTIVATE(CartogramFrame::OnActivate)
	EVT_CLOSE(CartogramFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), CartogramFrame::OnMenuClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CartogramCanvas, wxScrolledWindow)
    EVT_SIZE(CartogramCanvas::OnSize)
    EVT_MOUSE_EVENTS(CartogramCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(CartogramCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()


// ---------------------------------------------------------------------------
// Cartogram Frame
// ---------------------------------------------------------------------------

CartogramFrame::CartogramFrame(wxFrame *parent, Project* project,
							   const wxString& title,
							   const wxPoint& pos, const wxSize& size,
							   const long style)
           :TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
	my_children.Append(this);
	SetSizeHints(100, 100);
	
	int width, height;
	GetClientSize(&width, &height);

	m_splitter = new wxSplitterWindow(this);

	legend = new CartogramLegend(m_splitter, wxPoint(0, 0),
								 wxSize(0,0), m_gVar1);
	canvas = new CartogramCanvas(m_splitter, wxPoint(0, 0), wxSize(0,0));
	
	template_legend = legend;
	template_canvas = canvas;
	template_canvas->template_frame = this;
	template_legend->template_frame = this;
	
	m_splitter->SplitVertically(legend, canvas, 100);

	Show(true);
}

CartogramFrame::~CartogramFrame()
{
	my_children.DeleteObject(this);
}

void CartogramFrame::Update()
{
	wxClientDC dc(canvas);
	canvas->PrepareDC(dc);
	canvas->Selection(&dc);
}

void CartogramFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}

void CartogramFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In CartogramFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("CartogramFrame", GetTitle());
	}
	if ( event.GetActive() && canvas ) canvas->SetFocus();
}

void CartogramFrame::UpdateMenuBarCheckMarks(wxMenuBar* menuBar)
{
	UpdateMenuCheckMarks(menuBar->GetMenu(menuBar->FindMenu("Options")));
}

void CartogramFrame::UpdateMenuCheckMarks(wxMenu* menu)
{
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_15"),
								  canvas->m_HingeCheck15);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_30"),
								  canvas->m_HingeCheck30);
}

void CartogramFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In CartogramFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void CartogramFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In CartogramFrame::OnMenuClose");
	Close();
}


void CartogramFrame::MapMenus()
{
	LOG_MSG("In CartogramFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CARTOGRAM_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateMenuCheckMarks(optMenu);
}


void CartogramFrame::OnMoreIter100(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("In CartogramFrame::OnMoreIter100");

	for(int body=canvas->bodies-1; body>0; body--)
		canvas->radius[body] = canvas->radius[body-1];

	canvas->ConstructCartogram(100);
	canvas->Refresh();
}

void CartogramFrame::OnMoreIter500(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("In CartogramFrame::OnMoreIter500");

	for(int body=canvas->bodies-1; body>0; body--)
		canvas->radius[body] = canvas->radius[body-1];

	canvas->ConstructCartogram(500);
	canvas->Refresh();
}

void CartogramFrame::OnMoreIter1000(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("In CartogramFrame::OnMoreIter1000");

	for(int body=canvas->bodies-1; body>0; body--)
		canvas->radius[body] = canvas->radius[body-1];

	canvas->ConstructCartogram(1000);
	canvas->Refresh();
}

void CartogramFrame::OnHinge15(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("In CartogramFrame::OnHinge15");
	canvas->m_HingeCheck15 = true;
	canvas->m_HingeCheck30 = false;
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	canvas->AdjustHinge(1.5);
	canvas->Refresh();
}

void CartogramFrame::OnHinge30(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("In CartogramFrame::OnHinge30");
	canvas->m_HingeCheck15 = false;
	canvas->m_HingeCheck30 = true;
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	canvas->AdjustHinge(3.0);
	canvas->Refresh();
}


// ---------------------------------------------------------------------------
// Cartogram Canvas
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
CartogramCanvas::CartogramCanvas(wxWindow *parent, const wxPoint& pos,
								 const wxSize& size)
	: TemplateCanvas(parent, pos, size)
{
	MAX_NEIGHBOR = 50;

	location.resize(gObservation + 1);
	RawData = new double[gObservation + 1];
	m_HingeCheck15 = true;
	m_HingeCheck30 = false;

	for(int i=0; i < gObservation; i++) RawData[i] = m_gX[i];
	
	bodies = gObservation+1;

	xl = new double[bodies];
	yl = new double[bodies];
	tree = new struct leaf[bodies];
	list = new int[bodies];

	vector<double> x(gObservation+1);
	vector<double> y(gObservation+1);
	myBox* b = new myBox;

	long obs = gObservation;

	ComputeXY(gCompleteFileName, &obs, x, y, b, true);

	xMin = b->p1.x;
	yMin = b->p1.y;
	xMax = b->p2.x;
	yMax = b->p2.y;

	//  Construct Cartogram
	radius = new double[gObservation+1];
	loc_x = new double[gObservation+1];
	loc_y = new double[gObservation+1];
	outliers = new int[gObservation+1];

	InitCartogram(x, y, RawData);
	ConstructCartogram();

	for(int i=0; i<gObservation; i++) {
		if(xMin > (loc_x[i]-radius[i]))	xMin = (loc_x[i]-radius[i]);
		if(xMax < (loc_x[i]+radius[i]))	xMax = (loc_x[i]+radius[i]);
		if(yMin > (loc_y[i]-radius[i]))	yMin = (loc_y[i]-radius[i]);
		if(yMax < (loc_y[i]+radius[i]))	yMax = (loc_y[i]+radius[i]);
	}

	delete b; b = 0;

	colors = new wxColour[gObservation];

	for(int i=0; i<gObservation; i++) colors[i] = wxColour(0,200,0);

    SetBackgroundColour(wxColour("WHITE"));
    Refresh();
}

CartogramCanvas::~CartogramCanvas()
{
	delete [] ValueFlag; ValueFlag = 0;
	delete [] xl; xl = 0;
	delete [] yl; yl = 0;
	delete [] tree;	tree = 0;
	delete [] list;	list = 0;

	delete [] people; people = 0;
	delete [] people_temp; people_temp = 0;
	delete [] nbours; nbours = 0;
	delete [] nbour; nbour = 0;
	delete [] xvector; xvector = 0;
	delete [] yvector; yvector = 0;
	delete [] border; border = 0;
	delete [] perimeter; perimeter = 0;

	delete [] conti; conti = 0;

	delete [] RawData; RawData = 0;
	delete [] loc_x; loc_x = 0;
	delete [] loc_y; loc_y = 0;
	delete [] outliers; outliers = 0;

	delete [] colors; colors = 0;
}

// Define the repainting behaviour
void CartogramCanvas::OnDraw(wxDC& dc)
{
    LOG_MSG("Entering CartogramCanvas::OnDraw");
    selection_outline_visible = false;
	Draw(&dc);
	LOG_MSG("Exiting CartogramCanvas::OnDraw");
}

void CartogramCanvas::Draw(wxDC* dc)
{
	DrawAllCircles(dc);
}

void CartogramCanvas::OnEvent(wxMouseEvent& event)
{
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if (event.RightUp()) {
		wxMenu* optMenu = wxXmlResource::Get()->
			LoadMenu("ID_CARTOGRAM_VIEW_MENU_CONTEXT");
		((CartogramFrame*) template_frame)->UpdateMenuCheckMarks(optMenu);
		PopupMenu(optMenu, event.GetPosition());
		return;
	}
	
	TemplateCanvas::OnEvent(event);
	event.Skip();
}

void CartogramCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void CartogramCanvas::CheckSize()
{
	wxSize size2 = GetClientSize();

	int width = size2.x;
	int height = size2.y;

	double xDensity;
	double yDensity;

	Left= 10;  Right= 10;
    Top= 10;   Bottom= 10;
    Width= 40;
    Height= 40;


	Width = width;
	Height = height;
	xDensity= Width/(xMax-xMin);
	yDensity= Height/(yMax-yMin);

	int offset_x;
	int offset_y;
	if(xDensity > yDensity) {
		rFactor = yDensity;
		xDensity = yDensity;
		offset_x = (Width-(int) ((xMax-xMin)*xDensity))/2;
		offset_y = 0;
	} else {
		rFactor = xDensity;
		yDensity = xDensity;
		offset_x = 0;
		offset_y = (Height-(int) ((yMax-yMin)*yDensity))/2;
	}

	for (int cnt= 0; cnt < gObservation; ++cnt) {
		location.at(cnt).x = (int) ((loc_x[cnt]-xMin)*xDensity+offset_x);
		location.at(cnt).y = (int) (Height-(loc_y[cnt]-yMin)*yDensity-offset_y);
	}
}

void CartogramCanvas::OnSize(wxSizeEvent& event)
{
	CheckSize();
    Refresh();
    event.Skip();
}

void CartogramCanvas::InitCartogram(const vector<double>& x11,
									const vector<double>& y11,
									double* RawData)
{
	obs = gObservation;
	
	map_to_people = 9000000;
	map_to_coorindate = 300000;
	
	char buf_filename[512];
	strcpy( buf_filename, (const char*)gCompleteFileName.mb_str(wxConvUTF8) );
	char* filename = buf_filename;
	
	if (!gCompleteFileName.IsEmpty()) { 
		//  create shpfile		
		
		long nPoints; myBox* B=new myBox;
		nPoints = GetShpFileSize(gCompleteFileName);
		
		vector<double> x(nPoints+1);
		vector<double> y(nPoints+1);
		
		ComputeXY(gCompleteFileName, &nPoints, x, y, B, true);
		
		myPoint *pt = NULL;
		pt = new myPoint[nPoints+1];
		
		int i;
		for (i=0;i<nPoints;i++) {
			pt[i].x = x.at(i);
			pt[i].y = y.at(i);
		}
		int (*compare) (const void*, const void*);
		compare = XYSort;
		qsort((void*) pt, nPoints, sizeof(myPoint), compare);
		
		char dbffl[128];

		GenUtils::extension(dbffl,filename,"dbf");
		
		
		myBox BB;
		BB.p1.x = B->p1.x; BB.p1.y = B->p1.y;
		BB.p2.x = B->p2.x; BB.p2.y = B->p2.y;
		
		Thiessen*	TT;
		TT = new Thiessen(nPoints,x,y,BB);
		
		myPolygon* myP;
		
		myP = TT->GetPolygons();
		BB.p1.x -= 0.01; BB.p1.y -= 0.01;
		BB.p2.x += 0.01; BB.p2.y += 0.01;
		
		wxString otFile = gCompleteFileName+ "_temp.shp";
		char buf_otfl[512];
		strcpy( buf_otfl, (const char*)otFile.mb_str(wxConvUTF8) );
		char* otfl = buf_otfl;
		
		CreatePolygonShapeFile(otfl, dbffl, nPoints, myP, BB);
		
		conti = shp2gal(otFile,1,false);
		
		wxRemoveFile(otFile);
		
		int pos = otFile.Find('.',true);
		wxString otdbf = otFile.Left(pos)+ ".dbf";
		pos = otFile.Find('.',true);
		wxString otshx = otFile.Left(pos)+ ".shx";
		
		wxRemoveFile(otdbf);
		wxRemoveFile(otshx);
		
		delete [] pt;
		pt = NULL;
	} else {
		conti = shp2gal(filename, 1, false);
	}
	
	bodys  = gObservation;
	done = 0;
	
	people = new double[bodies];
	people_temp = new double[bodies];
	nbours = new int[bodies];
	nbour = new int[bodies*MAX_NEIGHBOR];
	xvector = new double[bodies];
	yvector = new double[bodies];
	border = new double[bodies*MAX_NEIGHBOR];
	perimeter = new double[bodies];
	
	t_dist = 0.0;
	t_radius = 0.0;
	
	long cnt = 0;
	// Checking for ZEROS and NEGATIVES
	int *Index0 = new int[obs];
	ValueFlag = new int[obs];
	// ValueFlag= 0:Negative; 1:zero; 2:positive
	for (cnt= 0; cnt < obs; ++cnt) {
	    Index0[cnt]= cnt;
		ValueFlag[cnt] = 2;
	}
	
	IndexSortD(RawData, Index0, 0, obs-1);
	
	double range;
	bool AllNegative = false;
	if (RawData[Index0[obs-1]] <= 0) {
		double range = RawData[Index0[obs-1]] - RawData[Index0[0]];
		if (range == 0.0) {
			range = 1.0;
		}
		
		for (body = 1; body < bodies; body++) {
			people[body-1] = 10.0 +
			((RawData[body-1]-RawData[Index0[0]]) / (range / 100.0));
			
		}
		AllNegative = true;
	}
	else {
		int k=0;
		while (k < obs && RawData[Index0[k]] < 0) {
			ValueFlag[Index0[k]] = 0;
			k++;
		}
		
		while (k < obs && RawData[Index0[k]] == 0) {
			ValueFlag[Index0[k]] = 1;
			k++;
		}
		
		range = RawData[Index0[obs-1]] - RawData[Index0[0]];
		if (range == 0.0) {
			wxMessageBox("All have the same values!");
			range = 1.0;
		}
	}
	
	for (body = 1; body < bodies; body++) {
		if (!AllNegative)
			people[body-1] = 10.0 +
				((RawData[body-1]-RawData[Index0[0]]) / (range / 100.0));
		xl[body] = x11.at(body-1)*map_to_coorindate/(xMax-xMin);
		yl[body] = y11.at(body-1)*map_to_coorindate/(xMax-xMin);
		nbours[body] = conti[body-1].Size();
		
		perimeter[body] = 0.0;
		for(nb = 1; nb <= nbours[body]; nb++) {
			boundary = 1.0;
			nbour[body*MAX_NEIGHBOR+nb] = conti[body-1].elt(nb-1)+1;
			
			border[body*MAX_NEIGHBOR+nb] = (float) boundary;
			perimeter[body] += border[body*MAX_NEIGHBOR+nb];
			
		}
		people_temp[body-1] = people[body-1];
	}
	delete [] Index0;
	Index0 = NULL;
	int *Index;
	Index = new int[obs];
	
	for (cnt= 0; cnt < obs; ++cnt)
	    Index[cnt]= cnt;
	
	int   obs1= obs - 1, obsPlus= obs + 1;
	IndexSortD(people, Index, 0, obs1);
	q[0] = Index[0];               // smallest value
	q[4] = Index[obs1];            // largest value
		// The following median is different that what is in BoxPlot, but
	// apparently it isn't used anywhere, so it doesn't really matter
	q[2] = Index[obsPlus/2-1];     // median

	if (obs >= 5) {
		//previously: q[1] = Index[obsPlus/4-1];     // 1st quartile
		q[1] = Index[(obs+1)/4 - 1];
		//previously: q[3] = Index[3*obsPlus/4-1];   // 3rd quartile
		q[3] = Index[3*(obs+1)/4 - 1];
	} else {
		q[1] = Index[0];
		q[3] = Index[obs-1];
	}
	
	delete [] Index;
	Index = NULL;
	
	
	double rMax = -1e200;
	double rMin = 1e200;
	
	double out_max = 1e200;
	double out_min = -1e200;
	
	double iqd = people[q[3]] - people[q[1]];           
	double miqd = 1.5 * iqd;
	
	double Lower = people[q[1]] - miqd;
	if (Lower < 0.0) Lower = 0.0;
    
	double Upper = people[q[3]] + miqd;
	out_max = Upper;
	out_min = Lower;
	int i = 0;
	
	for(i=0; i<obs; i++) {	
		if (people[i] > 0) {
			if( (people[i] > rMax) && (people[i] <= out_max) )
				rMax = people[i];
			if( (people[i] <= rMin) && (people[i] >= out_min) )
				rMin = people[i];
		}
	}
	
	for(i=0; i<obs; i++) {	
		if(people[i] > out_max) {
			outliers[i] = 2;
		} else if(people[i] < out_min) {
			outliers[i] = 1;
		} else {
			outliers[i] = 0; 
		}
		people[i] = people[i]/rMax*map_to_people;
		
	} 
	
	for(i=1; i<bodies; i++)
		people[bodies-i] = people[bodies-i-1];
	
	for (body = 1; body < bodies; body++) {
		for(nb = 1; nb <= nbours[body]; nb++) {
			if (nbour[body*MAX_NEIGHBOR+nb] > 0) {
				if(nbour[body*MAX_NEIGHBOR+nb] < body) {
					xd = (float)(xl[body] - xl[nbour[body*MAX_NEIGHBOR+nb]]);
					yd = (float)(yl[body] - yl[nbour[body*MAX_NEIGHBOR+nb]]);
					t_dist += sqrt(xd*xd+yd*yd);
					t_radius += sqrt(people[body]/pi) +
						sqrt(people[nbour[body*MAX_NEIGHBOR+nb]]/pi);
				}
			}
		}    
	}
	
	scale = t_dist / t_radius;
	widest = 0.0;
	
	for(body=1; body < bodies; body++) {
		radius[body] = scale*sqrt(people[body]/pi); 
		if(radius[body] > widest) widest = radius[body];
		xvector[body] = 0.0;
		yvector[body] = 0.0;
	}
}

void CartogramCanvas::AdjustHinge(double Hinge)
{
	double rMax = -1e200;
	double rMin = 1e200;
	
	double out_max = 1e200;
	double out_min = -1e200;
	
	for (body = 0; body < bodies-1; body++) {
		people[body] = people_temp[body];
	}
	
	double    iqd = people[q[3]] - people[q[1]];           
	double    miqd = Hinge * iqd;
	
	double Lower = people[q[1]] - miqd;                      
	if (Lower < 0.0) 
		Lower = 0.0;
    
	double Upper = people[q[3]] + miqd;
	out_max = Upper;
	out_min = Lower;
	int i = 0;
	
	for(i=0; i<obs; i++) {	
		if (people[i] > 0) {
			if( (people[i] > rMax) && (people[i] <= out_max) )
				rMax = people[i];
			if( (people[i] <= rMin) && (people[i] >= out_min) )
				rMin = people[i];
		}
	}
	
	for(i=0; i<obs; i++) {	
		if(people[i] > out_max)	{
			outliers[i] = 2;
		} else if(people[i] < out_min) {
			outliers[i] = 1;
		} else {
			outliers[i] = 0; 
		}
	} 
}

void CartogramCanvas::ConstructCartogram(int n_iter)
{
    for(itter = 0; itter < n_iter; itter++)
	{
        for(body = 1;body < bodies; body++){
            tree[body].id = 0;
        }
        end_pointer = 1;
        for (body = 1;body < bodies; body++){
            add_point(1,1);
        }
        
        displacement = 0.0;

        for (body = 1;body < bodies; body++) {
            //get <number> of neighbours within <distance> inti <list[]>
            number = 0;
            distance = (int)(widest + radius[body]);
            get_point(1,1);

            xrepel = yrepel = 0.0;
            xattract = yattract = 0.0;
            closest = widest;
            
            //work out repelling force of overlapping neighbours
            if(number > 0) {
                for(nb = 1; nb <= number; nb++) {
                    other = list[nb];
                    if (other != body) {
                        xd = xl[other]-xl[body];
                        yd = yl[other]-yl[body];
                        dist = sqrt(xd*xd+yd*yd);
                        if(dist < closest)
                             closest = dist;
                        overlap = radius[body] + radius[other]-dist;
                        if(overlap > 0 && dist > 1) {
                             xrepel = xrepel-overlap*(xl[other]-xl[body])/dist;
                             yrepel = yrepel-overlap*(yl[other]-yl[body])/dist;
                        }
                    }
                }
            }

            for (nb = 1; nb <= nbours[body] ; nb++) {
                other = nbour[body*MAX_NEIGHBOR+nb];
                if(other != 0) { //opt this
                    xd = (xl[body]-xl[other]);
                    yd = (yl[body]-yl[other]);
                    dist = sqrt(xd*xd+yd*yd);
                    overlap = dist -radius[body] - radius[other];
                    if(overlap>0) {                      
                        overlap = overlap *
							border[body*MAX_NEIGHBOR+nb]/perimeter[body];
                        xattract = xattract+overlap*(xl[other]-xl[body])/dist;
                        yattract = yattract + overlap*(yl[other]-yl[body])/dist;
                    }
                }
            }

            atrdst = sqrt(xattract * xattract + yattract * yattract);
            repdst = sqrt(xrepel * xrepel+ yrepel * yrepel);
            if (repdst > closest){
                xrepel = closest * xrepel / (repdst +1.0);
                yrepel = closest * yrepel / (repdst +1.0);
                repdst = closest;
            }
            if(repdst > 0){
                xtotal = (1.0-ratio) * xrepel +
					ratio*(repdst*xattract/(atrdst+1.0));
                ytotal = (1.0-ratio) * yrepel +
					ratio*(repdst*yattract/(atrdst+1.0));
            } else {
                if (atrdst > closest) {
                    xattract = closest *xattract/(atrdst+1);
                    yattract = closest *yattract/(atrdst+1);
                }
                xtotal = xattract;
                ytotal = yattract;
            }
            xvector[body] = friction * (xvector[body]+xtotal);
            yvector[body] = friction * (yvector[body]+ytotal);
            displacement += sqrt(xtotal * xtotal +ytotal * ytotal);
        }

        for(body = 1;body < bodies;body++) {
            xl[body] += (xvector[body] + 0.5);
            yl[body] += (yvector[body] + 0.5);
        }
        
        done++;
        displacement = displacement / bodies;
    }

	for(body=1; body< bodies; body++) {
        loc_x[body-1] = xl[body];
		loc_y[body-1] = yl[body];
		radius[body-1] = radius[body];
	}

    xMin = (loc_x[0]-radius[0]);
    xMax = (loc_x[0]+radius[0]);
    yMin = (loc_y[0]-radius[0]);
    yMax = (loc_y[0]+radius[0]);

	for(int i=1; i<gObservation; i++) {
		if(xMin > (loc_x[i]-radius[i])) xMin = (loc_x[i]-radius[i]);
		if(xMax < (loc_x[i]+radius[i]))	xMax = (loc_x[i]+radius[i]);
		if(yMin > (loc_y[i]-radius[i]))	yMin = (loc_y[i]-radius[i]);
		if(yMax < (loc_y[i]+radius[i]))	yMax = (loc_y[i]+radius[i]);
	}

    CheckSize();
}


void CartogramCanvas::add_point(int pointer, int axis)
{
    if(tree[pointer].id == 0) {
		tree[pointer].id = body;
		tree[pointer].id = body;
		tree[pointer].left = 0;
		tree[pointer].right = 0;
		tree[pointer].xpos = xl[body];
		tree[pointer].ypos = yl[body];
		
    } else {
		if(axis == 1) {
			if(xl[body] >= tree[pointer].xpos) {
				if(tree[pointer].left == 0){ 
					end_pointer += 1;
					tree[pointer].left = end_pointer;
				}
				add_point(tree[pointer].left, 3-axis);
			} else {
				if(tree[pointer].right == 0) { 
					end_pointer +=1;
					tree[pointer].right = end_pointer;
				}
				add_point(tree[pointer].right, 3-axis);
			}
		} else {
			if (yl[body] >= tree[pointer].ypos) {
				if(tree[pointer].left == 0) {
					end_pointer += 1;
					tree[pointer].left = end_pointer;
				}
				add_point(tree[pointer].left, 3-axis);
			} else {
				if(tree[pointer].right == 0) { 
					end_pointer += 1;
					tree[pointer].right = end_pointer;
				}
				add_point(tree[pointer].right, 3-axis);
			}
		}
	}
}


void CartogramCanvas::get_point(int pointer, int axis)
{
	if (pointer > 0) {
		if (tree[pointer].id > 0) {
			if (axis == 1) { 
				if (xl[body]-distance < tree[pointer].xpos)
					get_point(tree[pointer].right, 3-axis);
				
				if (xl[body]+distance >= tree[pointer].xpos)
					get_point(tree[pointer].left, 3-axis);
			}
			if (axis == 2) {
				if (yl[body]-distance < tree[pointer].ypos)
					get_point(tree[pointer].right, 3-axis);
				
				if (yl[body] + distance >= tree[pointer].ypos)
					get_point(tree[pointer].left, 3-axis);
			}
			if ((xl[body]-distance < tree[pointer].xpos) && 
				(xl[body]+distance >= tree[pointer].xpos)) {
				if ((yl[body]-distance < tree[pointer].ypos) &&
				   (yl[body]+distance >= tree[pointer].ypos))
				{ 
					number += 1;
					list[number] = tree[pointer].id;
				}
			}
		}
	}
}


void CartogramCanvas::DrawAllCircles(wxDC* pDC)
{
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		wxColour    color=colors[cnt];
		if (outliers[cnt] == 2)
			color = wxColour(244,4,4);
		else if (outliers[cnt] == 1)
			color = wxColour(4,4,244);

		if (ValueFlag[cnt] == 0)			// Negatives Value
			color = wxColour(59,59,59);
		else if (ValueFlag[cnt] == 1) // Zeros
			color = wxColour(255,255,255);
	  
//		if(gSelection.selected(cnt))
//			color = highlight_color;

		DrawCircle(pDC, (int)(location.at(cnt).x),
				   (int)(location.at(cnt).y),
				   radius[cnt], color, cnt);
	}
}

void CartogramCanvas::DrawCircle(wxDC* s, const int x, const int y,
								 const double radius, const wxColour color,
								 const int cnt)
{
	 DrawCircles( s, x, y , radius*rFactor, color, cnt);
}


inline void CartogramCanvas::DrawCircles(wxDC* s, const int x, const int y,
										 const double circleSize,
										 const wxColour color, const int cnt)
{
	wxBrush brush;
	brush.SetColour(color);
	s->SetBrush(brush); 
	s->DrawCircle((int)x, (int)y, (int)circleSize);

	if(gSelection.selected(cnt)) {
		brush.SetColour(highlight_color);
		brush.SetStyle(wxCROSSDIAG_HATCH);
		s->SetBrush(brush); 
		s->DrawCircle((int)x, (int)y, (int)circleSize);
	}
}


/** Determine which polygons are within the selection rectangle. Push
    all selected rectangles onto the Selection stack, set the gEvent
    type to either ADD_SELECTION or NEW_SELECTION, notify Selection global
    instance gSelection
    using Selection::Update() and finally call MyFrame::UpdateWholeView().
 */
int	CartogramCanvas::SelectByRect(wxMouseEvent& event)
{
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

	if (event.ShiftDown()) gEvent= ADD_SELECTION;
	else gEvent= NEW_SELECTION;
  
	int mCnt= 0;
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (location.at(cnt).x >= p1.x && location.at(cnt).x <= p2.x &&
			location.at(cnt).y >= p1.y && location.at(cnt).y <= p2.y &&
			(gEvent == NEW_SELECTION || !gSelection.selected(cnt)))  {
				  gSelection.Push( cnt );
				  ++mCnt;
		}
	}
	if (mCnt == 0) {
		gEvent = NO_EVENTS;
	} else {
		gSelection.Update();
		frame->UpdateWholeView(NULL); 
		gSelection.Reset(true);
	}
	return mCnt;
}

void CartogramCanvas::SelectByPoint(wxMouseEvent& event)
{
	int Id = GeoDaConst::EMPTY;
	for (int cnt= 0; cnt < gObservation; ++cnt) {
		if (fabs(location.at(cnt).x - gSelect1.x) < radius[cnt]*rFactor &&
			fabs(location.at(cnt).y - gSelect2.y) < radius[cnt]*rFactor) {
			if((geoda_sqr(location.at(cnt).x-gSelect1.x) +
				geoda_sqr(location.at(cnt).y-gSelect1.y)) <
			   geoda_sqr(radius[cnt]*rFactor))	{
				Id= cnt;  
				break;
			}
		}
	}
	
	if (Id == GeoDaConst::EMPTY) {  
		EmptyClick();
		gSelection.Reset(true); // reset to empty
		gEvent = NEW_SELECTION;
		gSelection.Update();
		frame->UpdateWholeView(NULL);
		gSelection.Reset(true);
		return;
	}
	
	bool isSelected= gSelection.selected(Id);
    if (event.ShiftDown()) {
		gEvent = isSelected ? DEL_SELECTION : ADD_SELECTION;
	} else {
		gEvent = NEW_SELECTION;		
	}

	gSelection.Push( Id );
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}

void CartogramCanvas::Selection(wxDC* pDC)
{
	int cnt;
	switch (gEvent) {
		case NEW_SELECTION:
			DrawAllCircles(pDC);
			break;
		case ADD_SELECTION:
			cnt = gSelection.Pop();
			while (cnt != GeoDaConst::EMPTY) {
				wxColour col = colors[cnt];
				if (outliers[cnt] == 2)
					col = wxColour(244, 4, 4);
				else if (outliers[cnt] == 1)
					col = wxColour(4, 4, 244);

				if (ValueFlag[cnt] == 0) // Negatives Value
					col = wxColour(59, 59, 59);
				else if (ValueFlag[cnt] == 1) // Zeros
					col = wxColour(255, 255, 255);

				DrawCircle(pDC, (int) (location.at(cnt).x),
					(int) (location.at(cnt).y), radius[cnt], col, cnt);
				cnt = gSelection.Pop();
			}
			break;
		case DEL_SELECTION:
			while ((cnt = gSelection.Pop()) != GeoDaConst::EMPTY) {
				wxColour col = colors[cnt];

				if (outliers[cnt] == 2)
					col = wxColour(244, 4, 4);
				else if (outliers[cnt] == 1)
					col = wxColour(4, 4, 244);

				if (ValueFlag[cnt] == 0) // Negatives Value
					col = wxColour(59, 59, 59);
				else if (ValueFlag[cnt] == 1) // Zeros
					col = wxColour(255, 255, 255);

				DrawCircle(pDC, (int) (location.at(cnt).x),
					(int) (location.at(cnt).y), radius[cnt], col, cnt);
			}
			break;
		default:
			break;
	}
	gSelection.Reset();
}


BEGIN_EVENT_TABLE(CartogramLegend, TemplateLegend) 
	EVT_MOUSE_EVENTS(CartogramLegend::OnEvent)
END_EVENT_TABLE()

CartogramLegend::CartogramLegend(wxWindow *parent, const wxPoint& pos,
								 const wxSize& size,
								 wxString var1)
	: TemplateLegend(parent, pos, size)
{
	d_rect = 20; 
	px = 10;
	py = 40;
	m_w = 15;
	m_l = 20;

	Var1 = var1;
}

CartogramLegend::~CartogramLegend()
{ 
}

void CartogramLegend::OnDraw(wxDC& dc)
{
	int cur_y = py;
 
	wxFont m_font(*wxNORMAL_FONT);
	m_font.SetPointSize(11);
	dc.SetFont(m_font);


	dc.DrawText(Var1+ " : Circle Size", 5,15);

	dc.SetPen(*wxBLACK_PEN);
	wxBrush brush;
				
	brush.SetColour(wxColour(0,200,0));
	dc.SetBrush(brush);

	dc.DrawText("Normal",(px +m_l+10), cur_y - (m_w/2));
	dc.DrawRectangle(px,cur_y-3,m_l, m_w);
	cur_y += d_rect;


	brush.SetColour(wxColour(244,4,4));
	dc.SetBrush(brush);

	dc.DrawText("High outliers",(px +m_l+10), cur_y - (m_w/2));
	dc.DrawRectangle(px,cur_y-3,m_l, m_w);
	cur_y += d_rect;


	brush.SetColour(wxColour(4,4,244));
	dc.SetBrush(brush);

	dc.DrawText("Low outliers ",(px +m_l+10), cur_y - (m_w/2));
	dc.DrawRectangle(px,cur_y-3,m_l, m_w);
	cur_y += d_rect;		
	brush.SetColour(wxColour(255,255,255));
	dc.SetBrush(brush);

	dc.DrawText("Zeros",(px +m_l+10), cur_y - (m_w/2));
	dc.DrawRectangle(px,cur_y-3,m_l, m_w);
	cur_y += d_rect;


	brush.SetColour(wxColour(59,59,59));
	dc.SetBrush(brush);

	dc.DrawText("Negatives",(px +m_l+10), cur_y - (m_w/2));
	dc.DrawRectangle(px,cur_y-3,m_l, m_w);
	cur_y += d_rect;

}

void CartogramLegend::OnEvent(wxMouseEvent& event)
{
	if(event.RightUp()) {
		LOG_MSG("CarogramLegend::OnEvent, event.RightUp() == true");
		wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_CARTOGRAM_VIEW_MENU_LEGEND");
		((CartogramFrame*) template_frame)->UpdateMenuCheckMarks(optMenu);
		PopupMenu(optMenu, event.GetPosition());
	}
}	

