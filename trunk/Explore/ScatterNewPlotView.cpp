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

#include "ScatterNewPlotView.h"
#include "../logger.h"
#include <wx/xrc/xmlres.h>
#include "../OpenGeoDa.h"
#include "../Project.h"
#include <boost/foreach.hpp>
#include "../ShapeOperations/ShapeUtils.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../DataViewer/DbfGridTableBase.h"
#include <math.h>
#include <limits>
#include <cfloat>
#include <iostream>
#include <sstream>
#include <iomanip>

IMPLEMENT_CLASS(ScatterNewPlotFrame, TemplateFrame)
BEGIN_EVENT_TABLE(ScatterNewPlotFrame, TemplateFrame)
	EVT_ACTIVATE(ScatterNewPlotFrame::OnActivate)
END_EVENT_TABLE()

IMPLEMENT_CLASS(ScatterNewPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(ScatterNewPlotCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

ScatterNewPlotCanvas::ScatterNewPlotCanvas(wxWindow *parent,
										   const wxPoint& pos,
										   const wxSize& size,
										   const wxString& varX,
										   const wxString& varY,
										   const wxString& varZ,
										   bool is_bubble_plot,
										   bool standardized_s)
	: TemplateCanvas(parent, pos, size, false, true),
	varX(varX), varY(varY), varZ(varZ),
	is_bubble_plot(is_bubble_plot),
	axis_scale_x(), axis_scale_y(),
	standardized(standardized_s),
	reg_line(0), reg_line_text1(0), reg_line_text2(0), stats_table(0),
	reg_line_selected(0), reg_line_selected_text1(0),
	reg_line_selected_text2(0), reg_line_selected_slope(0),
	reg_line_selected_infinite_slope(false), reg_line_selected_defined(false),
	reg_line_excluded(0), reg_line_excluded_text1(0),
	reg_line_excluded_text2(0), reg_line_excluded_slope(0),
	reg_line_excluded_infinite_slope(false), reg_line_excluded_defined(false),
	x_axis_through_origin(0), y_axis_through_origin(0),
	show_origin_axes(true),
	selected_stats_text_X(0), excluded_stats_text_X(0),
	selected_stats_text_Y(0), excluded_stats_text_Y(0),
	display_stats(false),
	show_reg_selected(false),
	show_reg_excluded(false)
{
	using namespace Shapefile;
	
	LOG_MSG("Entering ScatterNewPlotCanvas::ScatterNewPlotCanvas");
	highlight_state = &(MyFrame::GetProject()->highlight_state);
	shps_orig_xmin = 0;
	shps_orig_ymin = 0;
	shps_orig_xmax = 100;
	shps_orig_ymax = 100;
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 50;
	virtual_screen_marg_left = 50;
	virtual_screen_marg_right = 25;

	PopulateCanvas(standardized);
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting ScatterNewPlotCanvas::ScatterNewPlotCanvas");
}

ScatterNewPlotCanvas::~ScatterNewPlotCanvas()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::~ScatterNewPlotCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting ScatterNewPlotCanvas::~ScatterNewPlotCanvas");
}

void ScatterNewPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::DisplayRightClickMenu");
	wxMenu* optMenu =
		wxXmlResource::Get()->
			LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting ScatterNewPlotCanvas::DisplayRightClickMenu");
}

/** This method assumes that varX and varY are already set and are valid.  It
 will populate the corresponding X and Y vectors of data from the table data,
 either standardized or not, and will recreate all canvas objects as needed
 and refresh the canvas. */
