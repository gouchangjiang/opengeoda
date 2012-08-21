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

#ifndef __GEODA_CENTER_CONDITIONAL_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_VIEW_H__

#include <vector>
#include <wx/window.h>
#include <wx/sizer.h>

class ConditionalViewFrame;
class Project;
class DbfGridTableBase;

class ConditionalVariableXView : public TemplateCanvas
{
public:
	ConditionalVariableXView(const wxString& varXname,
							  const double* xVals,
							  wxWindow *parent, int num_obs,
							  const wxPoint& pos = wxDefaultPosition,
							  const wxSize& size = wxDefaultSize);

	ConditionalViewFrame* pFrame;
	void SetCuts();
    double xMax, xMin, xx[4];
	double xrMax, xrMin;
    wxString varX;
    double coordinate_handle[4];
    int sel;
	int num_obs;

    void OnEvent(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);

	virtual void CheckSize();
	void DrawLegend(wxDC *pDC);
	void DrawAxes(wxDC *pDC);

	virtual void OnDraw(wxDC& dc);

	void DragLinePlot(wxPoint point);
	inline void ClearLineFrame();
	int CheckHandle(wxPoint p);
	inline void DrawCircles(wxDC *s, const int x, const int y,
							const double circleSize, const wxColour color);

    DECLARE_EVENT_TABLE()
};


class ConditionalVariableYView : public TemplateCanvas
{
public:
	ConditionalVariableYView(const wxString& varYname,
							  const double* yVals,
							  wxWindow *parent, int num_obs,
							  const wxPoint& pos = wxDefaultPosition, 
							  const wxSize& size = wxDefaultSize);
	
	ConditionalViewFrame* pFrame;
	void SetCuts();
    double yMax, yMin, yy[4];
	double yrMax, yrMin;
    wxString varY;
    double coordinate_handle[4];
    int sel;
	int num_obs;
	
    void OnEvent(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);

	virtual void CheckSize();
	void DrawLegend(wxDC *pDC);
	void DrawAxes(wxDC *pDC);

	virtual void OnDraw(wxDC& dc);

	void DragLinePlot(wxPoint point);
	inline void ClearLineFrame();
	int CheckHandle(wxPoint p);
	inline void DrawCircles(wxDC *s, const int x, const int y,
							const double circleSize, const wxColour color);

    DECLARE_EVENT_TABLE()
};

class ConditionalVariableNULLView : public TemplateCanvas
{
public:
	ConditionalVariableNULLView(wxWindow *parent,
								 const wxPoint& pos = wxDefaultPosition,
								 const wxSize& size = wxDefaultSize);
	virtual void OnDraw(wxDC& dc) {};
	DECLARE_EVENT_TABLE()
};

class ConditionalVariableHeaderView : public TemplateCanvas
{
public:
	ConditionalVariableHeaderView(wxWindow *parent,
								   DbfGridTableBase* grid_base,
								   double* v3, const wxString& v3_name,
								   const wxPoint& pos = wxDefaultPosition,
								   const wxSize& size = wxDefaultSize);

    double vMax, vMin, vv[4];
    wxString var;
    double coordinate_handle[4];
    int sel;
	int num_obs;

    void OnSize(wxSizeEvent& event);

	virtual void CheckSize();
	void DrawLegend(wxDC *pDC);
	void DrawAxes(wxDC *pDC);
	virtual void OnDraw(wxDC& dc);

    DECLARE_EVENT_TABLE()
};

class MapSortElement
{
public:
	double value;
	double x;
	double y;
	int recordIndex;
	
	MapSortElement(double v, int i); 
	MapSortElement() {};
	bool operator > (MapSortElement& a);
	bool operator < (MapSortElement& a);
	MapSortElement& operator= (const MapSortElement& a);
};

class ConditionalViewFrame: public TemplateFrame
{
public:
    ConditionalViewFrame(wxFrame *parent,
						 Project* project,
						 double* v1, const wxString& v1_name,
						 double* v2, const wxString& v2_name,
						 double* v3, const wxString& v3_name,
						 double* v4, const wxString& v4_name,						 const wxString& title,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize,
						 const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalViewFrame();

	std::vector<wxWindow*> win_array; // 3x3=9 elem array of wxWindow pointers
	
	// either ConditionalVariableHeaderView or ConditionalVariableNULLView
	wxWindow* win_01_hdr_or_null;
	ConditionalVariableNULLView*	win_00_null;
	ConditionalVariableYView*		win_10_y_axis;
	ConditionalVariableNULLView*	win_20_null;
	ConditionalVariableXView*		win_21_x_axis;

	int mViewType;

	int *flags;
	double x1, x2, x3, x4, y1, y2, y3, y4;
	double *RawDataX, *RawDataY;
	int num_obs;
	
	void UpdateViews();

	void Update();

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();

	int Xbin;
	int Ybin;

	double xMin;
	double xMax;
	double yMin;
	double yMax;

	void XCategory();
	void YCategory();

	void QuickSortFieldValT(MapSortElement* array, int low, int high);
	void SortT(MapSortElement*, MapSortElement*, MapSortElement*);
	void SwapT(MapSortElement*, MapSortElement*);

    DECLARE_EVENT_TABLE()
};


inline float MIN(float r, float g, float b)
{
	if (r<g) {
		if (r<b) return r;
		return b;
	} else {
		if (g<b) return g;
		return b;
	}
}

inline float MAX(float r, float g, float b)
{
	if (r>g) {
		if (r>b) return r;
		return b;
	} else {
		if (g>b) return g;
		return b;
	}
}

inline void convert_rgb(float x, float y, float z, float* result)
{
	float h;
	float r=0, g=0, b=0;
	float f, p, q, t;
	int i;
	
	h = x;
	if (y == 0.0) {
		r = z;
		g = z;
		b = z;
	} else {
		if(h == 360.0)
			h = 0.0;
		h /= 60.0;
		i = (int) h;
		f = h - i;
		p = z * (1.0 - y);
		q = z * (1.0 - (y * f));
		t = z * (1.0 - y * (1.0 - f));
		
		switch(i) {
			case 0:
				r = z;  g = t;  b = p;
				break;
			case 1:
				r = q;  g = z;  b = p;
				break;
			case 2:
				r = p;  g = z;  b = t;
				break;
			case 3:
				r = p;  g = q;  b = z;
				break;
			case 4:
				r = t;  g = p;  b = z;
				break;
			case 5:
				r = z;  g = p;  b = q;
				break;
			default:
				r = 0.0; g = 0.0; b = 0.0;
				break;
		}
	}
	
	result[0] = r;
	result[1] = g;
	result[2] = b;
}


#endif

