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

#include <algorithm> // std::sort
#include <cfloat>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/math/distributions/fisher_f.hpp>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../DialogTools/MapQuantileDlg.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../OpenGeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ScatterNewPlotView.h"

IMPLEMENT_CLASS(ScatterNewPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(ScatterNewPlotCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

/** This is a more-basic constructor that does not populate the canvas. It
 is intended to be used in derived classes. */
ScatterNewPlotCanvas::ScatterNewPlotCanvas(wxWindow *parent,
										   TemplateFrame* t_frame,
										   Project* project_s,
										   const wxPoint& pos,
										   const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), num_obs(project_s->GetNumRecords()),
num_cats(1), num_time_vals(1),
highlight_state(project_s->highlight_state),
is_bubble_plot(false), axis_scale_x(), axis_scale_y(),
standardized(false), reg_line(0), stats_table(0),
reg_line_selected(0), reg_line_selected_slope(0),
reg_line_selected_infinite_slope(false), reg_line_selected_defined(false),
reg_line_excluded(0), reg_line_excluded_slope(0),
reg_line_excluded_infinite_slope(false), reg_line_excluded_defined(false),
x_axis_through_origin(0), y_axis_through_origin(0),
show_origin_axes(true), display_stats(false),
show_reg_selected(false), show_reg_excluded(false),
sse_c(0), sse_sel(0), sse_unsel(0),
chow_ratio(0), chow_pval(1), chow_valid(false), chow_test_text(0),
table_display_lines(0),
X(project_s->GetNumRecords()), Y(project_s->GetNumRecords()), Z(0),
obs_id_to_z_val_order(boost::extents[0][0])
{
	using namespace Shapefile;
	LOG_MSG("Entering ScatterNewPlotCanvas::ScatterNewPlotCanvas");
	template_frame = t_frame;
	use_category_brushes = true;
	draw_sel_shps_by_z_val = false;
	highlight_color = GeoDaConst::scatterplot_regression_selected_color;
	selectable_fill_color =
		GeoDaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GeoDaConst::scatterplot_regression_color;
		
	shps_orig_xmin = 0;
	shps_orig_ymin = 0;
	shps_orig_xmax = 100;
	shps_orig_ymax = 100;
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 50;
	virtual_screen_marg_left = 50;
	virtual_screen_marg_right = 25;
	
	UpdateDisplayLinesAndMargins();
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting ScatterNewPlotCanvas::ScatterNewPlotCanvas");
}

/** This constructor is intended to be used directly for creating new
 scatter plots and bubble charts */
ScatterNewPlotCanvas::ScatterNewPlotCanvas(wxWindow *parent,
										    TemplateFrame* t_frame,
										   Project* project_s,
										const std::vector<GeoDaVarInfo>& v_info,
										   const std::vector<int>& col_ids,
										   bool is_bubble_plot_s,
										   bool standardized_s,
										   const wxPoint& pos,
										   const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), var_info(v_info), num_obs(project_s->GetNumRecords()),
num_cats(is_bubble_plot ? 3 : 1), num_time_vals(1),
data(v_info.size()),
highlight_state(project_s->highlight_state),
is_bubble_plot(is_bubble_plot_s),
axis_scale_x(), axis_scale_y(),
standardized(standardized_s), reg_line(0), stats_table(0),
reg_line_selected(0), reg_line_selected_slope(0),
reg_line_selected_infinite_slope(false), reg_line_selected_defined(false),
reg_line_excluded(0), reg_line_excluded_slope(0),
reg_line_excluded_infinite_slope(false), reg_line_excluded_defined(false),
x_axis_through_origin(0), y_axis_through_origin(0),
show_origin_axes(true), display_stats(false),
show_reg_selected(false), show_reg_excluded(false),
sse_c(0), sse_sel(0), sse_unsel(0),
chow_ratio(0), chow_pval(1), chow_valid(false), chow_test_text(0),
table_display_lines(0),
X(project_s->GetNumRecords()), Y(project_s->GetNumRecords()),
Z(is_bubble_plot_s ? project_s->GetNumRecords() : 0),
obs_id_to_z_val_order(boost::extents[0][0])
{
	using namespace Shapefile;
	LOG_MSG("Entering ScatterNewPlotCanvas::ScatterNewPlotCanvas");
	template_frame = t_frame;
	
	DbfGridTableBase* grid_base = project->GetGridBase();

	for (int i=0; i<var_info.size(); i++) {
		grid_base->GetColData(col_ids[i], data[i]);
	}
	
	if (!is_bubble_plot) {
		highlight_color = GeoDaConst::scatterplot_regression_selected_color;
		selectable_fill_color =
			GeoDaConst::scatterplot_regression_excluded_color;
		selectable_outline_color = GeoDaConst::scatterplot_regression_color;
	}
	
	if (is_bubble_plot) {
		GeoDa::dbl_int_pair_vec_type v_sorted(num_obs);
		int times = var_info[2].is_time_variant ? grid_base->time_steps : 1;
		obs_id_to_z_val_order.resize(boost::extents[times][num_obs]);
		
		for (int t=0; t<times; t++) {
			for (int i=0; i<num_obs; i++) {
				v_sorted[i].first = data[2][t][i];
				v_sorted[i].second = i;
			}
			std::sort(v_sorted.begin(), v_sorted.end(),
					  GeoDa::dbl_int_pair_cmp_greater);
			for (int i=0; i<num_obs; i++) {
				obs_id_to_z_val_order[t][v_sorted[i].second] = i;
			}
		}
	}

	shps_orig_xmin = 0;
	shps_orig_ymin = 0;
	shps_orig_xmax = 100;
	shps_orig_ymax = 100;
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 50;
	virtual_screen_marg_left = 50;
	virtual_screen_marg_right = 25;
	
	use_category_brushes = true;
	draw_sel_shps_by_z_val = is_bubble_plot;
	UpdateDisplayLinesAndMargins();

	if (is_bubble_plot) {
		ChangeThemeType(ThemeUtilities::stddev);
	} else {
		ref_var_index = -1;
		num_time_vals = 1;
		for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
			if (var_info[i].is_ref_variable) ref_var_index = i;
		}
		if (ref_var_index != -1) {
			num_time_vals = (var_info[ref_var_index].time_max -
							 var_info[ref_var_index].time_min) + 1;
		}
		
		CreateCategoriesAllCanvasTms(1, num_time_vals); // 1 = #cats
		for (int t=0; t<num_time_vals; t++) {
			SetCategoryColor(t, 0, selectable_fill_color);
			for (int i=0; i<num_obs; i++) {
				AppendIdToCategory(t, 0, i);
			}
		}
		if (ref_var_index != -1) {
			SetCurrentCanvasTmStep(var_info[ref_var_index].time
								   - var_info[ref_var_index].time_min);
		}
		VarInfoAttributeChange();
		PopulateCanvas();
	}
	
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
	wxMenu* optMenu;
	if (is_bubble_plot) {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_BUBBLE_CHART_VIEW_MENU_OPTIONS");
	} else {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	}
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting ScatterNewPlotCanvas::DisplayRightClickMenu");
}

void ScatterNewPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
				menu1->AppendCheckItem(GeoDaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}
	
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	if (var_info[0].is_time_variant) {
		wxString s;
		s << "Fixed x-axis scale over time";
		wxMenuItem* mi =
		menu2->AppendCheckItem(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1, s, s);
		mi->Check(var_info[0].fixed_scale);
	}
	if (var_info[1].is_time_variant) {
		wxString s;
		s << "Fixed y-axis scale over time";
		wxMenuItem* mi =
		menu2->AppendCheckItem(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR2, s, s);
		mi->Check(var_info[1].fixed_scale);
	}
	
	menu->Prepend(wxID_ANY, "Scale Options", menu2, "Scale Options");
	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

void ScatterNewPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_REGRESSION_SELECTED"),
								  IsRegressionSelected());
	GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
								  IsRegressionExcluded());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
								  IsShowOriginAxes());
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
								  theme_type == ThemeUtilities::no_theme);	
	GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
								  theme_type == ThemeUtilities::quantile);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
								  theme_type == ThemeUtilities::percentile);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"),
								  theme_type == ThemeUtilities::hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"),
								  theme_type == ThemeUtilities::hinge_30);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
								  theme_type == ThemeUtilities::stddev);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
								  theme_type == ThemeUtilities::unique_values);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_EQUAL_INTERVALS"),
								  theme_type ==ThemeUtilities::equal_intervals);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
								  theme_type == ThemeUtilities::natural_breaks);
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