void ScatterNewPlotCanvas::PopulateCanvas(bool standardized)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	X.clear();
	Y.clear();
	Z.clear();

	this->standardized = standardized;
	wxSize size(GetVirtualSize());
	double scale_x, scale_y, trans_x, trans_y;
	MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
								   shps_orig_xmax, shps_orig_ymax,
								   virtual_screen_marg_top,
								   virtual_screen_marg_bottom,
								   virtual_screen_marg_left,
								   virtual_screen_marg_right,
								   size.GetWidth(), size.GetHeight(),
								   fixed_aspect_ratio_mode,
								   fit_to_window_mode,
								   &scale_x, &scale_y, &trans_x, &trans_y,
								   0, 0,
								   &current_shps_width, &current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;
	
	Project* project = MyFrame::GetProject();
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	int col1 = grid_base->FindColId(varX);
	int col2 = grid_base->FindColId(varY);
	int col3 = is_bubble_plot ? grid_base->FindColId(varZ) : wxNOT_FOUND;
	
	X.resize(project->GetNumRecords());
	Y.resize(project->GetNumRecords());
	if (is_bubble_plot) Z.resize(project->GetNumRecords());
	
	grid_base->col_data[col1]->GetVec(X);
	grid_base->col_data[col2]->GetVec(Y);
	if (is_bubble_plot) grid_base->col_data[col3]->GetVec(Z);
	
	statsX = SampleStatistics(X);
	statsY = SampleStatistics(Y);
	if (is_bubble_plot) statsZ = SampleStatistics(Z);
	if (standardized) {
		for (int i=0, iend=X.size(); i<iend; i++) {
			X[i] = (X[i]-statsX.mean)/statsX.sd_with_bessel;
			Y[i] = (Y[i]-statsY.mean)/statsY.sd_with_bessel;
			if (is_bubble_plot) Z[i] = (Z[i]-statsZ.mean)/statsZ.sd_with_bessel;
		}
		statsX = SampleStatistics(X);
		statsY = SampleStatistics(Y);
		if (is_bubble_plot) statsZ = SampleStatistics(Z);
		// mean shold be 0 and biased standard deviation should be 1
		double eps = 0.000001;
		if (-eps < statsX.mean && statsX.mean < eps) statsX.mean = 0;
		if (-eps < statsY.mean && statsY.mean < eps) statsY.mean = 0;
		if (is_bubble_plot) {
			if (-eps < statsZ.mean && statsZ.mean < eps) statsZ.mean = 0;
		}
	}
	LOG_MSG(wxString(statsX.ToString().c_str(), wxConvUTF8));
	LOG_MSG(wxString(statsY.ToString().c_str(), wxConvUTF8));
	if (is_bubble_plot)
		LOG_MSG(wxString(statsZ.ToString().c_str(), wxConvUTF8));
	regressionXY = SimpleLinearRegression(X, Y, statsX.mean, statsY.mean,
										  statsX.var_without_bessel,
										  statsY.var_without_bessel);
	LOG_MSG(wxString(regressionXY.ToString().c_str(), wxConvUTF8));
	
	axis_scale_x = AxisScale(statsX.min, statsX.max);
	LOG_MSG(wxString(axis_scale_x.ToString().c_str(), wxConvUTF8));
	axis_scale_y = AxisScale(statsY.min, statsY.max);
	LOG_MSG(wxString(axis_scale_y.ToString().c_str(), wxConvUTF8));
		
	// Populate TemplateCanvas::selectable_shps
	selectable_shps.resize(project->GetNumRecords());
	double scaleX = 100.0 / (axis_scale_x.scale_range);
	double scaleY = 100.0 / (axis_scale_y.scale_range);
	if (is_bubble_plot) {
		const double pi = 3.14159265;
		const double mean_rad = 18.0;
		const double mrs = mean_rad*mean_rad;
		const double mean_area = pi*mrs;
		const double mean_2rad = 30.0;
		const double mean_2area = pi*mean_2rad*mean_2rad;
		const double a=statsZ.mean;
		const double b=statsZ.mean + (2*statsZ.sd_without_bessel);
		const double A=mean_area; // area of mean point
		const double B=mean_2area; // area of point 2 sd above mean
		const double s = (B-A)/(b-a);
		const double c = A-(s*a);
		for (int i=0, iend=project->GetNumRecords(); i<iend; i++) {
			double area = (s*Z[i]) + c;
			double rad = sqrt(area/pi);
			if (area <= 0 || rad <= 2) {
				rad = 2;
			} else if (rad >= 40) {
				rad = 40;
			}
			selectable_shps[i] = 
			new MyCircle(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
									 (Y[i] - axis_scale_y.scale_min) * scaleY),
						 rad);
		}
	} else {
		for (int i=0, iend=project->GetNumRecords(); i<iend; i++) {
			selectable_shps[i] = 
			new MyPoint(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
									(Y[i] - axis_scale_y.scale_min) * scaleY));
		}
	}
	
	wxBrush t_brush(GeoDaConst::map_default_fill_colour);
	wxPen t_pen(GeoDaConst::map_default_outline_colour,
				GeoDaConst::map_default_outline_width);
	BOOST_FOREACH( MyShape* shp, selectable_shps ) {
		shp->pen = t_pen;
		shp->brush = t_brush;
	}
	
	// create axes
	x_baseline = new MyAxis(varX, axis_scale_x,
							wxPoint(0,0), wxPoint(100, 0));
	x_baseline->pen = *GeoDaConst::scatterplot_scale_pen;
	background_shps.push_front(x_baseline);
	y_baseline = new MyAxis(varY, axis_scale_y,
							wxPoint(0,0), wxPoint(0, 100));
	y_baseline->pen = *GeoDaConst::scatterplot_scale_pen;
	if (display_stats) {
		wxString x_cap;
		x_cap << varX << ", " << CreateStatsString(statsX);
		wxString y_cap;
		y_cap << varY << ", " << CreateStatsString(statsY);
		x_baseline->setCaption(x_cap);
		y_baseline->setCaption(y_cap);
	}
	background_shps.push_front(y_baseline);
	
	// create optional axes through origin
	x_axis_through_origin = new MyPolyLine(0,50,100,50);
	x_axis_through_origin->pen = *wxTRANSPARENT_PEN;
	y_axis_through_origin = new MyPolyLine(50,0,50,100);
	y_axis_through_origin->pen = *wxTRANSPARENT_PEN;
	background_shps.push_front(x_axis_through_origin);
	background_shps.push_front(y_axis_through_origin);	
	UpdateAxesThroughOrigin();
	
	// show regression lines
	reg_line = new MyPolyLine(0,100,0,100);
	double cc_degs_of_rot;
	double reg_line_slope;
	bool reg_line_infinite_slope;
	bool reg_line_defined;
	wxRealPoint a, b;
	CalcRegressionLine(*reg_line, reg_line_slope, reg_line_infinite_slope,
					   reg_line_defined, a, b, cc_degs_of_rot,
					   regressionXY, *GeoDaConst::scatterplot_reg_pen);
	foreground_shps.push_front(reg_line);
	if (reg_line_defined) {
		reg_line_text1 = new MyText(RegLineTextFromReg(regressionXY,1),
									*GeoDaConst::small_font, b, cc_degs_of_rot,
									MyText::right, MyText::bottom);
		reg_line_text2 = new MyText(RegLineTextFromReg(regressionXY,2),
									*GeoDaConst::small_font, b, cc_degs_of_rot,
									MyText::right, MyText::top);		
	} else {
		reg_line_text1 = new MyText();
		reg_line_text2 = new MyText();
	}
	reg_line_text1->pen = IsDisplayStats() ?
	*GeoDaConst::scatterplot_reg_pen : *wxTRANSPARENT_PEN;
	reg_line_text2->pen = IsDisplayStats() ?
	*GeoDaConst::scatterplot_reg_pen : *wxTRANSPARENT_PEN;
	foreground_shps.push_front(reg_line_text1);
	foreground_shps.push_front(reg_line_text2);
	// Create optional regression lines as transparent objects
	reg_line_selected = new MyPolyLine(0,100,0,100);
	reg_line_selected->pen = *wxTRANSPARENT_PEN;
	reg_line_selected->brush= *wxTRANSPARENT_BRUSH;
	foreground_shps.push_front(reg_line_selected);
	reg_line_excluded = new MyPolyLine(0,100,0,100);
	reg_line_excluded->pen = *wxTRANSPARENT_PEN;
	reg_line_excluded->brush= *wxTRANSPARENT_BRUSH;
	foreground_shps.push_front(reg_line_excluded);
	reg_line_selected_text1 = new MyText();
	reg_line_selected_text2 = new MyText();
	foreground_shps.push_front(reg_line_selected_text1);
	foreground_shps.push_front(reg_line_selected_text2);
	reg_line_excluded_text1 = new MyText();
	reg_line_excluded_text2 = new MyText();
	foreground_shps.push_front(reg_line_excluded_text1);
	foreground_shps.push_front(reg_line_excluded_text2);
	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		CalcStatsFromSelected(); // update both selected and excluded stats
	}
	if (IsRegressionSelected()) UpdateRegSelectedLine();
	if (IsRegressionExcluded()) UpdateRegExcludedLine();	
	
	// Create optional stats texts with a blank strings
	selected_stats_text_X = new MyText();
	excluded_stats_text_X = new MyText();
	selected_stats_text_Y = new MyText();
	excluded_stats_text_Y = new MyText();
	stats_table = new MyTable();
	stats_table->pen = *wxTRANSPARENT_PEN;
	
	UpdateDisplayStats();
	background_shps.push_front(selected_stats_text_X);
	background_shps.push_front(excluded_stats_text_X);
	background_shps.push_front(selected_stats_text_Y);
	background_shps.push_front(excluded_stats_text_Y);
	background_shps.push_front(stats_table);
	ResizeSelectableShps();
	
	LOG_MSG("Exiting ScatterNewPlotCanvas::PopulateCanvas");
}

