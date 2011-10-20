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

#ifndef __GEODA_CENTER_SCATTER_PLOT_VIEW_H__
#define __GEODA_CENTER_SCATTER_PLOT_VIEW_H__

#include <wx/dcgraph.h>
#include "../TemplateFrame.h"
#include "../TemplateCanvas.h"

class ScatterPlotCanvas : public TemplateCanvas, public Conditionable
{
public:
    ScatterPlotCanvas(wxWindow *parent,
					  const wxPoint& pos,
					  const wxSize& size,
					  bool conditional_view = false);
	virtual ~ScatterPlotCanvas();
    virtual void OnDraw(wxDC& dc);
	void OnMyPaint(wxPaintEvent& event);
	
    void OnEvent(wxMouseEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnSize(wxSizeEvent& event);

	enum  RegressionType  {
		REGRESS_NO,
		REGRESS_LINEAR,
		REGRESS_LOWESS,
		REGRESS_UNSELECTED,
	};

	vector<BasePoint> location;

	int gcObs; // might be less than gObservation if isCondionable==ture
	int	starSize;
	bool symmetricflag;
	double meanX, meanY, sdevX, sdevY;
	wxColour SpbackColor;
	
	wxColour BPbackColor;

protected:
	wxDC*			screen;
	void 			SlopeReport(wxDC* pDC, const char * header );
	bool 			excludeSelected;
	RegressionType  hasRegression;
	inline bool 	isLess(const double standard, const bool checkX);
	inline bool 	isGreater(const double standard, const bool checkX);
	void 			DrawAxes(wxDC* pDC);
	void 			DrawLegend(wxDC* pDC);
	void 			BuildMesh(const bool asymmetric= true);
	void 			ComputeRegression(const bool all= true);
	bool			Init();
	
	double     slope, intercept, xMin, xMax, yMin, yMax, xDensity, yDensity;
	double      	slopeX, interceptX;     // regression for a subsample
	DataPoint*  	RawData;
	int				xUnits, yUnits;
	bool    		regressionUnselected;
    wxString        Vertical, Horizontal;
	
	void DrawRegression(wxDC* pDC, const double rSlope,
						const double rIntercept, wxColour color, int PS);
	bool			StandardizedFlag;

public:
	void		Standardized();
	void		Destandardized();
	void		UpdateRegressionOption();
	void		CheckOption(const RegressionType rType);

	void		DrawAllPoints(wxDC* pDC);
	virtual int	SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC);
	virtual void CheckSize();
	virtual void Draw(wxDC* pDC);
	bool		Small() { return (Height < 100 || Width < 100);};

    bool  InTheArea(const BasePoint p, const BasePoint p1,
					const BasePoint p2) const  {
       return (p1.x <= p.x && p1.y <= p.y && p.x <= p2.x && p.y <= p2.y);
    } 

	void ViewStandardizedData();
	void ViewOriginalData();
	void ViewRegressionSelectedExcluded();

	void UpdateCondition(int *flags);
	bool	flick; // flag to help with refreshing calculations
	double  oldSlope;
	double  oldIntercept;
		
    DECLARE_EVENT_TABLE()
};

class ScatterPlotFrame: public TemplateFrame
{
public:
    ScatterPlotFrame(wxFrame *parent, Project* project, const wxString& title,
					 const wxPoint& pos, const wxSize& size, const long style);
    virtual ~ScatterPlotFrame();

	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();

	void Update();

	void OnViewStandardizedData(wxCommandEvent& event);
	void OnViewOriginalData(wxCommandEvent& event);
	void OnViewRegressionSelectedExcluded(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};


#endif

