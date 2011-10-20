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

#ifndef __GEODA_CENTER_SCATTER_NEW_PLOT_VIEW_H__
#define __GEODA_CENTER_SCATTER_NEW_PLOT_VIEW_H__

#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GenUtils.h"
#include "../Generic/MyShape.h"

class ScatterNewPlotCanvas;
class ScatterNewPlotFrame;

class ScatterNewPlotCanvas : public TemplateCanvas {
	DECLARE_CLASS(ScatterNewPlotCanvas)	
public:
	ScatterNewPlotCanvas(wxWindow *parent,
						 Project* project,
						 const wxPoint& pos,
						 const wxSize& size,
						 const wxString& varX,
						 const wxString& varY,
						 const wxString& varZ,
						 bool is_bubble_plot,
						 bool standardized = false);
	virtual ~ScatterNewPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HighlightState* o);

	void ViewStandardizedData();
	void ViewOriginalData();
	void ViewRegressionSelected(bool display);
	void ViewRegressionSelectedExcluded(bool display);
	void DisplayStatistics(bool display_stats);
	void ShowAxesThroughOrigin(bool show_origin_axes);
	
	bool IsStandardized() { return standardized; }
	bool IsDisplayStats() { return display_stats; }
	bool IsShowOriginAxes() { return show_origin_axes; }
	bool IsRegressionSelected() { return show_reg_selected; } 
	bool IsRegressionExcluded() { return show_reg_excluded; }
	
protected:
	void PopulateCanvas(bool standardized);
	void CalcStatsFromSelected();
	void CalcVarSdFromSumSquares(SampleStatistics& ss, double sum_squares);
	void CalcRegressionSelOrExcl(const SampleStatistics& ss_X,
								 const SampleStatistics& ss_Y,
								 SimpleLinearRegression& r,
								 bool selected);
	void UpdateRegSelectedLine();
	void UpdateRegExcludedLine();
	void CalcRegressionLine(MyPolyLine& reg_line, // return value
							double& slope, // return value
							bool& infinite_slope, // return value
							bool& regression_defined, // return value
							wxRealPoint& a, // return value
							wxRealPoint& b, // return value
							double& cc_degs_of_rot, // return value
							const SimpleLinearRegression& reg,
							const wxPen& pen);
	void UpdateDisplayStats();
	void UpdateAxesThroughOrigin();
	static wxString CreateStatsString(const SampleStatistics& s);
	static wxString DblToStr(double x, int precision = 3);
	double RegLineToDegCCFromHoriz(double a_x, double a_y,
								   double b_x, double b_y);
	wxString RegLineTextFromReg(const SimpleLinearRegression& reg, int line);
	
	HighlightState* highlight_state;
	wxString varX;
	wxString varY;
	wxString varZ;
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	MyAxis* x_baseline;
	MyAxis* y_baseline;
	std::vector<double> X;
	std::vector<double> Y;
	std::vector<double> Z;
	bool is_bubble_plot;
	SampleStatistics statsX;
	SampleStatistics statsXselected;
	SampleStatistics statsXexcluded;
	SampleStatistics statsY;
	SampleStatistics statsYselected;
	SampleStatistics statsYexcluded;
	SampleStatistics statsZ;
	SimpleLinearRegression regressionXY;
	SimpleLinearRegression regressionXYselected;
	SimpleLinearRegression regressionXYexcluded;
	bool standardized;

	MyPolyLine* reg_line;
	MyTable* stats_table;
	
	bool show_reg_selected;
	MyPolyLine* reg_line_selected;
	double reg_line_selected_slope;
	bool reg_line_selected_infinite_slope;
	bool reg_line_selected_defined;

	bool show_reg_excluded;
	MyPolyLine* reg_line_excluded;
	double reg_line_excluded_slope;
	bool reg_line_excluded_infinite_slope;
	bool reg_line_excluded_defined;

	MyPolyLine* x_axis_through_origin;
	MyPolyLine* y_axis_through_origin;
	bool show_origin_axes;
	bool display_stats;
	
	DECLARE_EVENT_TABLE()
};

class ScatterNewPlotFrame : public TemplateFrame {
    DECLARE_CLASS(ScatterNewPlotFrame)
public:
    ScatterNewPlotFrame(wxFrame *parent, Project* project,
            const wxString& title,
            const wxPoint& pos, const wxSize& size,
            const long style,
            const wxString& varX, const wxString& varY,
            const wxString& varZ, bool is_bubble_plot);
    virtual ~ScatterNewPlotFrame();

public:
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);

    void OnViewStandardizedData(wxCommandEvent& event);
    void OnViewOriginalData(wxCommandEvent& event);
    void OnViewRegressionSelected(wxCommandEvent& event);
    void OnViewRegressionSelectedExcluded(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
    void OnShowAxesThroughOrigin(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};


#endif