/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void ScatterNewPlotCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::update");
	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		CalcStatsFromSelected(); // update both selected and excluded stats
		if (IsRegressionSelected()) UpdateRegSelectedLine();
		if (IsRegressionExcluded()) UpdateRegExcludedLine();
	}
	if (IsDisplayStats()) UpdateDisplayStats();
	
	// Call TemplateCanvas::update to redraw objects as needed.
	TemplateCanvas::update(o);
	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		// we only need to redraw everything if the optional
		// regression lines have changed.
		Refresh();
	}
	
	LOG_MSG("Entering ScatterNewPlotCanvas::update");	
}


void ScatterNewPlotCanvas::ViewStandardizedData()
{
	LOG_MSG("In ScatterNewPlotCanvas::ViewStandardizedData");
	PopulateCanvas(true);
}

void ScatterNewPlotCanvas::ViewOriginalData()
{
	LOG_MSG("In ScatterNewPlotCanvas::ViewOriginalData");
	PopulateCanvas(false);
}

void ScatterNewPlotCanvas::ViewRegressionSelected(bool display)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::ViewRegressionSelected");
	if (!display) {
		reg_line_selected->pen = *wxTRANSPARENT_PEN;
		reg_line_selected_text1->pen = *wxTRANSPARENT_PEN;
		reg_line_selected_text2->pen = *wxTRANSPARENT_PEN;
		if ((show_reg_selected && !show_reg_excluded) && display_stats) {
			// there is no longer anything showing, but there
			// was previously something showing
			virtual_screen_marg_left = 50;
			virtual_screen_marg_bottom = 50;
			show_reg_selected = false;
			UpdateDisplayStats();
			ResizeSelectableShps();
		} else {
			show_reg_selected = false;
			UpdateDisplayStats();
			Refresh();
		}
	} else {
		if ((!show_reg_selected && !show_reg_excluded) && display_stats) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_selected = true;
			virtual_screen_marg_left = 75;
			virtual_screen_marg_bottom = 140;
			PopulateCanvas(IsStandardized());
		} else {
			show_reg_selected = true;
			UpdateRegSelectedLine();
			UpdateDisplayStats();
			Refresh();
		}
	}
	LOG_MSG("Exiting ScatterNewPlotCanvas::ViewRegressionSelected");
}

void ScatterNewPlotCanvas::UpdateRegSelectedLine()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::UpdateRegSelectedLine");
	double cc_degs_of_rot;
	wxRealPoint a, b;
	CalcRegressionLine(*reg_line_selected, reg_line_selected_slope,
					   reg_line_selected_infinite_slope,
					   reg_line_selected_defined, a, b, cc_degs_of_rot,
					   regressionXYselected,
					   *GeoDaConst::scatterplot_reg_selected_pen);
	if (reg_line_selected_defined) {
		reg_line_selected_text1->operator=(
				MyText(RegLineTextFromReg(regressionXYselected,1),
					   *GeoDaConst::small_font, a, cc_degs_of_rot,
					   MyText::left, MyText::bottom));
		reg_line_selected_text1->pen =
		IsDisplayStats() ?
			*GeoDaConst::scatterplot_reg_selected_pen : *wxTRANSPARENT_PEN;
		reg_line_selected_text2->operator=(
				  MyText(RegLineTextFromReg(regressionXYselected,2),
						 *GeoDaConst::small_font, a, cc_degs_of_rot,
						 MyText::left, MyText::top));
		reg_line_selected_text2->pen =
		IsDisplayStats() ?
			*GeoDaConst::scatterplot_reg_selected_pen : *wxTRANSPARENT_PEN;
	} else {
		reg_line_selected_text1->operator=(MyText());
		reg_line_selected_text2->operator=(MyText());
	}
	ApplyLastResizeToShp(reg_line_selected);
	ApplyLastResizeToShp(reg_line_selected_text1);
	ApplyLastResizeToShp(reg_line_selected_text2);
	LOG_MSG("Exiting ScatterNewPlotCanvas::UpdateRegSelectedLine");	
}

void ScatterNewPlotCanvas::ViewRegressionSelectedExcluded(bool display)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::ViewRegressionExcluded");
	if (!display) {
		reg_line_excluded->pen = *wxTRANSPARENT_PEN;
		reg_line_excluded_text1->pen = *wxTRANSPARENT_PEN;
		reg_line_excluded_text2->pen = *wxTRANSPARENT_PEN;
		if ((!show_reg_selected && show_reg_excluded) && display_stats) {
			// there is no longer anything showing, but there
			// was previously something showing
			virtual_screen_marg_left = 50;
			virtual_screen_marg_bottom = 50;
			show_reg_excluded = false;
			UpdateDisplayStats();
			ResizeSelectableShps();
		} else {
			show_reg_excluded = false;
			UpdateDisplayStats();
			Refresh();
		}
	} else {
		if ((!show_reg_selected && !show_reg_excluded) && display_stats) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_excluded = true;
			virtual_screen_marg_left = 75;
			virtual_screen_marg_bottom = 140;
			PopulateCanvas(IsStandardized());
		} else {
			show_reg_excluded = true;
			UpdateRegExcludedLine();
			UpdateDisplayStats();
			Refresh();
		}
	}
	LOG_MSG("Exiting ScatterNewPlotCanvas::ViewRegressionExcluded");	
}

void ScatterNewPlotCanvas::UpdateRegExcludedLine()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::UpdateRegExcludedLine");
	double cc_degs_of_rot;
	wxRealPoint a, b;
	CalcRegressionLine(*reg_line_excluded, reg_line_excluded_slope,
					   reg_line_excluded_infinite_slope,
					   reg_line_excluded_defined, a, b, cc_degs_of_rot,
					   regressionXYexcluded,
					   *GeoDaConst::scatterplot_reg_excluded_pen);
	if (reg_line_excluded_defined) {
		reg_line_excluded_text1->operator=(
					MyText(RegLineTextFromReg(regressionXYexcluded,1),
						   *GeoDaConst::small_font, a, cc_degs_of_rot,
						   MyText::left, MyText::bottom));
		reg_line_excluded_text1->pen =
		IsDisplayStats() ?
			*GeoDaConst::scatterplot_reg_excluded_pen : *wxTRANSPARENT_PEN;
		reg_line_excluded_text2->operator=(
					MyText(RegLineTextFromReg(regressionXYexcluded,2),
						   *GeoDaConst::small_font, a, cc_degs_of_rot,
						   MyText::left, MyText::top));
		reg_line_excluded_text2->pen =
		IsDisplayStats() ?
			*GeoDaConst::scatterplot_reg_excluded_pen : *wxTRANSPARENT_PEN;
	} else {
		reg_line_excluded_text1->operator=(MyText());
		reg_line_excluded_text2->operator=(MyText());
	}
	ApplyLastResizeToShp(reg_line_excluded);
	ApplyLastResizeToShp(reg_line_excluded_text1);
	ApplyLastResizeToShp(reg_line_excluded_text2);
	LOG_MSG("Exiting ScatterNewPlotCanvas::UpdateRegExcludedLine");
}

void ScatterNewPlotCanvas::DisplayStatistics(bool display_stats_s)
{
	LOG_MSG("In ScatterNewPlotCanvas::DisplayStatistics");
	display_stats = display_stats_s;
	if (display_stats) {		
		if (regressionXY.valid)
			reg_line_text1->pen = *GeoDaConst::scatterplot_reg_pen;
		reg_line_text2->pen = *GeoDaConst::scatterplot_reg_pen;
		if (show_reg_selected || show_reg_excluded) {
			UpdateDisplayStats();
			virtual_screen_marg_bottom = 140;
			virtual_screen_marg_left = 75;
			if (reg_line_selected_defined) {
				reg_line_selected_text1->pen =
					*GeoDaConst::scatterplot_reg_selected_pen;
				reg_line_selected_text2->pen =
					*GeoDaConst::scatterplot_reg_selected_pen;
			}
			if (reg_line_excluded_defined) {
				reg_line_excluded_text1->pen =
					*GeoDaConst::scatterplot_reg_excluded_pen;
				reg_line_excluded_text2->pen =
					*GeoDaConst::scatterplot_reg_excluded_pen;
			}
		}
		wxString x_cap;
		x_cap << varX << ", " << CreateStatsString(statsX);
		wxString y_cap;
		y_cap << varY << ", " << CreateStatsString(statsY);
		x_baseline->setCaption(x_cap);
		y_baseline->setCaption(y_cap);
		ResizeSelectableShps();
	} else {
		reg_line_text1->pen = *wxTRANSPARENT_PEN;
		reg_line_text2->pen = *wxTRANSPARENT_PEN;
		reg_line_selected_text1->pen = *wxTRANSPARENT_PEN;
		reg_line_selected_text2->pen = *wxTRANSPARENT_PEN;
		reg_line_excluded_text1->pen = *wxTRANSPARENT_PEN;
		reg_line_excluded_text2->pen = *wxTRANSPARENT_PEN;
		selected_stats_text_X->pen = *wxTRANSPARENT_PEN;
		excluded_stats_text_X->pen = *wxTRANSPARENT_PEN;
		selected_stats_text_Y->pen = *wxTRANSPARENT_PEN;
		excluded_stats_text_Y->pen = *wxTRANSPARENT_PEN;
		virtual_screen_marg_bottom = 50;
		virtual_screen_marg_left = 50;
		x_baseline->setCaption(varX);
		y_baseline->setCaption(varY);
		ResizeSelectableShps();
	}
}

void ScatterNewPlotCanvas::ShowAxesThroughOrigin(bool show_origin_axes_s)
{
	LOG_MSG("In ScatterNewPlotCanvas::ShowAxesThroughOrigin");
	show_origin_axes = show_origin_axes_s;
	UpdateAxesThroughOrigin();
}
	

void ScatterNewPlotCanvas::CalcStatsFromSelected()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::CalcStatsFromSelected");
	// find mean for X and Y according to highlight_state for both
	// the currently selected, and the complement.
	statsXselected = SampleStatistics();
	statsYselected = SampleStatistics();
	statsXexcluded = SampleStatistics();
	statsYexcluded = SampleStatistics();
	regressionXYselected = SimpleLinearRegression();
	regressionXYexcluded = SimpleLinearRegression();
	int selected_cnt = 0;
	int excluded_cnt = 0;

	std::vector<bool>& hl = highlight_state->GetHighlight();
	// calculate mean, min and max
	statsXselected.min = std::numeric_limits<double>::max();
	statsYselected.min = std::numeric_limits<double>::max();
	statsXexcluded.min = std::numeric_limits<double>::max();
	statsYexcluded.min = std::numeric_limits<double>::max();
	statsXselected.max = std::numeric_limits<double>::min();
	statsYselected.max = std::numeric_limits<double>::min();
	statsXexcluded.max = std::numeric_limits<double>::min();
	statsYexcluded.max = std::numeric_limits<double>::min();
	for (int i=0, iend=X.size(); i<iend; i++) {
		if (hl[i]) {
			selected_cnt++;
			statsXselected.mean += X[i];
			statsYselected.mean += Y[i];
			if (X[i] < statsXselected.min) statsXselected.min = X[i];
			if (Y[i] < statsYselected.min) statsYselected.min = Y[i];
			if (X[i] > statsXselected.max) statsXselected.max = X[i];
			if (Y[i] > statsYselected.max) statsYselected.max = Y[i];
		} else {
			excluded_cnt++;
			statsXexcluded.mean += X[i];
			statsYexcluded.mean += Y[i];
			if (X[i] < statsXexcluded.min) statsXexcluded.min = X[i];
			if (Y[i] < statsYexcluded.min) statsYexcluded.min = Y[i];
			if (X[i] > statsXexcluded.max) statsXexcluded.max = X[i];
			if (Y[i] > statsYexcluded.max) statsYexcluded.max = Y[i];
		}
	}
	if (selected_cnt == 0) {
		statsXexcluded = statsX;
		statsYexcluded = statsY;
		regressionXYexcluded = regressionXY;
	} else if (excluded_cnt == 0) {
		statsXselected = statsX;
		statsYselected = statsY;
		regressionXYselected = regressionXY;
	} else {
		statsXselected.mean /= selected_cnt;
		statsYselected.mean /= selected_cnt;
		statsXexcluded.mean /= excluded_cnt;
		statsYexcluded.mean /= excluded_cnt;
		statsXselected.sample_size = selected_cnt;
		statsYselected.sample_size = selected_cnt;
		statsXexcluded.sample_size = excluded_cnt;
		statsYexcluded.sample_size = excluded_cnt;
	
		double sum_squaresXselected = 0;
		double sum_squaresYselected = 0;
		double sum_squaresXexcluded = 0;
		double sum_squaresYexcluded = 0;
		// calculate standard deviations and variances
		for (int i=0, iend=X.size(); i<iend; i++) {
			if (hl[i]) {
				sum_squaresXselected += X[i] * X[i];
				sum_squaresYselected += Y[i] * Y[i];
			} else {
				sum_squaresXexcluded += X[i] * X[i];
				sum_squaresYexcluded += Y[i] * Y[i];
			}
		}
	
		CalcVarSdFromSumSquares(statsXselected, sum_squaresXselected);
		CalcVarSdFromSumSquares(statsYselected, sum_squaresYselected);
		CalcVarSdFromSumSquares(statsXexcluded, sum_squaresXexcluded);
		CalcVarSdFromSumSquares(statsYexcluded, sum_squaresYexcluded);
	
		CalcRegressionSelOrExcl(statsXselected, statsYselected,
								regressionXYselected, true);
		CalcRegressionSelOrExcl(statsXexcluded, statsYexcluded,
								regressionXYexcluded, false);
	}
	
	LOG(wxString(regressionXYselected.ToString().c_str(), wxConvUTF8));
	LOG(wxString(regressionXYexcluded.ToString().c_str(), wxConvUTF8));
	LOG_MSG("Entering ScatterNewPlotCanvas::CalcStatsFromSelected");
}

void ScatterNewPlotCanvas::CalcVarSdFromSumSquares(SampleStatistics& ss,
												   double sum_squares)
{
	double n = ss.sample_size;
	ss.var_without_bessel = (sum_squares/n) - (ss.mean*ss.mean);
	ss.sd_without_bessel = sqrt(ss.var_without_bessel);
	
	if (ss.sample_size == 1) {
		ss.var_with_bessel = ss.var_without_bessel;
		ss.sd_with_bessel = ss.sd_without_bessel;
	} else {
		ss.var_with_bessel = (n/(n-1)) * ss.var_without_bessel;
		ss.sd_with_bessel = sqrt(ss.var_with_bessel);
	}
}

void ScatterNewPlotCanvas::CalcRegressionSelOrExcl(const SampleStatistics& ss_X,
												   const SampleStatistics& ss_Y,
												   SimpleLinearRegression& r,
												   bool selected)
{
	if (ss_X.sample_size != ss_Y.sample_size || ss_X.sample_size < 2 ||
		ss_X.var_without_bessel <= 4*DBL_MIN ) return;

	int n=0;
	double expectXY = 0;
	std::vector<bool>& hl = highlight_state->GetHighlight();
	double sum_x_squared = 0;
	if (selected) {
		for (int i=0, iend=X.size(); i<iend; i++) {
			if (hl[i]) {
				expectXY += X[i]*Y[i];
				sum_x_squared += X[i] * X[i];
				n++;
			}
		}
	} else {
		for (int i=0, iend=X.size(); i<iend; i++) {
			if (!hl[i]) {
				expectXY += X[i]*Y[i];
				sum_x_squared += X[i] * X[i];
				n++;
			}
		}
	}
	expectXY /= (double) ss_X.sample_size;
	
	r.covariance = expectXY - ss_X.mean * ss_Y.mean;
	r.beta = r.covariance / ss_X.var_without_bessel;
	double d = ss_X.sd_without_bessel * ss_Y.sd_without_bessel;
	if (d > 4*DBL_MIN) {
		r.correlation = r.covariance / d;
		r.valid_correlation = true;
	} else {
		r.valid_correlation = false;
	}
	
	r.alpha = ss_Y.mean - r.beta * ss_X.mean;
	r.valid = true;
	
	double SS_tot = ss_Y.var_without_bessel * ss_Y.sample_size;
	double SS_err = 0;
	double err=0;
	if (selected) {
		for (int i=0, iend=Y.size(); i<iend; i++) {
			if (hl[i]) {
				err = Y[i] - (r.alpha + r.beta * X[i]);
				SS_err += err * err;
			}
		}
	} else {
		for (int i=0, iend=Y.size(); i<iend; i++) {
			if (!hl[i]) {
				err = Y[i] - (r.alpha + r.beta * X[i]);
				SS_err += err * err;
			}
		}
	}
	if (SS_err < 16*DBL_MIN) {
		r.r_squared = 1;
	} else {
		r.r_squared = 1 - SS_err / SS_tot;
	}
	if (n>2 && ss_X.var_without_bessel > 4*DBL_MIN) {
		r.std_err_of_estimate = SS_err/(n-2); // SS_err/(n-k-1), k=1
		r.std_err_of_estimate = sqrt(r.std_err_of_estimate);
		r.std_err_of_beta = r.std_err_of_estimate/
			sqrt(n*ss_X.var_without_bessel);
		r.std_err_of_alpha = r.std_err_of_beta * sqrt(sum_x_squared / n);
		
		if (r.std_err_of_alpha >= 16*DBL_MIN) {
			r.t_score_alpha = r.alpha / r.std_err_of_alpha;
		} else {
			r.t_score_alpha = 100;
		}
		if (r.std_err_of_beta >= 16*DBL_MIN) {
			r.t_score_beta = r.beta / r.std_err_of_beta;
		} else {
			r.t_score_beta = 100;
		}
		r.p_value_alpha =
		SimpleLinearRegression::TScoreTo2SidedPValue(r.t_score_alpha, n-2);
		r.p_value_beta =
		SimpleLinearRegression::TScoreTo2SidedPValue(r.t_score_beta, n-2);

		r.valid_std_err = true;
	}
	
}

/** reg_line, slope, infinite_slope and regression_defined are all return
 values. */
void ScatterNewPlotCanvas::CalcRegressionLine(MyPolyLine& reg_line,
											  double& slope,
											  bool& infinite_slope,
											  bool& regression_defined,
											  wxRealPoint& reg_a,
											  wxRealPoint& reg_b,
											  double& cc_degs_of_rot,
											  const SimpleLinearRegression& reg,
											  const wxPen& pen)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::CalcRegressionLine");
	reg_line.brush = *wxTRANSPARENT_BRUSH; // ensure brush is transparent
	slope = 0; //default
	infinite_slope = false; // default
	regression_defined = true; // default

	if (!reg.valid) {
		regression_defined = false;
		reg_line.pen = *wxTRANSPARENT_PEN;
		return;
	}
	
	reg_a = wxRealPoint(0, 0);
	reg_b = wxRealPoint(0, 0);
	
	LOG(reg.beta);
	
	// bounding box is [axis_scale_x.scale_min, axis_scale_y.scale_max] x
	// [axis_scale_y.scale_min, axis_scale_y.scale_max]
	// By the constuction of the scale, we know that regression line is
	// not equal to any of the four bounding box lines.  Therefore, the
	// regression_line intersects the box at two unique points.  We must
	// determine the two points of interesection.
	if (reg.valid) {
		// It should be the case that the slope beta is at most 1/2.
		// So, we should calculate the points of intersection with the
		// two vertical bounding box lines.
		reg_a = wxRealPoint(axis_scale_x.scale_min,
							reg.alpha + reg.beta*axis_scale_x.scale_min);
		reg_b = wxRealPoint(axis_scale_x.scale_max,
							reg.alpha + reg.beta*axis_scale_x.scale_max);
		if (reg_a.y < axis_scale_y.scale_min) {
			reg_a.x = (axis_scale_y.scale_min - reg.alpha)/reg.beta;
			reg_a.y = axis_scale_y.scale_min;
		} else if (reg_a.y > axis_scale_y.scale_max) {
			reg_a.x = (axis_scale_y.scale_max - reg.alpha)/reg.beta;
			reg_a.y = axis_scale_y.scale_max;
		}
		if (reg_b.y < axis_scale_y.scale_min) {
			reg_b.x = (axis_scale_y.scale_min - reg.alpha)/reg.beta;
			reg_b.y = axis_scale_y.scale_min;
		} else if (reg_b.y > axis_scale_y.scale_max) {
			reg_b.x = (axis_scale_y.scale_max - reg.alpha)/reg.beta;
			reg_b.y = axis_scale_y.scale_max;
		}
		slope = reg.beta;
	} else {
		regression_defined = false;
		reg_line.pen = *wxTRANSPARENT_PEN;
		return;
	}
	
	// scaling factors to transform to screen coordinates.
	double scaleX = 100.0 / (axis_scale_x.scale_range);
	double scaleY = 100.0 / (axis_scale_y.scale_range);	
	reg_a.x = (reg_a.x - axis_scale_x.scale_min) * scaleX;
	reg_a.y = (reg_a.y - axis_scale_y.scale_min) * scaleY;
	reg_b.x = (reg_b.x - axis_scale_x.scale_min) * scaleX;
	reg_b.y = (reg_b.y - axis_scale_y.scale_min) * scaleY;
	
	reg_line = MyPolyLine(reg_a.x, reg_a.y, reg_b.x, reg_b.y);
	cc_degs_of_rot = RegLineToDegCCFromHoriz(reg_a.x, reg_a.y,
											 reg_b.x, reg_b.y);
	
	LOG(slope);
	LOG(infinite_slope);
	LOG(regression_defined);
	LOG(cc_degs_of_rot);
	LOG(reg_a.x);
	LOG(reg_a.y);
	LOG(reg_b.x);
	LOG(reg_b.y);
	
	reg_line.pen = pen;
	LOG_MSG("Exiting ScatterNewPlotCanvas::CalcRegressionLine");
}

/** This method builds up the display optional stats string from scratch every
 time. It assumes the calling function will do the screen Refresh. */