wxString ScatterNewPlotCanvas::GetCanvasTitle()
{
	DbfGridTableBase* gb = project->GetGridBase();
	wxString s(is_bubble_plot ? "Bubble Chart" : "Scatter Plot");	
	s << " - x: " << GetNameWithTime(0) << ", y: " << GetNameWithTime(1);
	if (is_bubble_plot) {
		s << ", size: " << GetNameWithTime(2);
		s << ", " << GetCategoriesTitle();
	}
	return s;
}

wxString ScatterNewPlotCanvas::GetCategoriesTitle()
{
	if (theme_type == ThemeUtilities::no_theme) {
		return "Themeless";
	}
	wxString s;
	s << ThemeUtilities::ThemeTypeToString(theme_type);
	if (is_bubble_plot) {
		s << ": " << GetNameWithTime(3);
	}
	return s;
}

wxString ScatterNewPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification. */
void ScatterNewPlotCanvas::ChangeThemeType(
									ThemeUtilities::ThemeType new_map_theme)
{
	theme_type = new_map_theme;
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::OnSaveCategories()
{
	wxString t_name = ThemeUtilities::ThemeTypeToString(theme_type);
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
	SaveCategories(title, label, "CATEGORIES");
}

void ScatterNewPlotCanvas::SetHighlightColor(wxColour color)
{	
	highlight_color = color;
	//GeoDaConst::scatterplot_reg_selected_pen->SetColour(highlight_color);
	UpdateRegSelectedLine();
	UpdateDisplayStats();
	TemplateCanvas::SetHighlightColor(color);
}

void ScatterNewPlotCanvas::SetSelectableFillColor(wxColour color)
{
	if (!is_bubble_plot) {
		selectable_fill_color = color;
		//GeoDaConst::scatterplot_reg_excluded_pen->
		//	SetColour(selectable_fill_color);
		for (int t=0; t<GetCanvasTmSteps(); t++) {
			SetCategoryColor(t, 0, selectable_fill_color);
		}
		UpdateRegExcludedLine();
		UpdateDisplayStats();
	}
	TemplateCanvas::SetSelectableFillColor(color);
}

void ScatterNewPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
	if (!is_bubble_plot) {
		selectable_outline_color = color;
		//GeoDaConst::scatterplot_reg_pen->SetColour(selectable_outline_color);
		if (reg_line) {
			reg_line->setPen(selectable_outline_color);
		}
		UpdateDisplayStats();
	}
	TemplateCanvas::SetSelectableOutlineColor(color);
}

/** This method assumes that x and y names are already set and are valid. It
 will populate the corresponding X and Y vectors of data from the table data,
 either standardized or not, and will recreate all canvas objects as needed
 and refresh the canvas. */
void ScatterNewPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
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
	
	LOG(x_data.shape()[0]);
	LOG(x_data.shape()[1]);
	LOG(X.size());
	LOG(y_data.shape()[0]);
	LOG(y_data.shape()[1]);
	LOG(Y.size());
	int xt = var_info[0].time-var_info[0].time_min;
	int yt = var_info[1].time-var_info[1].time_min;
	LOG(xt);
	LOG(yt);
	for (int i=0; i<num_obs; i++) {
		X[i] = x_data[xt][i];
		Y[i] = y_data[yt][i];
	}
	if (is_bubble_plot) {
		int zt = var_info[2].time-var_info[2].time_min;
		for (int i=0; i<num_obs; i++) {
			Z[i] = z_data[zt][i];
		}
	}
	
	// global scaling only supported for non-standardized values at this time
	double x_max = var_info[0].max_over_time;
	double x_min = var_info[0].min_over_time;
	double y_max = var_info[1].max_over_time;
	double y_min = var_info[1].min_over_time;	
	
	statsX = SampleStatistics(X);
	statsY = SampleStatistics(Y);
	if (is_bubble_plot) statsZ = SampleStatistics(Z);
	if (standardized) {
		for (int i=0, iend=X.size(); i<iend; i++) {
			X[i] = (X[i]-statsX.mean)/statsX.sd_with_bessel;
			Y[i] = (Y[i]-statsY.mean)/statsY.sd_with_bessel;
			if (is_bubble_plot) Z[i] = (Z[i]-statsZ.mean)/statsZ.sd_with_bessel;
		}
		// we are ignoring the global scaling option here
		x_max = (statsX.max - statsX.mean)/statsX.sd_with_bessel;
		x_min = (statsX.min - statsX.mean)/statsX.sd_with_bessel;
		y_max = (statsY.max - statsY.mean)/statsY.sd_with_bessel;
		y_min = (statsY.min - statsY.mean)/statsY.sd_with_bessel;
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
	
	//LOG_MSG(wxString(statsX.ToString().c_str(), wxConvUTF8));
	//LOG_MSG(wxString(statsY.ToString().c_str(), wxConvUTF8));
	if (is_bubble_plot) {
		//LOG_MSG(wxString(statsZ.ToString().c_str(), wxConvUTF8));
	}
	regressionXY = SimpleLinearRegression(X, Y, statsX.mean, statsY.mean,
										  statsX.var_without_bessel,
										  statsY.var_without_bessel);
	sse_c = regressionXY.error_sum_squares;
	//LOG_MSG(wxString(regressionXY.ToString().c_str(), wxConvUTF8));
	
	if (!var_info[0].fixed_scale && !standardized) {
		x_max = var_info[0].max[var_info[0].time];
		x_min = var_info[0].min[var_info[0].time];
	}
	if (!var_info[1].fixed_scale && !standardized) {
		y_max = var_info[1].max[var_info[1].time];
		y_min = var_info[1].min[var_info[1].time];
	}
	
	double x_pad = 0.1 * (x_max - x_min);
	double y_pad = 0.1 * (y_max - y_min);
	axis_scale_x = AxisScale(x_min - x_pad, x_max + x_pad);
	axis_scale_y = AxisScale(y_min - y_pad, y_max + y_pad);
	
	// used by status bar for showing selection rectangle range
	data_scale_xmin = axis_scale_x.scale_min;
	data_scale_xmax = axis_scale_x.scale_max;
	data_scale_ymin = axis_scale_y.scale_min;
	data_scale_ymax = axis_scale_y.scale_max;
	
	//LOG_MSG(wxString(axis_scale_x.ToString().c_str(), wxConvUTF8));
	//LOG_MSG(wxString(axis_scale_y.ToString().c_str(), wxConvUTF8));
		
	// Populate TemplateCanvas::selectable_shps
	selectable_shps.resize(num_obs);
	double scaleX = 100.0 / (axis_scale_x.scale_range);
	double scaleY = 100.0 / (axis_scale_y.scale_range);
	if (is_bubble_plot) {
		selectable_shps_type = circles;
		
		const double pi = 3.14159265;
		const double rad_mn = 10;
		const double area_mn = pi * rad_mn * rad_mn;
		const double rad_sd = 20;
		const double area_sd = pi * rad_sd * rad_sd;
		const double a = area_sd - area_mn;
		const double b = area_mn;
		const double min_rad = 3;
		const double min_area = pi * min_rad * min_rad;
		wxRealPoint pt;
		if (statsZ.max-statsZ.min <= 0.00000001 ||
			statsZ.var_without_bessel == 0) {
			for (int i=0; i<num_obs; i++) {
				pt.x = (X[i] - axis_scale_x.scale_min) * scaleX;
				pt.y = (Y[i] - axis_scale_y.scale_min) * scaleY;
				selectable_shps[i] = new MyCircle(pt, rad_mn);
			}
		} else {
			for (int i=0; i<num_obs; i++) {
				double z = (Z[i] - statsZ.mean)/statsZ.sd_without_bessel;
				double area_z = (a*z + b) + min_area + area_sd;
				if (area_z < min_area) area_z = min_area;
				double r = sqrt(area_z/pi);
				pt.x = (X[i] - axis_scale_x.scale_min) * scaleX;
				pt.y = (Y[i] - axis_scale_y.scale_min) * scaleY;
				selectable_shps[i] = new MyCircle(pt, r);
			}
		}
	} else {
		selectable_shps_type = points;
		for (int i=0; i<num_obs; i++) {
			selectable_shps[i] = 
			new MyPoint(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
									(Y[i] - axis_scale_y.scale_min) * scaleY));
		}
	}
	
	// create axes
	x_baseline = new MyAxis(GetNameWithTime(0), axis_scale_x,
							wxRealPoint(0,0), wxRealPoint(100, 0));
	x_baseline->setPen(*GeoDaConst::scatterplot_scale_pen);
	background_shps.push_back(x_baseline);
	y_baseline = new MyAxis(GetNameWithTime(1), axis_scale_y,
							wxRealPoint(0,0), wxRealPoint(0, 100));
	y_baseline->setPen(*GeoDaConst::scatterplot_scale_pen);
	background_shps.push_back(y_baseline);
	
	// create optional axes through origin
	x_axis_through_origin = new MyPolyLine(0,50,100,50);
	x_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	y_axis_through_origin = new MyPolyLine(50,0,50,100);
	y_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	background_shps.push_back(x_axis_through_origin);
	background_shps.push_back(y_axis_through_origin);
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
					   regressionXY, wxPen(selectable_outline_color));
	reg_line_selected = new MyPolyLine(0,100,0,100);
	reg_line_selected->setPen(*wxTRANSPARENT_PEN);
	reg_line_selected->setBrush(*wxTRANSPARENT_BRUSH);
	reg_line_excluded = new MyPolyLine(0,100,0,100);
	reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
	reg_line_excluded->setBrush(*wxTRANSPARENT_BRUSH);

	foreground_shps.push_back(reg_line);
	foreground_shps.push_back(reg_line_selected);
	foreground_shps.push_back(reg_line_excluded);

	if (is_bubble_plot) {
		reg_line->setPen(*wxTRANSPARENT_PEN);
		reg_line_selected->setPen(*wxTRANSPARENT_PEN);
		reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
	}	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		CalcStatsFromSelected(); // update both selected and excluded stats
	}
	if (IsRegressionSelected()) UpdateRegSelectedLine();
	if (IsRegressionExcluded()) UpdateRegExcludedLine();

	chow_test_text = new MyText();
	chow_test_text->hidden = true;
	foreground_shps.push_back(chow_test_text);
	stats_table = new MyTable();
	stats_table->hidden = true;
	foreground_shps.push_back(stats_table);
	if (!is_bubble_plot) {
		UpdateDisplayStats();
	}
	
	PopCanvPreResizeShpsHook();
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting ScatterNewPlotCanvas::PopulateCanvas");
}

