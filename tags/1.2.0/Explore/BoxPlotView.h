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

#ifndef __GEODA_CENTER_BOX_PLOT_VIEW_H__
#define __GEODA_CENTER_BOX_PLOT_VIEW_H__

#include "../TemplateFrame.h"
#include "../TemplateCanvas.h"

class BoxPlotCanvas : public TemplateCanvas, public Conditionable
{
public:
    BoxPlotCanvas(wxWindow* parent,
				  double* v1, int num_obs, const wxString& v1_name,
				  const wxPoint& pos, const wxSize& size,
				  bool conditional_view = false);
	virtual ~BoxPlotCanvas();
    virtual void OnDraw(wxDC& dc);

    void OnEvent(wxMouseEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnSize(wxSizeEvent& event);

	virtual int	SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC);
    virtual void CheckSize();
	virtual void Draw(wxDC* pDC);

protected:
	int		circRadius;
    double	Density;
	double  Upper;
	double  Lower;
    int		mainBox; // pixel width of box
	
	int		Q1Ind, Q3Ind;
	double  MinVal, MaxVal, MedianVal, Q1Val, Q3Val, IQR, Mean;
	double	UpperWhisker, LowerWhisker;
	int		MedianIndex;
	
    double*	RawData;
	double* RRawData;
    int*	Index;
    int*	Invert;
	int*	mapper;

	wxString FieldName;

public:
    void CheckHinge(bool hinge3);
    void DrawBox(wxDC* pDC);
    void DrawRectangle(wxDC* dc, int left, int top, int right, int bottom,
					   const wxColour color, bool frameOnly);
    int FindProxy(const double Proxy);
    void DrawPoint(wxDC* pDC, const int elt);
    void DrawMedian(wxDC* pDC);
    bool Init();

	int num_obs;
	int gcObs;  // keeps track of the number of displayed observations.  This
                // can be less than num_obs when in Conditional Plot.

	bool hinge3; // default is false which indicates hinge is 1.5, not 3.0

	void OnOptionsHinge15();
	void OnOptionsHinge30();

	void UpdateCondition(int *flags);

    DECLARE_EVENT_TABLE()
};

class BoxPlotFrame: public TemplateFrame
{
public:
    BoxPlotCanvas *canvas;
    BoxPlotFrame(wxFrame *parent,
				 double* v1, int num_obs, const wxString& v1_name,
				 Project* project,
				 const wxString& title, const wxPoint& pos,
				 const wxSize& size, const long style);
    virtual ~BoxPlotFrame();

	wxString m_bpvarnm;

	void Update();

    void OnQuit(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	void UpdateMenuCheckMarks(wxMenu* menu);
	void UpdateMenuBarCheckMarks(wxMenuBar* menuBar);
	virtual void MapMenus();

	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
private:
	
    DECLARE_EVENT_TABLE()
};

#endif
