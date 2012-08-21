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
#include <wx/dcmemory.h>
#include <wx/graphics.h>
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
#include "PCPNewView.h"

IMPLEMENT_CLASS(PCPNewCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(PCPNewCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(PCPNewCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

PCPNewCanvas::PCPNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
								   Project* project_s,
								   const std::vector<GeoDaVarInfo>& v_info,
								   const std::vector<int>& col_ids,
								   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), var_info(v_info), num_obs(project_s->GetNumRecords()),
num_time_vals(1), num_vars(v_info.size()),
data(v_info.size()),
highlight_state(project_s->highlight_state),
display_stats(false), show_axes(true), standardized(false),
pcp_selectstate(pcp_start), show_pcp_control(false),
overall_abs_max_std_exists(false), theme_var(0),
num_cats(6), remember_num_cats(true), theme_type(ThemeUtilities::stddev)
{
	using namespace Shapefile;
	LOG_MSG("Entering PCPNewCanvas::PCPNewCanvas");
	template_frame = t_frame;	
	DbfGridTableBase* grid_base = project->GetGridBase();
		
	LOG(var_info.size());
	
	//std::vector< std::vector<GeoDa::dbl_int_pair_vec_type> > data_sorted;
	//std::vector< std::vector<HingeStats> > hinge_stats;
	//data_sorted.resize(v_info.size());
	//hinge_stats.resize(v_info.size());
	data_stats.resize(v_info.size());
	
	std::vector<double> temp_vec(num_obs);
	for (int v=0; v<num_vars; v++) {
		grid_base->GetColData(col_ids[v], data[v]);
		int data_times = data[v].shape()[0];
		data_stats[v].resize(data_times);
		//hinge_stats[v].resize(data_times);
		//data_sorted[v].resize(data_times);
		for (int t=0; t<data_times; t++) {
			//data_sorted[v][t].resize(num_obs);
			for (int i=0; i<num_obs; i++) {
				temp_vec[i] = data[v][t][i];
				//data_sorted[v][t][i].first = data[v][t][i];
				//data_sorted[v][t][i].second = i;
			}
			//std::sort(data_sorted[v][t].begin(),
			//		  data_sorted[v][t].end(),
			//		  GeoDa::dbl_int_pair_cmp_less);
			//hinge_stats[v][t].CalculateHingeStats(data_sorted[v][t]);
			data_stats[v][t].CalculateFromSample(temp_vec);
			double min = data_stats[v][t].min;
			double max = data_stats[v][t].max;
			if (min != max) {
				double mean = data_stats[v][t].mean;
				double sd = data_stats[v][t].sd_with_bessel;
				double s_min = (min - mean)/sd;
				double s_max = (max - mean)/sd;
				double abs_max =
					GenUtils::max<double>(GenUtils::abs<double>(s_min),
										  GenUtils::abs<double>(s_max));
				if (!overall_abs_max_std_exists) {
					overall_abs_max_std_exists = true;
					overall_abs_max_std = abs_max;
				} else if (abs_max > overall_abs_max_std) {
					overall_abs_max_std = abs_max;
				}
			}
		}
	}
	
	control_labels.resize(num_vars);
	control_circs.resize(num_vars);
	control_lines.resize(num_vars);
	var_order.resize(num_vars);
	for (int v=0; v<num_vars; v++) var_order[v] = v;
	
	/*
	for (int v=0; v<num_vars; v++) {
		int data_times = data[v].shape()[0];
		for (int t=0; t<data_times; t++) {
			for (int i=0; i<num_obs; i++) {
				LOG(data[v][t][i]);
			}
		}
	}
	 */
	
	selectable_fill_color = GeoDaConst::pcp_line_color;
	//highlight_color = GeoDaConst::highlight_color;
	highlight_color = wxColour(68, 244, 136); // light mint green
	
	fixed_aspect_ratio_mode = false;
	use_category_brushes = true;
	selectable_shps_type = polylines;
	
	ChangeThemeType(ThemeUtilities::stddev);
	
	/*
	VarInfoAttributeChange();
	CreateCategoriesAllCanvasTms(1, num_time_vals); // 1 = #cats
	for (int t=0; t<num_time_vals; t++) {
		SetCategoryColor(t, 0, selectable_fill_color);
		for (int i=0; i<num_obs; i++) AppendIdToCategory(t, 0, i);
	}
	if (ref_var_index != -1) {
		SetCurrentCanvasTmStep(var_info[ref_var_index].time
							   - var_info[ref_var_index].time_min);
	}
	PopulateCanvas();
	*/
	 
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting PCPNewCanvas::PCPNewCanvas");
}

PCPNewCanvas::~PCPNewCanvas()
{
	LOG_MSG("Entering PCPNewCanvas::~PCPNewCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting PCPNewCanvas::~PCPNewCanvas");
}

void PCPNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering PCPNewCanvas::DisplayRightClickMenu");
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_PCP_NEW_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting PCPNewCanvas::DisplayRightClickMenu");
}

void PCPNewCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
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

	/*
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	if (var_info[0].is_time_variant) {
		wxString s;
		s << "Fixed scale over time";
		wxMenuItem* mi =
		menu2->AppendCheckItem(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1, s, s);
		mi->Check(var_info[0].fixed_scale);
	}
	 */
		
	//menu->Prepend(wxID_ANY, "Scale Options", menu2, "Scale Options");
	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

void PCPNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !standardized);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  standardized);
	
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
	
	//if (var_info[0].is_time_variant) {
	//	GeneralWxUtils::CheckMenuItem(menu,
	//								  GeoDaConst::ID_TIME_SYNC_VAR1,
	//								  var_info[0].sync_with_global_time);
	//	GeneralWxUtils::CheckMenuItem(menu,
	//								  GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
	//								  var_info[0].fixed_scale);
	//}
}