void ScatterNewPlotCanvas::PopCanvPreResizeShpsHook()
{
}

void ScatterNewPlotCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::TitleOrTimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetGridBase()->curr_time_step;
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min)) return;
	if (cts > ref_time_max) {
		ref_time = ref_time_max;
	} else if (cts < ref_time_min) {
		ref_time = ref_time_min;
	} else {
		ref_time = cts;
	}
	if (var_info[0].sync_with_global_time) {
		var_info[0].time = ref_time + var_info[0].ref_time_offset;
	}
	if (var_info[1].sync_with_global_time) {
		var_info[1].time = ref_time + var_info[1].ref_time_offset;
	}
	if (is_bubble_plot && var_info[2].sync_with_global_time) {
		var_info[2].time = ref_time + var_info[2].ref_time_offset;
	}
	if (is_bubble_plot && var_info[3].sync_with_global_time) {
		var_info[3].time = ref_time + var_info[3].ref_time_offset;
	}
	SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting ScatterNewPlotCanvas::TitleOrTimeChange");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void ScatterNewPlotCanvas::VarInfoAttributeChange()
{
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	int x_tms = (var_info[0].time_max-var_info[0].time_min) + 1;
	int y_tms = (var_info[1].time_max-var_info[1].time_min) + 1;
	x_data.resize(boost::extents[x_tms][num_obs]);
	for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
		int tt = t-var_info[0].time_min;
		for (int i=0; i<num_obs; i++) x_data[tt][i] = data[0][t][i];
	}
	y_data.resize(boost::extents[y_tms][num_obs]);
	for (int t=var_info[1].time_min; t<=var_info[1].time_max; t++) {
		int tt = t-var_info[1].time_min;
		for (int i=0; i<num_obs; i++) y_data[tt][i] = data[1][t][i];
	}
	if (is_bubble_plot) {
		int z_tms = (var_info[2].time_max-var_info[2].time_min) + 1;
		z_data.resize(boost::extents[z_tms][num_obs]);
		for (int t=var_info[2].time_min; t<=var_info[2].time_max; t++) {
			int tt = t-var_info[2].time_min;
			for (int i=0; i<num_obs; i++) z_data[tt][i] = data[2][t][i];
		}
	}
	//GeoDa::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_cats and ref_var_index */
