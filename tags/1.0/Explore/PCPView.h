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

#ifndef __GEODA_CENTER_PCP_VIEW_H__
#define __GEODA_CENTER_PCP_VIEW_H__

#include <vector>

typedef double *p_double;
typedef BasePoint *p_point;

class DbfGridTableBase;

class PCPCanvas : public TemplateCanvas {
public:
    PCPCanvas(const std::vector<int>& pcp_col_ids,
			  DbfGridTableBase* grid_base, 
			  wxWindow *parent,
			  const wxPoint& pos, const wxSize& size);
    virtual ~PCPCanvas();

    virtual void OnDraw(wxDC& dc);
    void OnEvent(wxMouseEvent& event);
    void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnSize(wxSizeEvent& event);

    bool BoxTest(int cnt, wxPoint p1, wxPoint p2);
    inline double LineToPointDist(int cnt, wxPoint p);
    void DrawLine(wxDC* s, const int cnt, const wxColour color);
    void SetDims();
    void ShowFirst();
    void DrawAllLines(wxDC *pDC);
    void DrawLegend(wxDC *pDC);
    void DrawAxes(wxDC *pDC);
    virtual void Draw(wxDC* pDC);
    p_double* data;
    p_double* sdata;
    p_double* rdata;
    int PCP1;
    int* l_map;

    double* mins;
    double* maxs;
    double* coordinate_handle;

    p_point* location;
    p_point p_mean;
    bool StandardizedData;

    int n_plp_sel;
	std::vector<wxString> plp_sel;
    int yGap1;
    int yGap2;

	enum PcpControlModes {
		pcp_mode_default, pcp_mode_ctrl_sel
	};
	int n_obs;
	int pcp_mode;
	int pcp_selected_ctrl_point;
	bool is_line_drag_visible;
	int line_drag_y_pos;
	void ErasePCPTempLine();
	void DrawPCPTempLine();
	
    virtual int SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC);
    virtual void CheckSize();

	void MoveBefore(int l1, int l2);
	void MoveToEnd(int l1);
    void SwitchLinePlot(int l1, int l2);
    inline void DrawCircles(wxDC *s, const int x, const int y,
        const double circleSize, const wxColour color);

    void ViewStandardizedData();
    void ViewOriginalData();

    DECLARE_EVENT_TABLE()
};

class PCPFrame : public TemplateFrame {
public:
    PCPCanvas *canvas;
    PCPFrame(const std::vector<int>& pcp_col_ids,
			 wxFrame *parent, Project* project, 
			 const wxString& title,
			 const wxPoint& pos, const wxSize& size,
			 const long style);
    virtual ~PCPFrame();

    void Update();

    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();

    void OnViewStandardizedData(wxCommandEvent& event);
    void OnViewOriginalData(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif

