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

#ifndef __GEODA_CENTER_MAPVIEW_H__
#define __GEODA_CENTER_MAPVIEW_H__

#include <fstream>
#include <vector>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "ShapeOperations/shp2cnt.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"

typedef int* pInt;

class MapMovieFrame;
class MovieControlPan;
class MapMovieCanvas : public TemplateCanvas, public Conditionable
{
public:
	MapMovieCanvas(wxWindow *parent, int num_obs,
				   const wxPoint& pos=wxDefaultPosition,
				   const wxSize& size=wxDefaultSize,
				   bool conditional_view = false);
	virtual ~MapMovieCanvas();
	void OnPaint(wxPaintEvent& event);
	void AddMap(const wxString& path,
				double* v1, const wxString& v1_name);
	void Remove();

	void OnSize(wxSizeEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	
	bool isThereMap;

	void CheckSize();

	int m_type;  // Map Type
	double xMin, yMin, xMax, yMax;
	long n_total;
	std::vector<int> idx;
	std::vector<double> xs;
	std::vector<double> ys;
	std::vector<wxPoint> location;
	int Width, Height, Left, Right, Top, Bottom;
	double* RawData;
	int* hidx;
	bool bLink;

	int num_obs;
	int StarSize;

	std::vector<int> n_parts_per_cell;
	pInt* ppc;

	MapMovieFrame* myP;

	int current_frame;
	int type;

	void InvalidatePolygon(int id, bool s);
	void TimerCall();

	void UpdateCondition(int *flags);
    void Selection(wxDC* pDC);

	DECLARE_EVENT_TABLE()
};

class MapMovieTimer: public wxTimer
{
public:
	MapMovieTimer():wxTimer() {}

	MapMovieFrame* myP;
	virtual void Notify();
};

class MapMovieFrame: public TemplateFrame
{
public:	
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	void UpdateMenuCheckMarks(wxMenu* menu);

	MapMovieFrame(wxFrame *parent,
				  double* v1, int num_obs, const wxString& v1_name,
				  Project* project, int type,
				  const wxString& title, const wxPoint& pos,
				  const wxSize& size, const long style);
	virtual ~MapMovieFrame();
    
	MapMovieCanvas *canvas;
	MovieControlPan *control;
	MapMovieTimer *timer;	
	
	wxSplitterWindow* m_splitter;
	wxSplitterWindow* m_splitterMap;

	DECLARE_EVENT_TABLE()
};

#endif