void ScatterNewPlotCanvas::CreateAndUpdateCategories()
{
	cats_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_valid[t] = true;
	cats_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_error_message[t] = wxEmptyString;
	
	if (!is_bubble_plot || theme_type == ThemeUtilities::no_theme) {
		CreateCategoriesAllCanvasTms(1, num_time_vals); // 1 = #cats
		for (int t=0; t<num_time_vals; t++) {
			SetCategoryColor(t, 0, GeoDaConst::map_default_fill_colour);
			SetCategoryLabel(t, 0, "");
			for (int i=0; i<num_obs; i++) AppendIdToCategory(t, 0, i);
		}
		if (ref_var_index != -1) {
			SetCurrentCanvasTmStep(var_info[ref_var_index].time
								   - var_info[ref_var_index].time_min);
		}
		if (is_bubble_plot) {
			CreateZValArrays(num_time_vals, num_obs);
			for (int t=0; t<num_time_vals; t++) {
				int tt = var_info[2].time_min;
				if (var_info[2].sync_with_global_time) {
					tt += t;
				}
				for (int cat=0, ce=GetNumCategories(t) ; cat<ce; cat++) {
					for (int i=0, ie=categories[t].cat_vec[cat].ids.size();
						 i<ie; i++) {
						int obs_id = categories[t].cat_vec[cat].ids[i];
						int ord = obs_id_to_z_val_order[tt][obs_id];
						z_val_order[t][ord][0] = obs_id;  // obs id
						z_val_order[t][ord][1] = cat;  // category id
					}
				}
			}
		}
		return;
	}
	
	// Everything below assumes that theme_type != no_theme
	std::vector<GeoDa::dbl_int_pair_vec_type> cat_var_sorted(num_time_vals);	
	for (int t=0; t<num_time_vals; t++) {
		// Note: need to be careful here: what about when a time variant
		// variable is not synced with time?  time_min should reflect this,
		// so possibly ok.
		cat_var_sorted[t].resize(num_obs);
		for (int i=0; i<num_obs; i++) {
			int tm = var_info[3].is_time_variant ? t : 0;
			cat_var_sorted[t][i].first = data[3][tm+var_info[3].time_min][i];
			cat_var_sorted[t][i].second = i;
		}
	}	
	
	// Sort each vector in ascending order
	for (int t=0; t<num_time_vals; t++) {
		if (cats_valid[t]) { // only sort data with valid data
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  GeoDa::dbl_int_pair_cmp_less);
		}
	}
	
	int num_cats = 1;
	if (theme_type == ThemeUtilities::quantile ||
		theme_type == ThemeUtilities::natural_breaks ||
		theme_type == ThemeUtilities::equal_intervals) {
		// Need to ask user for number of categories
		
		wxString title;
		if (theme_type == ThemeUtilities::quantile) {
			title = "Quantile";
		} else if (theme_type == ThemeUtilities::natural_breaks) {
			title = "Natural Breaks";
		} else if (theme_type == ThemeUtilities::equal_intervals) {
			title = "Equal Intervals";
		}
		
		MapQuantileDlg dlg(this, 1, ThemeUtilities::max_num_classes, 4, title);
		dlg.SetTitle(title);
		if (dlg.ShowModal() != wxID_OK) {
			num_cats = 4;
		} else {
			num_cats = dlg.classes;
		}
	}
	
	ThemeUtilities::SetThemeCategories(num_time_vals, num_cats, theme_type,
									   var_info[3], cat_var_sorted,
									   this, cats_valid,
									   cats_error_message);
	
	CreateZValArrays(num_time_vals, num_obs);
	for (int t=0; t<num_time_vals; t++) {
		int tt = var_info[2].time_min;
		if (var_info[2].sync_with_global_time) {
			tt += t;
		}
		for (int cat=0, ce=GetNumCategories(t) ; cat<ce; cat++) {
			for (int i=0, ie=categories[t].cat_vec[cat].ids.size();
				 i<ie; i++) {
				int obs_id = categories[t].cat_vec[cat].ids[i];
				int ord = obs_id_to_z_val_order[tt][obs_id];
				z_val_order[t][ord][0] = obs_id;  // obs id
				z_val_order[t][ord][1] = cat;  // category id
			}
		}
	}
	
	if (ref_var_index != -1) {
		SetCurrentCanvasTmStep(var_info[ref_var_index].time
							   - var_info[ref_var_index].time_min);
	}
}

void ScatterNewPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In ScatterNewPlotCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In ScatterNewPlotCanvas::FixedScaleVariableToggle");
	var_info[var_index].fixed_scale =
		!var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ViewStandardizedData()
{
	LOG_MSG("In ScatterNewPlotCanvas::ViewStandardizedData");
	standardized = true;
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ViewOriginalData()
{
	LOG_MSG("In ScatterNewPlotCanvas::ViewOriginalData");
	standardized = false;
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ViewRegressionSelected(bool display)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::ViewRegressionSelected");
	bool changed = false;
	if (!display) {
		reg_line_selected->setPen(*wxTRANSPARENT_PEN);
		if ((show_reg_selected && !show_reg_excluded) && display_stats) {
			// there is no longer anything showing, but there
			// was previously something showing
			show_reg_selected = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		} else {
			show_reg_selected = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	} else {
		if ((!show_reg_selected && !show_reg_excluded) && display_stats) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_selected = true;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			PopulateCanvas();
		} else {
			show_reg_selected = true;
			CalcStatsFromSelected();
			UpdateRegSelectedLine();
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	}
	Refresh();
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
					   wxPen(highlight_color));
	ApplyLastResizeToShp(reg_line_selected);
	layer2_valid = false;
	LOG_MSG("Exiting ScatterNewPlotCanvas::UpdateRegSelectedLine");	
}

void ScatterNewPlotCanvas::ViewRegressionSelectedExcluded(bool display)
{
	LOG_MSG("Entering ScatterNewPlotCanvas::ViewRegressionSelectedExcluded");
	bool changed = false;
	if (!display) {
		reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
		if ((!show_reg_selected && show_reg_excluded) && display_stats) {
			// there is no longer anything showing, but there
			// was previously something showing
			show_reg_excluded = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		} else {
			show_reg_excluded = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	} else {
		if ((!show_reg_selected && !show_reg_excluded) && display_stats) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_excluded = true;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			PopulateCanvas();
		} else {
			CalcStatsFromSelected();
			show_reg_excluded = true;
			UpdateRegExcludedLine();
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	}
	Refresh();
	LOG_MSG("Exiting ScatterNewPlotCanvas::ViewRegressionSelectedExcluded");	
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
					   wxPen(selectable_fill_color));
	ApplyLastResizeToShp(reg_line_excluded);
	layer2_valid = false;
	LOG_MSG("Exiting ScatterNewPlotCanvas::UpdateRegExcludedLine");
}

void ScatterNewPlotCanvas::DisplayStatistics(bool display_stats_s)
{
	LOG_MSG("In ScatterNewPlotCanvas::DisplayStatistics");
	display_stats = display_stats_s;
	UpdateDisplayStats();
	UpdateDisplayLinesAndMargins();
	ResizeSelectableShps();
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
	statsXselected.max = -std::numeric_limits<double>::max();
	statsYselected.max = -std::numeric_limits<double>::max();
	statsXexcluded.max = -std::numeric_limits<double>::max();
	statsYexcluded.max = -std::numeric_limits<double>::max();
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
		sse_sel = SS_err;
	} else {
		for (int i=0, iend=Y.size(); i<iend; i++) {
			if (!hl[i]) {
				err = Y[i] - (r.alpha + r.beta * X[i]);
				SS_err += err * err;
			}
		}
		sse_unsel = SS_err;
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

void ScatterNewPlotCanvas::ComputeChowTest()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::ComputeChowTest");
	wxString s;
	s << "Chow test for sel/unsel regression subsets: ";
	int tot_sel = highlight_state->GetTotalHighlighted();
	int hl_size = highlight_state->GetHighlightSize();
	double N = X.size();
	double K = 1;
	double sse_u = sse_sel + sse_unsel;
	if (K+1 <= 0 || N-2*(K+1) <= 0 || sse_u == 0) {
		chow_valid = false;
		s << "can't compute";
		chow_test_text->setText(s);
		return;
	}
	if (tot_sel >= 2 && tot_sel <= hl_size-2) {
		chow_valid = true;
		using namespace boost::math;
		// Compute Chow test ratio (F value) based on
		// sse_c, sse_sel and sse_unsel
		// note number of restrictions is K+1 since intercepts are constrained
		// to be equal.
		chow_ratio = ((sse_c - sse_u) * (N-2*(K+1))) / (sse_u*(K+1));
		LOG(chow_ratio);
		// constructs and f-distribution with numerator degrees of
		// freedom K+1 and denominator degrees of freedom N-2*(K+1);
		fisher_f_distribution<> f_dist(K+1, N-2*(K+1));
		LOG_MSG("numerator df:");
		LOG(f_dist.degrees_of_freedom1());
		LOG_MSG("denominator df:");
		LOG(f_dist.degrees_of_freedom2());
		chow_pval = 1-cdf(f_dist, chow_ratio);
		chow_valid = true;
		LOG(chow_pval);
	} else {
		chow_valid = false;
	}
	if (chow_valid) {
		s << "distrib=F(" << K+1 << "," << N-2*(K+1) << ")";
		s << ", ratio=" << GenUtils::DblToStr(chow_ratio, 4);
		s << ", p-val=" << GenUtils::DblToStr(chow_pval, 4);
	} else {
		s << "need two valid regressions";
	}
	chow_test_text->setText(s);
	
	LOG_MSG("Exiting ScatterNewPlotCanvas::ComputeChowTest");
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
	//LOG_MSG("Entering ScatterNewPlotCanvas::CalcRegressionLine");
	reg_line.setBrush(*wxTRANSPARENT_BRUSH); // ensure brush is transparent
	slope = 0; //default
	infinite_slope = false; // default
	regression_defined = true; // default

	if (!reg.valid) {
		regression_defined = false;
		reg_line.setPen(*wxTRANSPARENT_PEN);
		return;
	}
	
	reg_a = wxRealPoint(0, 0);
	reg_b = wxRealPoint(0, 0);
	
	//LOG(reg.beta);
	
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
		reg_line.setPen(*wxTRANSPARENT_PEN);
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
	
	//LOG(slope);
	//LOG(infinite_slope);
	//LOG(regression_defined);
	//LOG(cc_degs_of_rot);
	//LOG(reg_a.x);
	//LOG(reg_a.y);
	//LOG(reg_b.x);
	//LOG(reg_b.y);
	
	reg_line.setPen(pen);
	//LOG_MSG("Exiting ScatterNewPlotCanvas::CalcRegressionLine");
}

/** This method builds up the display optional stats string from scratch every
 time. It assumes the calling function will do the screen Refresh. */
void ScatterNewPlotCanvas::UpdateDisplayStats()
{
	if (display_stats) {
		// fill out the regression stats table
		int rows = 2;
		if (show_reg_selected) rows++;
		if (show_reg_excluded) rows++;
		int cols = 10;
		std::vector<wxString> vals(rows*cols);
		std::vector<MyTable::CellAttrib> attributes(rows*cols);
		int i=0; int j=0;
		for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
			attributes[k].color = *wxBLACK;
		}
		int tot_obs = highlight_state->GetHighlightSize();
		int tot_sel_obs = highlight_state->GetTotalHighlighted();
		int tot_unsel_obs = tot_obs - tot_sel_obs;
		vals[i*cols+j++] = "#obs";
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
			attributes[k].color = selectable_outline_color;
		}
		vals[i*cols+j++] << tot_obs;
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.r_squared);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.std_err_of_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.t_score_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.p_value_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.std_err_of_beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.t_score_beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.p_value_beta);
		if (show_reg_selected) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color = highlight_color;
			}
			vals[i*cols+j++] << tot_sel_obs;
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.r_squared);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.std_err_of_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.t_score_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.p_value_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.std_err_of_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.t_score_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.p_value_beta);
		}
		if (show_reg_excluded) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color = selectable_fill_color;
			}
			vals[i*cols+j++] << tot_unsel_obs;
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.r_squared);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.std_err_of_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.t_score_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.p_value_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.std_err_of_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.t_score_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.p_value_beta);		
		}
		int x_nudge = (virtual_screen_marg_left-virtual_screen_marg_right)/2;
		
		stats_table->operator=(MyTable(vals, attributes, rows, cols,
									   *GeoDaConst::small_font,
									   wxRealPoint(50, 0),
									   MyText::h_center, MyText::top,
									   MyText::h_center, MyText::v_center,
									   3, 8, -x_nudge, 45)); //62));
		stats_table->setPen(*wxBLACK_PEN);
		stats_table->hidden = false;
		
		if (show_reg_selected && show_reg_excluded) {
			int table_w=0, table_h=0;
			wxClientDC dc(this);
			stats_table->GetSize(dc, table_w, table_h);
			ComputeChowTest();
			wxString s = chow_test_text->getText();
			chow_test_text->operator=(MyText(s, *GeoDaConst::small_font,
											 wxRealPoint(50,0), 0,
											 MyText::h_center, MyText::v_center,
											 -x_nudge,
											 table_h+62)); //117));
			chow_test_text->setPen(*wxBLACK_PEN);
			chow_test_text->hidden = false;
		} else {
			chow_test_text->setText("");
			chow_test_text->hidden = true;
		}
		
		ApplyLastResizeToShp(chow_test_text);
		ApplyLastResizeToShp(stats_table);
	} else {
		chow_test_text->setText("");
		chow_test_text->hidden = true;
		
		stats_table->hidden = true;
	}
	layer2_valid = false;
}

void ScatterNewPlotCanvas::UpdateAxesThroughOrigin()
{
	x_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	y_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	if (show_origin_axes &&
		axis_scale_y.scale_min < 0 && 0 < axis_scale_y.scale_max) {
		double y_inter = 100.0 * ((-axis_scale_y.scale_min) /
			(axis_scale_y.scale_max-axis_scale_y.scale_min));
		x_axis_through_origin->operator=(MyPolyLine(0,y_inter,100,y_inter));
		x_axis_through_origin->setPen(*GeoDaConst::scatterplot_origin_axes_pen);
		ApplyLastResizeToShp(x_axis_through_origin);
	}
	if (show_origin_axes &&
		axis_scale_x.scale_min < 0 && 0 < axis_scale_x.scale_max) {
		double x_inter = 100.0 * ((-axis_scale_x.scale_min) /
			(axis_scale_x.scale_max-axis_scale_x.scale_min));
		y_axis_through_origin->operator=(MyPolyLine(x_inter,0,x_inter,100));
		y_axis_through_origin->setPen(*GeoDaConst::scatterplot_origin_axes_pen);
		ApplyLastResizeToShp(y_axis_through_origin);
	}
	layer0_valid = false;
}

bool ScatterNewPlotCanvas::UpdateDisplayLinesAndMargins()
{
	LOG_MSG("Entering ScatterNewPlotCanvas::UpdateDisplayLinesAndMargins");
	bool changed = false;
	int lines = 0;
	int table_w=0, table_h=0;
	if (display_stats && stats_table) {
		wxClientDC dc(this);	
		stats_table->GetSize(dc, table_w, table_h);
		LOG(table_w);
		LOG(table_h);
	}
	virtual_screen_marg_bottom = 50;
	if (!display_stats) {
		lines = 0;
		//virtual_screen_marg_bottom = 50;
	} else if (!show_reg_selected && !show_reg_excluded) {
		lines = 1;
		virtual_screen_marg_bottom += 10;
		//virtual_screen_marg_bottom = 90;
	} else if (show_reg_selected != show_reg_excluded) {
		lines = 2;
		virtual_screen_marg_bottom += 10;
		//virtual_screen_marg_bottom = 90+13;
	} else {
		lines = 3;
		virtual_screen_marg_bottom += 30;  // leave room for Chow Test
		//virtual_screen_marg_bottom = 90+2*13+20;
	}
	virtual_screen_marg_bottom += table_h;
	if (table_display_lines != lines) {
		layer0_valid = false;
		layer1_valid = false;
		layer2_valid = false;
		changed = true;
	}
	
	table_display_lines = lines;
	LOG(table_display_lines);
	LOG(changed ? "true" : "false");
	LOG_MSG("Exiting ScatterNewPlotCanvas::UpdateDisplayLinesAndMargins");
	return changed;
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

void ScatterNewPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "obs " << hover_obs[0]+1 << " = (";
			s << X[hover_obs[0]] << ", " << Y[hover_obs[0]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[0]];
				s << ", " << data[3][var_info[3].time][hover_obs[0]];
			}
			s << ")";
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = (";
			s << X[hover_obs[1]] << ", " << Y[hover_obs[1]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[1]];
				s << ", " << data[3][var_info[3].time][hover_obs[1]];
			}
			s << ")";
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = (";
			s << X[hover_obs[2]] << ", " << Y[hover_obs[2]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[2]];
				s << ", " << data[3][var_info[3].time][hover_obs[2]];
			}
			s << ")";
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	} else if (mousemode == select &&
			   (selectstate == dragging || selectstate == brushing)) {
		s << "#selected=" << highlight_state->GetTotalHighlighted();
		if (brushtype == rectangle) {
			wxRealPoint pt1 = MousePntToObsPnt(sel1);
			wxRealPoint pt2 = MousePntToObsPnt(sel2);
			wxString xmin = GenUtils::DblToStr(GenUtils::min<double>(pt1.x,
																	 pt2.x));
			wxString xmax = GenUtils::DblToStr(GenUtils::max<double>(pt1.x,
																	 pt2.x));
			wxString ymin = GenUtils::DblToStr(GenUtils::min<double>(pt1.y,
																	 pt2.y));
			wxString ymax = GenUtils::DblToStr(GenUtils::max<double>(pt1.y,
																	 pt2.y));
			s << ", select rect: ";
			s << GetNameWithTime(0) << "=[" << xmin << "," << xmax << "] and ";
			s << GetNameWithTime(1) << "=[" << ymin << "," << ymax << "]";
		}
		
	}
	sb->SetStatusText(s);
}