/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void PCPNewCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering PCPNewCanvas::update");

	// we want to force a full redraw of all selected objects
	layer1_valid = false;
	layer2_valid = false;
 
	Refresh();
	
	LOG_MSG("Entering PCPNewCanvas::update");	
}

wxString PCPNewCanvas::GetCanvasTitle()
{
	wxString s("Parallel Coordinate Plot: ");
	s << GetNameWithTime(var_order[0]) << ", ";
	if (num_vars > 2) s << "..., ";
	s << GetNameWithTime(var_order[num_vars-1]);
	return s;
}

wxString PCPNewCanvas::GetCategoriesTitle()
{
	if (theme_type == ThemeUtilities::no_theme) {
		return "Themeless";
	}
	wxString s;
	s << ThemeUtilities::ThemeTypeToString(theme_type);
	s << ": " << GetNameWithTime(theme_var);
	return s;
}


wxString PCPNewCanvas::GetNameWithTime(int var)
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
void PCPNewCanvas::ChangeThemeType(ThemeUtilities::ThemeType new_map_theme)
{
	theme_type = new_map_theme;
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void PCPNewCanvas::OnSaveCategories()
{
	wxString t_name = ThemeUtilities::ThemeTypeToString(theme_type);
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
	SaveCategories(title, label, "CATEGORIES");
}

void PCPNewCanvas::PopulateCanvas()
{
	LOG_MSG("Entering PCPNewCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	double x_min = 0;
	double x_max = 100;
	double y_min = 0;
	double y_max = 100;
	
	shps_orig_xmin = x_min;
	shps_orig_ymin = y_min;
	shps_orig_xmax = x_max;
	shps_orig_ymax = y_max;
	virtual_screen_marg_top = 25;
	if (!display_stats && !standardized) {
		virtual_screen_marg_bottom = 25;
	} else if (!display_stats && standardized) {
		virtual_screen_marg_bottom = 33;
	} else {
		virtual_screen_marg_bottom =  25+25;
	}
	virtual_screen_marg_left = 135;
	virtual_screen_marg_right = 25;
	
	// For each variable, we know it's current time and it's
	// min/max values over it's available times.  This is all that
	// we need to know in order to create the PCP.
	
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
	
	selectable_shps.resize(num_obs);
	
	MyShape* s = 0;
	wxRealPoint* pts = new wxRealPoint[num_vars];
	double std_fact = 1;
	if (overall_abs_max_std_exists) std_fact = 100.0/(2.0*overall_abs_max_std);
	double nvf = 100.0/((double) (num_vars-1));
	for (int i=0; i<num_obs; i++) {
		for (int v=0; v<num_vars; v++) {
			int vv = var_order[v];
			int t = var_info[vv].time;
			double min = data_stats[vv][t].min;
			double max = data_stats[vv][t].max;
			if (min == max) {
				pts[v].x = (x_max-x_min)/2.0;
			} else if (!standardized) {
				double rng = (var_info[vv].fixed_scale ? 
							  (var_info[vv].max_over_time - 
							   var_info[vv].min_over_time) : max-min);
				pts[v].x = 100.0*((data[vv][t][i]-min) / rng);
			} else  {
				double mean = data_stats[vv][t].mean;
				double sd = data_stats[vv][t].sd_with_bessel;
				pts[v].x = ((data[vv][t][i]-mean)/sd)+overall_abs_max_std;
				pts[v].x *= std_fact;
			}
			pts[v].y = 100.0-(nvf*((double) v));
		}
		selectable_shps[i] = new MyPolyLine(num_vars, pts);
	}
	wxPen control_line_pen(GeoDaConst::pcp_horiz_line_color);
	control_line_pen.SetWidth(2);
	for (int v=0; v<num_vars; v++) {
		int y_del = display_stats ? -8 : 0;
		int vv = var_order[v];
		int t = var_info[vv].time;
		double y_pos = 100.0-(nvf*((double) v));
		s = new MyPolyLine(0, y_pos, 100, y_pos);
		s->setPen(control_line_pen);
		background_shps.push_back(s);
		control_lines[v] = (MyPolyLine*) s;
		s = new MyRay(wxRealPoint(0, y_pos), 180, 10);
		s->setPen(control_line_pen);
		background_shps.push_back(s);
		s = new MyCircle(wxRealPoint(0, y_pos), 3.0);
		s->setNudge(-10, 0);
		s->setPen(control_line_pen);
		s->setBrush(*wxWHITE_BRUSH);
		background_shps.push_back(s);
		control_circs[v] = (MyCircle*) s;
		s = new MyText(GetNameWithTime(vv), *GeoDaConst::small_font,
					   wxRealPoint(0, y_pos), 0, MyText::right,
					   MyText::v_center, -25, 0+y_del);
		background_shps.push_back(s);
		control_labels[v] = (MyText*) s;
		wxString m;
		double t_min = data_stats[vv][t].min;
		double t_max = data_stats[vv][t].max;
		double t_mean = data_stats[vv][t].mean;
		double t_sd = (t_min == t_max ? 0 : data_stats[vv][t].sd_with_bessel);
		if (standardized) {
			if (t_min == t_max) {
				t_min = 0;
				t_max = 0;
				t_sd = 0;
			} else {
				double mean = data_stats[vv][t].mean;
				double sd = data_stats[vv][t].sd_with_bessel;
				t_min = (t_min-mean)/sd;
				t_max = (t_max-mean)/sd;
				t_sd = 1;
			}
			t_mean = 0;
		}
		
		if (display_stats) {
			m << "[" << GenUtils::DblToStr(t_min, 4);
			m << ", " << GenUtils::DblToStr(t_max, 4) << "]";
			s = new MyText(m, *GeoDaConst::small_font, wxRealPoint(0, y_pos), 0,
						   MyText::right, MyText::v_center, -25, 15+y_del);
			background_shps.push_back(s);
			int cols = 2;
			int rows = 2;
			std::vector<wxString> vals(rows*cols);
			vals[0] << "mean";
			vals[1] << GenUtils::DblToStr(t_mean, 4);
			vals[2] << "s.d.";
			vals[3] << GenUtils::DblToStr(t_sd, 4);
			std::vector<MyTable::CellAttrib> attribs(0); // undefined
			s = new MyTable(vals, attribs, rows, cols, *GeoDaConst::small_font,
							wxRealPoint(0, y_pos), MyText::right,
							MyText::top, MyText::right, MyText::v_center,
							3, 7, -25, 25+y_del);
			background_shps.push_back(s);
		}
	}
	if (standardized) {
		// add dotted lines and labels for sd and mean
		// add dotted line for mean in center
		s = new MyPolyLine(50, 0, 50, 100);
		s->setPen(*GeoDaConst::scatterplot_origin_axes_pen);
		background_shps.push_back(s);
		s = new MyText(wxString::Format("%d", 0),
					   *GeoDaConst::small_font, wxRealPoint(50, 0), 0,
					   MyText::h_center, MyText::v_center, 0, 12);
		background_shps.push_back(s);
		int sd_abs = overall_abs_max_std;
		for (int i=1; i<=sd_abs && overall_abs_max_std_exists; i++) {
			double sd_p = (double) i;
			sd_p += overall_abs_max_std;
			sd_p *= std_fact;
			double sd_m = (double) -i;
			sd_m += overall_abs_max_std;
			sd_m *= std_fact;
			s = new MyPolyLine(sd_p, 0, sd_p, 100);
			s->setPen(*GeoDaConst::scatterplot_origin_axes_pen);
			background_shps.push_back(s);
			s = new MyText(wxString::Format("%d", i),
						   *GeoDaConst::small_font, wxRealPoint(sd_p, 0), 0,
						   MyText::h_center, MyText::v_center, 0, 12);
			background_shps.push_back(s);
			s = new MyPolyLine(sd_m, 0, sd_m, 100);
			s->setPen(*GeoDaConst::scatterplot_origin_axes_pen);
			background_shps.push_back(s);
			s = new MyText(wxString::Format("%d", -i),
						   *GeoDaConst::small_font, wxRealPoint(sd_m, 0), 0,
						   MyText::h_center, MyText::v_center, 0, 12);
			background_shps.push_back(s);
		}
	}
	
	delete [] pts;
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting PCPNewCanvas::PopulateCanvas");
}

void PCPNewCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering PCPNewCanvas::TitleOrTimeChange");
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
	for (int v=0; v<num_vars; v++) {
		if (var_info[v].sync_with_global_time) {
			var_info[v].time = ref_time + var_info[v].ref_time_offset;
		}
	}
	SetCurrentCanvasTmStep(ref_time - ref_time_min);
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting PCPNewCanvas::TitleOrTimeChange");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void PCPNewCanvas::VarInfoAttributeChange()
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
	
	//GeoDa::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_cats and ref_var_index */
void PCPNewCanvas::CreateAndUpdateCategories()
{
	cats_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_valid[t] = true;
	cats_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_error_message[t] = wxEmptyString;
	
	if (theme_type == ThemeUtilities::no_theme) {
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
			int tm = var_info[theme_var].is_time_variant ? t : 0;
			cat_var_sorted[t][i].first = 
				data[theme_var][tm+var_info[theme_var].time_min][i];
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
	
	if (!remember_num_cats &&
		(theme_type == ThemeUtilities::quantile ||
		 theme_type == ThemeUtilities::natural_breaks ||
		 theme_type == ThemeUtilities::equal_intervals)) {
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
			remember_num_cats = true;
		}
	}
	
	ThemeUtilities::SetThemeCategories(num_time_vals, num_cats, theme_type,
									   var_info[theme_var], cat_var_sorted,
									   this, cats_valid,
									   cats_error_message);
	
	if (ref_var_index != -1) {
		SetCurrentCanvasTmStep(var_info[ref_var_index].time
							   - var_info[ref_var_index].time_min);
	}
}

void PCPNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In PCPNewCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPNewCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In PCPNewCanvas::FixedScaleVariableToggle");
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPNewCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPNewCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPNewCanvas::StandardizeData(bool standardize)
{
	if (standardize == standardized) return;
	standardized = standardize;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

// wxMouseEvent notes:
// LeftDown(): true when the left button is first pressed down
// LeftIsDown(): true while the left button is down. During a mouse dragging
//  operation, this will continue to return true, while LeftDown() is false.
// RightDown/RightIsDown: similar to Left.
// Moving(): returns true when mouse is moved, but no buttons are pressed.
// Dragging(): returns true when mouse is moved and at least one mouse button is
//   pressed.
// CmdDown(): Checks if MetaDown under Mac and ControlDown on other platforms.
// ButtonDClick(int but = wxMOUSE_BTN_ANY): checks for double click of any
//   button. Can also specify wxMOUSE_BTN_LEFT / RIGHT / MIDDLE.  Or
//   LeftDCLick(), etc.
// LeftUp(): returns true at the moment the button changed to up.

void PCPNewCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if ((mousemode != select) ||
		(mousemode == select && selectstate != start)) {
		show_pcp_control = false;
		TemplateCanvas::OnMouseEvent(event);
		return;
	}
	
	// To create the impression of an outline control moving, we will
	// create / delete an outline object in the foreground_objects.
	// We can tell the system that just the foreground objects need
	// to be redrawn at each step, or we can possibly manually draw
	// the foreground objects ourselves.
	
	if (pcp_selectstate == pcp_start) {
		if (event.LeftDown()) {
			
			// if the mouse position is at one of the control dots, then
			// proceed, otherwise call TemplateCanvas::OnMouseEvent(event)

			int label_match = -1;
			prev = GetActualPos(event);
			sel1 = prev;
			for (int v=0; v<num_vars; v++) {
				if (control_labels[v]->pointWithin(sel1)) {
					label_match = v;
					break;
				}
			}

			int circ_match = -1;
			LOG_MSG(GenUtils::PtToStr(sel1));
			for (int v=0; v<num_vars && label_match==-1; v++) {
				wxPoint cpt = control_circs[v]->center;
				cpt.x += control_circs[v]->getXNudge();
				cpt.y += control_circs[v]->getYNudge();
				LOG_MSG(GenUtils::PtToStr(cpt));
				if (GenUtils::distance(sel1, cpt) <=
					((double) control_circs[v]->radius)+1.5) {
					circ_match = v;
					break;
				}
			}
			
			if (label_match != -1) {
				LOG_MSG(wxString::Format("Selected control_label %d",
										 label_match));
				control_label_sel = label_match;
				pcp_selectstate = pcp_leftdown_on_label;
			} else if (circ_match != -1) {
				LOG_MSG(wxString::Format("Selected control_circ %d",
										 circ_match));
				control_line_sel = circ_match;
				pcp_selectstate = pcp_leftdown_on_circ;
			} else {
				LOG_MSG(wxString::Format("No controls selected"));
				show_pcp_control = false;
				TemplateCanvas::OnMouseEvent(event);
				return;
			}
		} else {
			show_pcp_control = false;
			TemplateCanvas::OnMouseEvent(event);
			return;
		}
	} else if (pcp_selectstate == pcp_leftdown_on_label) {
		if (event.LeftUp() || event.RightUp()) {
			sel2 = GetActualPos(event);
			LOG_MSG(wxString::Format("Final mouse position on release: "
									 "(%d,%d)", sel2.x, sel2.y));
			VarLabelClicked();
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}
	} else if (pcp_selectstate == pcp_leftdown_on_circ) {
		if (event.Moving() || event.Dragging()) {
			wxPoint act_pos = GetActualPos(event);
			if (fabs((double) (prev.x - act_pos.x)) +
				fabs((double) (prev.y - act_pos.y)) > 2) {
				sel1 = prev;
				sel2 = GetActualPos(event);
				pcp_selectstate = pcp_dragging;
				
				LOG_MSG(wxString::Format("Draw control line at position "
										 "(%d,%d)", sel2.x, sel2.y));
				show_pcp_control = true;
				Refresh();
			}
		} else {
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}
	} else if (pcp_selectstate == pcp_dragging) {
		if (event.Dragging()) { // mouse moved while buttons still down
			sel2 = GetActualPos(event);
			
			LOG_MSG(wxString::Format("Draw control line at position "
									 "(%d,%d)", sel2.x, sel2.y));
			show_pcp_control = true;
			Refresh();
		} else if (event.LeftUp()) {
			sel2 = GetActualPos(event);
			LOG_MSG(wxString::Format("Final control line position: "
									 "(%d,%d)", sel2.x, sel2.y));
			MoveControlLine(sel2.y); // will invalidate layer1 if needed
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}  else if (event.RightDown()) {
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}			
	}
}

void PCPNewCanvas::VarLabelClicked()
{
	int v = var_order[control_label_sel];
	wxString msg;
	msg << "control_label_sel " << control_label_sel << " clicked which";
	msg << "\n corresponds to actual var " << v << " with name = ";
	msg << GetNameWithTime(v);
	LOG_MSG(msg);
	theme_var = v;
	ChangeThemeType(theme_type);
	TemplateLegend* tl = template_frame->GetTemplateLegend();
	if (tl) tl->Refresh();
}

void PCPNewCanvas::PaintControls(wxDC& dc)
{
	if (!show_pcp_control) return;
	// draw control line
	wxPen pen(*wxBLUE_PEN);
	pen.SetWidth(2);
	dc.SetPen(pen);
	dc.SetBrush(*wxWHITE_BRUSH);
	wxPoint cpt = control_circs[control_line_sel]->center;
	cpt.x += control_circs[control_line_sel]->getXNudge();
	cpt.y = sel2.y;
	int x_end = control_lines[control_line_sel]->points[1].x;
	
	dc.DrawLine(cpt.x, cpt.y, x_end, cpt.y);
	dc.DrawCircle(cpt, control_circs[control_line_sel]->radius);
}

/**
 Determines final location of control.  If order changes, update
 var_order, invalidate bitmaps and call PopulateCanvas.
 */
void PCPNewCanvas::MoveControlLine(int final_y)
{
	LOG_MSG("Entering PCPNewCanvas::MoveControlLine");
	LOG(control_line_sel);
	
	LOG_MSG("original var_order");
	for (int i=0; i<num_vars; i++) LOG(var_order[i]);
	
	std::vector<int> new_order(num_vars);
	// starting line is control_line_sel
	// determine which control lines final_y is between
	if (final_y < control_lines[0]->points[0].y) {
		if (control_line_sel == 0) return;
		LOG_MSG("Final control line pos is above control line 0");
		// move control line into first position
		new_order[0] = control_line_sel;
		for (int i=1; i<=control_line_sel; i++) new_order[i] = i-1;
		for (int i=control_line_sel+1; i<num_vars; i++) new_order[i] = i;
		//for (int i=0; i<num_vars; i++) LOG(new_order[i]);
	} else if (final_y > control_lines[num_vars-1]->points[0].y) {
		if (control_line_sel == num_vars - 1) return;
		LOG_MSG("Final control line pos is below last control line");
		// move control line into last position
		for (int i=0; i<control_line_sel; i++) new_order[i] = i;
		for (int i=control_line_sel; i<num_vars-1; i++) new_order[i] = i+1;
		new_order[num_vars-1] = control_line_sel;
	} else {
		for (int v=1; v<num_vars; v++) {
			if (final_y < control_lines[v]->points[0].y) {
				if (control_line_sel == v || control_line_sel == v-1) return;
				LOG_MSG(wxString::Format("Final control line pos is just "
										 "above control line %d", v));
				
				if (control_line_sel > v) {
					for (int i=0; i<v; i++) new_order[i] = i;
					new_order[v] = control_line_sel;
					for (int i=v+1; i<=control_line_sel; i++) new_order[i]=i-1;
					for (int i=control_line_sel+1; i<num_vars; i++) {
						new_order[i] = i;
					}
				} else {
					for (int i=0; i<control_line_sel; i++) new_order[i] = i;
					for (int i=control_line_sel; i<v-1; i++) new_order[i]=i+1;
					new_order[v-1] = control_line_sel;
					for (int i=v; i<num_vars; i++) new_order[i] = i;
				}
				break;
			}
		}
	}
	std::vector<int> old_var_order(num_vars);
	for (int i=0; i<num_vars; i++) old_var_order[i] = var_order[i];
	
	LOG_MSG("control lines reorder: ");
	for (int i=0; i<num_vars; i++) LOG(new_order[i]);
	
	for (int i=0; i<num_vars; i++) {
		var_order[i] = old_var_order[new_order[i]];
	}
	LOG_MSG("final var_order:");
	for (int i=0; i<num_vars; i++) LOG(var_order[i]);
	
	LOG_MSG("Exiting PCPNewCanvas::MoveControlLine");
	invalidateBms();
	PopulateCanvas();
}

void PCPNewCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		// obs: 1,3,5,... obs 1 = (1.23, 432.3, -23)
		if (total_hover_obs > 1) {
			s << "obs: " << hover_obs[0]+1 << "," << hover_obs[1]+1;
			if (total_hover_obs > 2) s << "," << hover_obs[2]+1;
			if (total_hover_obs > 3) s << "," << hover_obs[3]+1;
			if (total_hover_obs > 4) s << "," << hover_obs[4]+1;
			if (total_hover_obs > 5) s << ",...";
			s << " ";
		}
		if (total_hover_obs != 0) {
			int ob = hover_obs[0];
			s << "obs " << ob+1 << " = (";
			for (int v=0; v<num_vars-1; v++) {
				int t = var_info[var_order[v]].time;
				s << GenUtils::DblToStr(data[var_order[v]][t][ob], 3);
				s << ", ";
			}
			int t = var_info[var_order[num_vars-1]].time;
			s << GenUtils::DblToStr(data[var_order[num_vars-1]][t][ob],3);
			s << ")";
		}
	} else if (mousemode == select &&
			   (selectstate == dragging || selectstate == brushing)) {
		s << "#selected=" << highlight_state->GetTotalHighlighted();
	}
	
	sb->SetStatusText(s);
}


