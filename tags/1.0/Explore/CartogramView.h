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

#ifndef __GEODA_CENTER_CARTOGRAM_VIEW_H__
#define __GEODA_CENTER_CARTOGRAM_VIEW_H__

#include "../TemplateFrame.h"
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"

class CartogramCanvas : public TemplateCanvas {
public:
    CartogramCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size);
    virtual ~CartogramCanvas();
    virtual void OnDraw(wxDC& dc);

    void OnEvent(wxMouseEvent& event);
    void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnSize(wxSizeEvent& event);

    int MAX_NEIGHBOR;
    int done, bodys, itter;
    int nb, other;
    double boundary;
    double *people;
    double *people_temp;
    int *nbours, *nbour;
    double *xvector, *yvector, *border, *perimeter;

    wxColour *colors;

    GalElement* conti;
    double widest, displacement, closest;
    double xrepel, yrepel, xattract, yattract, scale;
    double dist, xd, yd, overlap, t_radius, t_dist;
    double atrdst, repdst, xtotal, ytotal;

    inline void DrawCircles(wxDC* s, const int x, const int y,
            const double circleSize, const wxColour color,
            const int cnt);
    double rFactor;
    void DrawCircle(wxDC* s, const int x, const int y, const double radius,
            const wxColour color, const int cnt);
    void DrawAllCircles(wxDC* pDC);
    void InitCartogram(const std::vector<double>& x11,
            const std::vector<double>& y11,
            double* RawData);
    void ConstructCartogram(int n_iter = 100);


    virtual int SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC);
    virtual void CheckSize();
    virtual void Draw(wxDC* pDC);

    double* loc_x;
    double* loc_y;
    double* radius;
    int* outliers;
    double xMin, xMax, yMin, yMax;
    bool m_HingeCheck15;
    bool m_HingeCheck30;
    long obs;
    int q[6];

    double map_to_people;
    double map_to_coorindate;

    double* RawData;
    std::vector<BasePoint> location;
    double size_factor;

    struct leaf *tree;

    int bodies;
    int body, number, end_pointer;
    double distance;
    double *xl, *yl;
    int *list;
    int* ValueFlag;

    void add_point(int pointer, int axis);
    void get_point(int pointer, int axis);

    void AdjustHinge(double Hinge);

private:
    DECLARE_EVENT_TABLE()
};

class CartogramLegend : public TemplateLegend {
public:
    CartogramLegend(wxWindow *parent, const wxPoint& pos,
					const wxSize& size, wxString var1);
    ~CartogramLegend();

    virtual void OnDraw(wxDC& dc);
    void OnEvent(wxMouseEvent& event);

    int px, py, m_w, m_l;
    int d_rect;

    wxString Var1;

    DECLARE_EVENT_TABLE()
};

class CartogramFrame : public TemplateFrame {
public:
    CartogramCanvas *canvas;
    CartogramLegend *legend;

    CartogramFrame(wxFrame *parent, Project* project, const wxString& title,
				   const wxPoint& pos, const wxSize& size, const long style);
    virtual ~CartogramFrame();

    wxSplitterWindow* m_splitter;

    void Update();

    void OnClose(wxCloseEvent& event);
    void OnMenuClose(wxCommandEvent& event);
    void OnActivate(wxActivateEvent& event);
    void UpdateMenuCheckMarks(wxMenu* menu);
    void UpdateMenuBarCheckMarks(wxMenuBar* menuBar);
    virtual void MapMenus();

    void OnQuit(wxCommandEvent& event);

    void OnMoreIter100(wxCommandEvent& event);
    void OnMoreIter500(wxCommandEvent& event);
    void OnMoreIter1000(wxCommandEvent& event);

    void OnHinge15(wxCommandEvent& event);
    void OnHinge30(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
};




#endif