ScatterNewPlotLegend::ScatterNewPlotLegend(wxWindow *parent,
										   TemplateCanvas* t_canvas,
										   const wxPoint& pos,
										   const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

ScatterNewPlotLegend::~ScatterNewPlotLegend()
{
    LOG_MSG("In ScatterNewPlotLegend::~ScatterNewPlotLegend");
}

IMPLEMENT_CLASS(ScatterNewPlotFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(ScatterNewPlotFrame, TemplateFrame)
	EVT_ACTIVATE(ScatterNewPlotFrame::OnActivate)
END_EVENT_TABLE()

ScatterNewPlotFrame::ScatterNewPlotFrame(wxFrame *parent, Project* project,
										 const wxPoint& pos, const wxSize& size,
										 const long style)
: TemplateFrame(parent, project, "", pos, size, style),
is_bubble_plot(false)
{
	LOG_MSG("Entering ScatterNewPlotFrame::ScatterNewPlotFrame");
	LOG_MSG("Exiting ScatterNewPlotFrame::ScatterNewPlotFrame");
}


ScatterNewPlotFrame::ScatterNewPlotFrame(wxFrame *parent, Project* project,
									const std::vector<GeoDaVarInfo>& var_info,
										 const std::vector<int>& col_ids,
										 bool is_bubble_plot_s,
										 const wxString& title,
										 const wxPoint& pos,
										 const wxSize& size,
										 const long style)
: TemplateFrame(parent, project, title, pos, size, style),
is_bubble_plot(is_bubble_plot_s)
{
	LOG_MSG("Entering ScatterNewPlotFrame::ScatterNewPlotFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	
	wxSplitterWindow* splitter_win = 0;
	if (is_bubble_plot) {
		splitter_win = new wxSplitterWindow(this);
		splitter_win->SetMinimumPaneSize(10);
	}
		
	if (is_bubble_plot) {
		template_canvas = new ScatterNewPlotCanvas(splitter_win, this, project,
												   var_info, col_ids,
												   is_bubble_plot,
												   false, wxDefaultPosition,
												   wxSize(width,height));
	} else {
		template_canvas = new ScatterNewPlotCanvas(this, this, project,
												   var_info, col_ids,
												   is_bubble_plot,
												   false, wxDefaultPosition,
												   wxSize(width,height));
	}
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
	if (is_bubble_plot) {
		template_legend = new ScatterNewPlotLegend(splitter_win,
												   template_canvas,
												   wxPoint(0,0), wxSize(0,0));
		
		splitter_win->SplitVertically(template_legend, template_canvas,
								GeoDaConst::bubble_chart_default_legend_width);
	}
	
	Show(true);
	LOG_MSG("Exiting ScatterNewPlotFrame::ScatterNewPlotFrame");
}

ScatterNewPlotFrame::~ScatterNewPlotFrame()
{
	LOG_MSG("In ScatterNewPlotFrame::~ScatterNewPlotFrame");
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void ScatterNewPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ScatterNewPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("ScatterNewPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void ScatterNewPlotFrame::MapMenus()
{
	LOG_MSG("In ScatterNewPlotFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	if (is_bubble_plot) {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_BUBBLE_CHART_VIEW_MENU_OPTIONS");
	} else {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	}
	((ScatterNewPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((ScatterNewPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("ScatterNewPlotFrame::UpdateOptionMenuItems: Options "
				"menu not found");
	} else {
		((ScatterNewPlotCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void ScatterNewPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void ScatterNewPlotFrame::update(FramesManager* o)
{
	LOG_MSG("In ScatterNewPlotFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void ScatterNewPlotFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
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

void ScatterNewPlotFrame::OnThemeless(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::no_theme);
}

void ScatterNewPlotFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::hinge_15);
}

void ScatterNewPlotFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::hinge_30);
}

void ScatterNewPlotFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::quantile);
}

void ScatterNewPlotFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::percentile);
}

void ScatterNewPlotFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::stddev);
}

void ScatterNewPlotFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::unique_values);
}

void ScatterNewPlotFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::natural_breaks);
}

void ScatterNewPlotFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::equal_intervals);
}

void ScatterNewPlotFrame::ChangeThemeType(ThemeUtilities::ThemeType new_theme)
{
	((ScatterNewPlotCanvas*) template_canvas)->ChangeThemeType(new_theme);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Refresh();
}

void ScatterNewPlotFrame::OnSaveCategories(wxCommandEvent& event)
{
	((ScatterNewPlotCanvas*) template_canvas)->OnSaveCategories();
}