PCPNewLegend::PCPNewLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

PCPNewLegend::~PCPNewLegend()
{
    LOG_MSG("In PCPNewLegend::~PCPNewLegend");
}


IMPLEMENT_CLASS(PCPNewFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(PCPNewFrame, TemplateFrame)
	EVT_ACTIVATE(PCPNewFrame::OnActivate)
END_EVENT_TABLE()

PCPNewFrame::PCPNewFrame(wxFrame *parent, Project* project,
								 const std::vector<GeoDaVarInfo>& var_info,
								 const std::vector<int>& col_ids,
								 const wxString& title,
								 const wxPoint& pos,
								 const wxSize& size,
								 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering PCPNewFrame::PCPNewFrame");
	
	int width, height;
	GetClientSize(&width, &height);

	wxSplitterWindow* splitter_win = 0;
	splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	template_canvas = new PCPNewCanvas(splitter_win, this, project,
									   var_info, col_ids,
									   wxDefaultPosition,
									   wxSize(width,height));

	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
	template_legend = new PCPNewLegend(splitter_win,
									   template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								GeoDaConst::bubble_chart_default_legend_width);
	
	Show(true);
	LOG_MSG("Exiting PCPNewFrame::PCPNewFrame");
}

PCPNewFrame::~PCPNewFrame()
{
	LOG_MSG("In PCPNewFrame::~PCPNewFrame");
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void PCPNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In PCPNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("PCPNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void PCPNewFrame::MapMenus()
{
	LOG_MSG("In PCPNewFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_PCP_NEW_PLOT_VIEW_MENU_OPTIONS");
	((PCPNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((PCPNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void PCPNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("PCPNewFrame::UpdateOptionMenuItems: Options "
				"menu not found");
	} else {
		((PCPNewCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void PCPNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void PCPNewFrame::update(FramesManager* o)
{
	LOG_MSG("In PCPNewFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
}

void PCPNewFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void PCPNewFrame::OnShowAxes(wxCommandEvent& event)
{
	LOG_MSG("In PCPNewFrame::OnShowAxes");
	PCPNewCanvas* t = (PCPNewCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void PCPNewFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	LOG_MSG("In PCPNewFrame::OnDisplayStatistics");
	PCPNewCanvas* t = (PCPNewCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void PCPNewFrame::OnViewOriginalData(wxCommandEvent& event)
{
	LOG_MSG("In PCPNewFrame::OnViewOriginalData");
	PCPNewCanvas* t = (PCPNewCanvas*) template_canvas;
	t->StandardizeData(false);
	UpdateOptionMenuItems();
}

void PCPNewFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	LOG_MSG("In PCPNewFrame::OnViewStandardizedData");
	PCPNewCanvas* t = (PCPNewCanvas*) template_canvas;
	t->StandardizeData(true);
	UpdateOptionMenuItems();
}

void PCPNewFrame::OnThemeless(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::no_theme);
}

void PCPNewFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::hinge_15);
}

void PCPNewFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::hinge_30);
}

void PCPNewFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::quantile);
}

void PCPNewFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::percentile);
}

void PCPNewFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::stddev);
}

void PCPNewFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::unique_values);
}

void PCPNewFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::natural_breaks);
}

void PCPNewFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeThemeType(ThemeUtilities::equal_intervals);
}

void PCPNewFrame::ChangeThemeType(ThemeUtilities::ThemeType new_theme)
{
	((PCPNewCanvas*) template_canvas)->ForgetNumCats();
	((PCPNewCanvas*) template_canvas)->ChangeThemeType(new_theme);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Refresh();
}

void PCPNewFrame::OnSaveCategories(wxCommandEvent& event)
{
	((PCPNewCanvas*) template_canvas)->OnSaveCategories();
}