void ScatterNewPlotCanvas::UpdateDisplayStats()
{
	selected_stats_text_X->pen=*wxTRANSPARENT_PEN;
	excluded_stats_text_X->pen=*wxTRANSPARENT_PEN;	
	selected_stats_text_Y->pen=*wxTRANSPARENT_PEN;
	excluded_stats_text_Y->pen=*wxTRANSPARENT_PEN;	
	if (display_stats) {
		if (show_reg_selected && show_reg_excluded) {
			selected_stats_text_X->
			operator=(MyText(CreateStatsString(statsXselected),
							 *wxNORMAL_FONT,
							 wxRealPoint(0, 0), 0,
							 MyText::left, MyText::bottom, 0, 60));
			selected_stats_text_Y->
			operator=(MyText(CreateStatsString(statsYselected),
							 *wxNORMAL_FONT,
							 wxRealPoint(0, 0), 90,
							 MyText::left, MyText::top, -60, 0));
			excluded_stats_text_X->
			operator=(MyText(CreateStatsString(statsXexcluded),
							 *wxNORMAL_FONT,
							 wxRealPoint(100, 0), 0,
							 MyText::right, MyText::bottom, 0, 60));
			excluded_stats_text_Y->
			operator=(MyText(CreateStatsString(statsYexcluded),
							 *wxNORMAL_FONT,
							 wxRealPoint(0, 100), 90,
							 MyText::right, MyText::top, -60, 0));			
		} else if (show_reg_selected && !show_reg_excluded) {
			selected_stats_text_X->
			operator=(MyText(CreateStatsString(statsXselected),
							 *wxNORMAL_FONT,
							 wxRealPoint(50, 0), 0,
							 MyText::h_center, MyText::bottom, 0, 60));
			selected_stats_text_Y->
			operator=(MyText(CreateStatsString(statsYselected),
							 *wxNORMAL_FONT,
							 wxRealPoint(0, 50), 90,
							 MyText::h_center, MyText::top, -60, 0));			
		} else if (!show_reg_selected && show_reg_excluded) {
			excluded_stats_text_X->
			operator=(MyText(CreateStatsString(statsXexcluded),
							 *wxNORMAL_FONT,
							 wxRealPoint(50, 0), 0,
							 MyText::h_center, MyText::bottom, 0, 60));
			excluded_stats_text_Y->
			operator=(MyText(CreateStatsString(statsYexcluded),
							 *wxNORMAL_FONT,
							 wxRealPoint(0, 50), 90,
							 MyText::h_center, MyText::top, -60, 0));			
		}
		if (show_reg_selected) {
			selected_stats_text_X->pen=
				*GeoDaConst::scatterplot_reg_selected_pen;
			selected_stats_text_Y->pen=
				*GeoDaConst::scatterplot_reg_selected_pen;
		}
		if (show_reg_excluded) {
			excluded_stats_text_X->pen=
				*GeoDaConst::scatterplot_reg_excluded_pen;
			excluded_stats_text_Y->pen=
				*GeoDaConst::scatterplot_reg_excluded_pen;	
		}
		ApplyLastResizeToShp(selected_stats_text_X);
		ApplyLastResizeToShp(excluded_stats_text_X);
		ApplyLastResizeToShp(selected_stats_text_Y);
		ApplyLastResizeToShp(excluded_stats_text_Y);
		
		// fill out the regression stats table
		int rows = 2;
		if (show_reg_selected) rows++;
		if (show_reg_excluded) rows++;
		int cols = 9;
		std::vector<wxString> vals(rows*cols);
		std::vector<MyTable::CellAttrib> attributes(rows*cols);
		int i=0; int j=0;
		for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
			attributes[k].color = *wxBLACK;
		}
		vals[i*cols+j++] = "R^2";
		vals[i*cols+j++] = "const a";
		vals[i*cols+j++] = "std-err a";
		vals[i*cols+j++] = "t-stat a";
		vals[i*cols+j++] = "p-value a";
		vals[i*cols+j++] = "slope b";
		vals[i*cols+j++] = "std-err b";
		vals[i*cols+j++] = "t-stat b";
		vals[i*cols+j++] = "p-value b";
		i++; j=0;
		for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
			attributes[k].color = GeoDaConst::scatterplot_regression_color;
		}		
		vals[i*cols+j++] << DblToStr(regressionXY.r_squared);
		vals[i*cols+j++] << DblToStr(regressionXY.alpha);
		vals[i*cols+j++] << DblToStr(regressionXY.std_err_of_alpha);
		vals[i*cols+j++] << DblToStr(regressionXY.t_score_alpha);
		vals[i*cols+j++] << DblToStr(regressionXY.p_value_alpha);
		vals[i*cols+j++] << DblToStr(regressionXY.beta);
		vals[i*cols+j++] << DblToStr(regressionXY.std_err_of_beta);
		vals[i*cols+j++] << DblToStr(regressionXY.t_score_beta);
		vals[i*cols+j++] << DblToStr(regressionXY.p_value_beta);
		if (show_reg_selected) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color =
					GeoDaConst::scatterplot_regression_selected_color;
			}			
			vals[i*cols+j++] << DblToStr(regressionXYselected.r_squared);
			vals[i*cols+j++] << DblToStr(regressionXYselected.alpha);
			vals[i*cols+j++] << DblToStr(regressionXYselected.std_err_of_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYselected.t_score_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYselected.p_value_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYselected.beta);
			vals[i*cols+j++] << DblToStr(regressionXYselected.std_err_of_beta);
			vals[i*cols+j++] << DblToStr(regressionXYselected.t_score_beta);
			vals[i*cols+j++] << DblToStr(regressionXYselected.p_value_beta);
		}
		if (show_reg_excluded) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color =
				GeoDaConst::scatterplot_regression_excluded_color;
			}			
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.r_squared);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.alpha);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.std_err_of_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.t_score_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.p_value_alpha);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.beta);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.std_err_of_beta);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.t_score_beta);
			vals[i*cols+j++] << DblToStr(regressionXYexcluded.p_value_beta);		
		}
		int x_nudge = (virtual_screen_marg_left-virtual_screen_marg_right)/2;
		
		stats_table->operator=(MyTable(vals, attributes, rows, cols,
									   *GeoDaConst::small_font, wxRealPoint(50, 0),
									   MyText::h_center, MyText::top,
									   MyText::h_center, MyText::v_center,
									   3, 8, -x_nudge, 62));
		stats_table->pen = *wxBLACK_PEN;
		ApplyLastResizeToShp(stats_table);
	}
}

void ScatterNewPlotCanvas::UpdateAxesThroughOrigin()
{
	x_axis_through_origin->pen = *wxTRANSPARENT_PEN;
	y_axis_through_origin->pen = *wxTRANSPARENT_PEN;
	if (show_origin_axes &&
		axis_scale_y.scale_min < 0 && 0 < axis_scale_y.scale_max) {
		double y_inter = 100.0 * ((-axis_scale_y.scale_min) /
			(axis_scale_y.scale_max-axis_scale_y.scale_min));
		x_axis_through_origin->operator=(MyPolyLine(0,y_inter,100,y_inter));
		x_axis_through_origin->pen = *GeoDaConst::scatterplot_origin_axes_pen;
		ApplyLastResizeToShp(x_axis_through_origin);
	}
	if (show_origin_axes &&
		axis_scale_x.scale_min < 0 && 0 < axis_scale_x.scale_max) {
		double x_inter = 100.0 * ((-axis_scale_x.scale_min) /
			(axis_scale_x.scale_max-axis_scale_x.scale_min));
		y_axis_through_origin->operator=(MyPolyLine(x_inter,0,x_inter,100));
		y_axis_through_origin->pen = *GeoDaConst::scatterplot_origin_axes_pen;
		ApplyLastResizeToShp(y_axis_through_origin);
	}
}

wxString ScatterNewPlotCanvas::CreateStatsString(const SampleStatistics& s)
{
	std::stringstream ss;
	ss << std::setprecision(3);
	if (s.sample_size > 0) {
		ss << "[" << s.min << ", " << s.max << "], m=";
		ss << s.mean << ", sd=" << s.sd_with_bessel;
	} else {
		ss << "no observations";
	}
	return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString ScatterNewPlotCanvas::DblToStr(double x, int precision)
{
	std::stringstream ss;
	ss << std::setprecision(precision);
	ss << x;
	return wxString(ss.str().c_str(), wxConvUTF8);
}

/** This method will be used for adding text annotations to the displayed
 regression lines.  To avoid drawing text upside down, we will only
 returns values in the range [90,-90). The return value is the degrees
 of counter-clockwise rotation from the x-axis. */
double ScatterNewPlotCanvas::RegLineToDegCCFromHoriz(double a_x, double a_y,
													 double b_x, double b_y)
{	
	//LOG_MSG("Entering ScatterNewPlotCanvas::RegLineToDegCCFromHoriz");
	double dist = GenUtils::distance(wxRealPoint(a_x,a_y),
									 wxRealPoint(b_x,b_y));
	if (dist <= 4*DBL_MIN) return 0;
	// normalize slope vector c = (c_x, c_y)
	double x = (b_x - a_x) / dist;
	double y = (b_y - a_y) / dist;
	const double eps = 0.00001;
	if (-eps <= x && x <= eps) return 90;
	if (-eps <= y && y <= eps) return 0;
	
	//Recall: (x,y) = (cos(theta), sin(theta))  and  theta = acos(x)
	double theta = acos(x) * 57.2957796; // 180/pi = 57.2957796
	if (y < 0) theta = 360.0 - theta;

	//LOG_MSG("Exiting ScatterNewPlotCanvas::RegLineToDegCCFromHoriz");
	return theta;
}

wxString ScatterNewPlotCanvas::RegLineTextFromReg(
										const SimpleLinearRegression& reg,
										int line)
{
	using namespace std;
	stringstream ss;
	ss << setprecision(3);
	if (line == 1) {
		ss << "y=" << reg.alpha;
		if (reg.beta >= 0) {
			ss << "+" << reg.beta << "x";
		} else {
			ss << "-" << -reg.beta << "x";
		}
		ss << ", r2=" << reg.r_squared;
	}
	if (line == 2) {
		ss << "std errs a,b=";
		if (reg.valid_std_err) {
			ss << reg.std_err_of_alpha << "," << reg.std_err_of_beta;
			ss << ", t-stat a,b=" << reg.t_score_alpha << ",";
			ss << reg.t_score_beta;
			ss << ", p-vals a,b=" << reg.p_value_alpha << ",";
			ss << reg.p_value_beta;
		} else {
			ss << "undef";
		}
		//ss << ", corr=";
		//if (reg.valid_correlation) {
		//	ss << reg.correlation;
		//} else {
		//	ss << "undef";
		//}
	}
	return wxString(ss.str().c_str(), wxConvUTF8);
}

ScatterNewPlotFrame::ScatterNewPlotFrame(wxFrame *parent, Project* project,
										 const wxString& title,
										 const wxPoint& pos,
										 const wxSize& size,
										 const long style,
										 const wxString& varX,
										 const wxString& varY,
										 const wxString& varZ,
										 bool is_bubble_plot)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering ScatterNewPlotFrame::ScatterNewPlotFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
	template_canvas = new ScatterNewPlotCanvas(this, wxDefaultPosition,
											   wxSize(width,height),
											   varX, varY, varZ,
											   is_bubble_plot, false);
	template_canvas->template_frame = this;
	template_canvas->SetScrollRate(1,1);
	
	Show(true);
	LOG_MSG("Exiting ScatterNewPlotFrame::ScatterNewPlotFrame");
}

ScatterNewPlotFrame::~ScatterNewPlotFrame()
{
	LOG_MSG("In ScatterNewPlotFrame::~ScatterNewPlotFrame");
	DeregisterAsActive();
}

void ScatterNewPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("ScatterNewPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ScatterNewPlotFrame::MapMenus()
{
	LOG_MSG("In ScatterNewPlotFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::UpdateOptionMenuItems()
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateOptionMenuItems(); // set common items first

	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	ScatterNewPlotCanvas* canvas = (ScatterNewPlotCanvas*) template_canvas;
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  canvas->IsStandardized());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !canvas->IsStandardized());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_VIEW_REGRESSION_SELECTED"),
								  canvas->IsRegressionSelected());
	GeneralWxUtils::CheckMenuItem(mb,
								  XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
								  canvas->IsRegressionExcluded());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_DISPLAY_STATISTICS"),
								  canvas->IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
								  canvas->IsShowOriginAxes());
}

void ScatterNewPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items first

	ScatterNewPlotCanvas* canvas = (ScatterNewPlotCanvas*) template_canvas;
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  canvas->IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !canvas->IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_REGRESSION_SELECTED"),
								  canvas->IsRegressionSelected());
	GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
								  canvas->IsRegressionExcluded());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  canvas->IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
								  canvas->IsShowOriginAxes());	
}

void ScatterNewPlotFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnViewStandardizedData");
	((ScatterNewPlotCanvas*) template_canvas)->ViewStandardizedData();
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewOriginalData(wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnViewOriginalData");
	((ScatterNewPlotCanvas*) template_canvas)->ViewOriginalData();
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnViewRegressionSelected");
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ViewRegressionSelected(!t->IsRegressionSelected());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewRegressionSelectedExcluded(
														wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnViewRegressionSelectedExcluded");
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ViewRegressionSelectedExcluded(!t->IsRegressionExcluded());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnDisplayStatistics");
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnShowAxesThroughOrigin");
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ShowAxesThroughOrigin(!t->IsShowOriginAxes());
	UpdateOptionMenuItems();
}

