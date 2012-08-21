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
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../DialogTools/HistIntervalDlg.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../OpenGeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "HistogramView.h"

IMPLEMENT_CLASS(HistogramCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(HistogramCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int HistogramCanvas::MAX_INTERVALS = 200;
const int HistogramCanvas::default_intervals = 7;
const double HistogramCanvas::left_pad_const = 0;
const double HistogramCanvas::right_pad_const = 0;
const double HistogramCanvas::interval_width_const = 10;
const double HistogramCanvas::interval_gap_const = 0;

/**
 Want to have an interactive interval size slider.  Will always
 cap number of intervals at 200.  So, max_intervals determines
 the minimum interval size allowed and data range determines
 the max interval size allowed.  Interactive slider could appear
 as an embedded control.  Perhaps bring it up as part of the "Intervals"
 window.  The user can either specify the number of intervals, or can
 specify the interval size.
 
 Colors will either be all the same, or will offer one or two color
 schemes, perhaps randomly assigned.  Actually, we should use a 
 color scheme by default so that it will be easier to follow intervals
 over time.
 
 Could have an optional display stats which will indicate number of
 items in each category, category range, and number currently selected.
 
 User can use mouse rectangle to select several categories at once or
 can point and shift select.
 
 Keep in mind a special data Weights viewing mode where we always
 indicate the 0 category and we deal with integers.  Perhaps should
 have ability to treat data as integers as a built-in mode.  Constructor
 will either take var_info or we can pass in connectivity data
 from a shape file.
 */

HistogramCanvas::HistogramCanvas(wxWindow *parent, TemplateFrame* t_frame,
								   Project* project_s,
								   const std::vector<GeoDaVarInfo>& v_info,
								   const std::vector<int>& col_ids,
								   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), var_info(v_info), num_obs(project_s->GetNumRecords()),
num_time_vals(1),
highlight_state(project_s->highlight_state),
x_axis(0), y_axis(0), display_stats(false), show_axes(true),
scale_x_over_time(true), scale_y_over_time(true)
{
	using namespace Shapefile;
	LOG_MSG("Entering HistogramCanvas::HistogramCanvas");
	template_frame = t_frame;	
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	std::vector<d_array_type> data(v_info.size());
	
	sel_scratch.resize(num_obs);
	grid_base->GetColData(col_ids[0], data[0]);
	int data0_times = data[0].shape()[0];
	data_stats.resize(data0_times);
	hinge_stats.resize(data0_times);
	data_sorted.resize(data0_times);
	data_min_over_time = data[0][0][0];
	data_max_over_time = data[0][0][0];
	for (int t=0; t<data0_times; t++) {
		data_sorted[t].resize(num_obs);
		for (int i=0; i<num_obs; i++) {
			data_sorted[t][i].first = data[0][t][i];
			data_sorted[t][i].second = i;
		}
		std::sort(data_sorted[t].begin(), data_sorted[t].end(),
				  GeoDa::dbl_int_pair_cmp_less);
		data_stats[t].CalculateFromSample(data_sorted[t]);
		hinge_stats[t].CalculateHingeStats(data_sorted[t]);
		if (data_stats[t].min < data_min_over_time) {
			data_min_over_time = data_stats[t].min;
		}
		if (data_stats[t].max > data_max_over_time) {
			data_max_over_time = data_stats[t].max;
		}
	}
	
	obs_id_to_ival.resize(boost::extents[data0_times][num_obs]);
	max_intervals = GenUtils::min<int>(MAX_INTERVALS, num_obs);
	cur_intervals = GenUtils::min<int>(max_intervals, default_intervals);
	if (num_obs > 49) {
		int c = sqrt((double) num_obs);
		cur_intervals = GenUtils::min<int>(max_intervals, c);
		cur_intervals = GenUtils::min<int>(cur_intervals, 25);
	}
	ival_size.resize(data0_times);
	ival_ref_pt.resize(data0_times);
	min_ival_val.resize(data0_times);
	max_ival_val.resize(data0_times);
	max_num_obs_in_ival.resize(data0_times);
	ival_to_obs_ids.resize(data0_times);
	
	InitIntervals();
	
	highlight_color = GeoDaConst::highlight_color;
	
	fixed_aspect_ratio_mode = false;
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	ref_var_index = var_info[0].is_time_variant ? 0 : -1;	
	VarInfoAttributeChange();
	PopulateCanvas();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting HistogramCanvas::HistogramCanvas");
}

HistogramCanvas::~HistogramCanvas()
{
	LOG_MSG("Entering HistogramCanvas::~HistogramCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting HistogramCanvas::~HistogramCanvas");
}

void HistogramCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering HistogramCanvas::DisplayRightClickMenu");
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_HISTOGRAM_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting HistogramCanvas::DisplayRightClickMenu");
}

void HistogramCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!var_info[0].is_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	{
		wxString s;
		s << "Synchronize " << var_info[0].name << " with Time Control";
		wxMenuItem* mi =
		menu1->AppendCheckItem(GeoDaConst::ID_TIME_SYNC_VAR1+0, s, s);
		mi->Check(var_info[0].sync_with_global_time);
	}
	
	/*
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	{
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

void HistogramCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	if (var_info[0].is_time_variant) {
		GeneralWxUtils::CheckMenuItem(menu,
									  GeoDaConst::ID_TIME_SYNC_VAR1,
									  var_info[0].sync_with_global_time);
		GeneralWxUtils::CheckMenuItem(menu,
									  GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
									  var_info[0].fixed_scale);
	}
}

void HistogramCanvas::DetermineMouseHoverObjects()
{
	total_hover_obs = 0;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (selectable_shps[i]->pointWithin(sel1)) {
			hover_obs[total_hover_obs++] = i;
			if (total_hover_obs == max_hover_obs) break;
		}
	}
}

// The following function assumes that the set of selectable objects
// all rectangles.
void HistogramCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	bool rect_sel = (!pointsel && (brushtype == rectangle));
	
	int t = var_info[0].time;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	int total_sel_shps = selectable_shps.size();
	
	wxPoint lower_left;
	wxPoint upper_right;
	if (rect_sel) {
		GenUtils::StandardizeRect(sel1, sel2, lower_left, upper_right);
	}
	if (!shiftdown) {
		bool any_selected = false;
		for (int i=0; i<total_sel_shps; i++) {
			MyRectangle* rec = (MyRectangle*) selectable_shps[i];
			if ((pointsel && rec->pointWithin(sel1)) ||
				(rect_sel &&
				 GenUtils::RectsIntersect(rec->lower_left, rec->upper_right,
										  lower_left, upper_right))) {
				//LOG_MSG(wxString::Format("ival %d selected", i));
				any_selected = true;
				break;
			} else {
				//LOG_MSG(wxString::Format("ival %d not selected", i));
				//LOG_MSG(wxString::Format(""));
			}
		}
		if (!any_selected) {
			highlight_state->SetEventType(HighlightState::unhighlight_all);
			highlight_state->notifyObservers();
			return;
		}
	}
	
	for (int i=0; i<total_sel_shps; i++) {
		MyRectangle* rec = (MyRectangle*) selectable_shps[i];
		bool selected = ((pointsel && rec->pointWithin(sel1)) ||
						 (rect_sel &&
						  GenUtils::RectsIntersect(rec->lower_left,
												   rec->upper_right,
												   lower_left, upper_right)));
		bool all_sel = (ival_obs_cnt[t][i] == ival_obs_sel_cnt[t][i]);
		if (pointsel && all_sel && selected) {
			// unselect all in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
				 it != ival_to_obs_ids[t][i].end(); it++) {
				nuh[total_newly_unselected++] = (*it);
			}
		} else if (!all_sel && selected) {
			// select currently unselected in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
				 it != ival_to_obs_ids[t][i].end(); it++) {
				if (hs[*it]) continue;
				nh[total_newly_selected++] = (*it);
			}
		} else if (!selected && !shiftdown) {
			// unselect all selected in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
				 it != ival_to_obs_ids[t][i].end(); it++) {
		if (!hs[*it]) continue;
				nuh[total_newly_unselected++] = (*it);
			}
		}
	}
	
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	UpdateStatusBar();
}

void HistogramCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	int t = var_info[0].time;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_cnt[t][i] == 0) continue;
		selectable_shps[i]->paintSelf(dc);
	}
}

void HistogramCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	dc.SetPen(wxPen(highlight_color));
	dc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	int t = var_info[0].time;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_sel_cnt[t][i] == 0) continue;
		double s = (((double) ival_obs_sel_cnt[t][i]) /
					((double) ival_obs_cnt[t][i]));
		MyRectangle* rec = (MyRectangle*) selectable_shps[i];
		dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
						 rec->upper_right.x - rec->lower_left.x,
						 (rec->upper_right.y - rec->lower_left.y)*s);
	}	
}

/** Override of TemplateCanvas method. */
void HistogramCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering HistogramCanvas::update");
	
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	
	HighlightState::EventType type = highlight_state->GetEventType();
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
	
	UpdateIvalSelCnts();
	
	Refresh();
	
	LOG_MSG("Entering HistogramCanvas::update");	
}

wxString HistogramCanvas::GetCanvasTitle()
{
	DbfGridTableBase* gb = project->GetGridBase();
	wxString s("Histogram: ");	
	s << GetNameWithTime(0);
	return s;
}

wxString HistogramCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void HistogramCanvas::PopulateCanvas()
{
	LOG_MSG("Entering HistogramCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	int time = var_info[0].time;
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
		+ interval_width_const * cur_intervals + 
		+ interval_gap_const * (cur_intervals-1);
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		orig_x_pos[i] = left_pad_const + interval_width_const/2.0
		+ i * (interval_width_const + interval_gap_const);
	}
	
	shps_orig_xmin = x_min;
	shps_orig_xmax = x_max;
	shps_orig_ymin = 0;
	shps_orig_ymax = (scale_y_over_time ? overall_max_num_obs_in_ival :
					  max_num_obs_in_ival[time]);
	if (show_axes) {
		axis_scale_y = AxisScale(0, shps_orig_ymax, 5);
		shps_orig_ymax = axis_scale_y.scale_max;
		y_axis = new MyAxis("Frequency", axis_scale_y,
							wxRealPoint(0,0), wxRealPoint(0, shps_orig_ymax),
							-9, 0);
		background_shps.push_back(y_axis);
		
		axis_scale_x = AxisScale(0, max_ival_val[time]);
		//shps_orig_xmax = axis_scale_x.scale_max;
		axis_scale_x.data_min = min_ival_val[time];
		axis_scale_x.data_max = max_ival_val[time];
		axis_scale_x.scale_min = axis_scale_x.data_min;
		axis_scale_x.scale_max = axis_scale_x.data_max;
		double range = axis_scale_x.scale_max - axis_scale_x.scale_min;
		LOG(axis_scale_x.data_max);
		axis_scale_x.scale_range = range;
		axis_scale_x.p = floor(log10(range));
		axis_scale_x.ticks = cur_intervals+1;
		axis_scale_x.tics.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str_show.resize(axis_scale_x.tics_str.size());
		for (int i=0; i<axis_scale_x.ticks; i++) {
			axis_scale_x.tics[i] =
				axis_scale_x.data_min +
					range*((double) i)/((double) axis_scale_x.ticks-1);
			LOG(axis_scale_x.tics[i]);
			std::ostringstream ss;
			ss << std::setprecision(3) << axis_scale_x.tics[i];
			axis_scale_x.tics_str[i] = ss.str();
			axis_scale_x.tics_str_show[i] = false;
		}
		int tick_freq = ceil(((double) cur_intervals)/10.0);
		for (int i=0; i<axis_scale_x.ticks; i++) {
			if (i % tick_freq == 0) {
				axis_scale_x.tics_str_show[i] = true;
			}
		}
		axis_scale_x.tic_inc = axis_scale_x.tics[1]-axis_scale_x.tics[0];
		x_axis = new MyAxis(GetNameWithTime(0), axis_scale_x, wxRealPoint(0,0),
							wxRealPoint(shps_orig_xmax, 0), 0, 9);
		background_shps.push_back(x_axis);
	}
	
	MyShape* s = 0;
	int table_w=0, table_h=0;
	if (display_stats) {
		int y_d = show_axes ? 0 : -32;
		int cols = 1;
		int rows = 5;
		std::vector<wxString> vals(rows);
		vals[0] << "from";
		vals[1] << "to";
		vals[2] << "#obs";
		vals[3] << "% of total";
		vals[4] << "sd from mean";
		std::vector<MyTable::CellAttrib> attribs(0); // undefined
		s = new MyTable(vals, attribs, rows, cols, *GeoDaConst::small_font,
						wxRealPoint(0, 0), MyText::h_center, MyText::top,
						MyText::right, MyText::v_center, 3, 10, -62, 53+y_d);
		background_shps.push_back(s);
		{
			wxClientDC dc(this);
			((MyTable*) s)->GetSize(dc, table_w, table_h);
		}
		for (int i=0; i<cur_intervals; i++) {
			int t = time;
			std::vector<wxString> vals(rows);
			double ival_min = (i == 0) ? min_ival_val[t] : ival_breaks[t][i-1];
			double ival_max = ((i == cur_intervals-1) ?
							   max_ival_val[t] : ival_breaks[t][i]);
			double p = 100.0*((double) ival_obs_cnt[t][i])/((double) num_obs);
			double sd = data_stats[t].sd_with_bessel;
			double mean = data_stats[t].mean;
			double sd_d = 0;
			if (ival_max < mean && sd > 0) {
				sd_d = (ival_max - mean)/sd;
			} else if (ival_min > mean && sd > 0) {
				sd_d = (ival_min - mean)/sd;
			}
			vals[0] << GenUtils::DblToStr(ival_min, 3);
			vals[1] << GenUtils::DblToStr(ival_max, 3);
			vals[2] << ival_obs_cnt[t][i];
			vals[3] << GenUtils::DblToStr(p, 3);
			vals[4] << GenUtils::DblToStr(sd_d, 3);
			
			std::vector<MyTable::CellAttrib> attribs(0); // undefined
			s = new MyTable(vals, attribs, rows, cols, *GeoDaConst::small_font,
							wxRealPoint(orig_x_pos[i], 0),
							MyText::h_center, MyText::top,
							MyText::h_center, MyText::v_center, 3, 10, 0,
							53+y_d);
			background_shps.push_back(s);
		}
		
		wxString sts;
		sts << "min: " << data_stats[time].min;
		sts << ", max: " << data_stats[time].max;
		sts << ", median: " << hinge_stats[time].Q2;
		sts << ", mean: " << data_stats[time].mean;
		sts << ", s.d.: " << data_stats[time].sd_with_bessel;
		sts << ", #obs: " << num_obs;
	
		s = new MyText(sts, *GeoDaConst::small_font,
					   wxRealPoint(shps_orig_xmax/2.0, 0), 0,
					   MyText::h_center, MyText::v_center, 0,
					   table_h + 70 + y_d); //145+y_d);
		background_shps.push_back(s);
	}
	
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25;
	virtual_screen_marg_left = 25;
	virtual_screen_marg_right = 25;
	
	if (show_axes || display_stats) {
		if (!display_stats) {
			virtual_screen_marg_bottom += 32;
			virtual_screen_marg_left += 35;
		} else {
			int y_d = show_axes ? 0 : -35;
			virtual_screen_marg_bottom += table_h + 65 + y_d; //135 + y_d;
			virtual_screen_marg_left += 82;
		}
	}
		
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

	selectable_shps.resize(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		double x0 = orig_x_pos[i] - interval_width_const/2.0;
		double x1 = orig_x_pos[i] + interval_width_const/2.0;
		double y0 = 0;
		double y1 = ival_obs_cnt[time][i];
		selectable_shps[i] = new MyRectangle(wxRealPoint(x0, 0),
											 wxRealPoint(x1, y1));
		int sz = GeoDaConst::qualitative_colors.size();
		selectable_shps[i]->setPen(GeoDaConst::qualitative_colors[i%sz]);
		selectable_shps[i]->setBrush(GeoDaConst::qualitative_colors[i%sz]);
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting HistogramCanvas::PopulateCanvas");
}

void HistogramCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering HistogramCanvas::TitleOrTimeChange");
	if (!is_any_sync_with_global_time) return;
	
	var_info[0].time = project->GetGridBase()->curr_time_step;

	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting HistogramCanvas::TitleOrTimeChange");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void HistogramCanvas::VarInfoAttributeChange()
{
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	if (var_info[0].is_time_variant) is_any_time_variant = true;
	if (var_info[0].sync_with_global_time) {
		is_any_sync_with_global_time = true;
	}
	ref_var_index = -1;
	num_time_vals = 1;
	if (var_info[0].is_ref_variable) ref_var_index = 0;
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	//GeoDa::PrintVarInfoVector(var_info);
}

void HistogramCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In HistogramCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In HistogramCanvas::FixedScaleVariableToggle");
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::HistogramIntervals()
{
	HistIntervalDlg dlg(1, cur_intervals, max_intervals, this);
	if (dlg.ShowModal () != wxID_OK) return;
	if (cur_intervals == dlg.num_intervals) return;
	cur_intervals = dlg.num_intervals;
	InitIntervals();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** based on data_min_over_time, data_max_over_time,
 cur_intervals, scale_x_over_time:
 calculate interval breaks and populate
 obs_id_to_ival, ival_obs_cnt and ival_obs_sel_cnt */ 
void HistogramCanvas::InitIntervals()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	int ts = obs_id_to_ival.shape()[0];
	ival_breaks.resize(boost::extents[ts][cur_intervals-1]);
	ival_obs_cnt.resize(boost::extents[ts][cur_intervals]);
	ival_obs_sel_cnt.resize(boost::extents[ts][cur_intervals]);
	for (int t=0; t<ts; t++) ival_to_obs_ids[t].clear();
	for (int t=0; t<ts; t++) ival_to_obs_ids[t].resize(cur_intervals);
	for (int t=0; t<ts; t++) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_cnt[t][i] = 0;
			ival_obs_sel_cnt[t][i] = 0;
		}
	}
	for (int t=0; t<ts; t++) {
		LOG_MSG(wxString::Format("t = %d", t));
		if (scale_x_over_time) {
			min_ival_val[t] = data_min_over_time;
			max_ival_val[t] = data_max_over_time;
		} else {
			min_ival_val[t] = data_sorted[t][0].second;
			max_ival_val[t] = data_sorted[t][num_obs-1].second;
		}
		if (min_ival_val[t] == max_ival_val[t]) {
			if (min_ival_val[t] == 0) {
				max_ival_val[t] = 1;
			} else {
				max_ival_val[t] += fabs(max_ival_val[t])/2.0;
			}
		}
		double range = max_ival_val[t] - min_ival_val[t];
		ival_size[t] = range/cur_intervals;
		ival_ref_pt[t] = min_ival_val[t];
		
		for (int i=0; i<cur_intervals-1; i++) {
			ival_breaks[t][i] = min_ival_val[t]+ival_size[t]*((double) (i+1));
		}
		for (int i=0, cur_ival=0; i<num_obs; i++) {
			while (cur_ival <= cur_intervals-2 &&
				   data_sorted[t][i].first >= ival_breaks[t][cur_ival]) {
				cur_ival++;
			}
			ival_to_obs_ids[t][cur_ival].push_front(data_sorted[t][i].second);
			obs_id_to_ival[t][data_sorted[t][i].second] = cur_ival;
			ival_obs_cnt[t][cur_ival]++;
			if (hs[data_sorted[t][i].second]) {
				ival_obs_sel_cnt[t][cur_ival]++;
			}
		}
	}
	overall_max_num_obs_in_ival = 0;
	for (int t=0; t<ts; t++) {
		max_num_obs_in_ival[t] = 0;
		for (int i=0; i<cur_intervals; i++) {
			if (ival_obs_cnt[t][i] > max_num_obs_in_ival[t]) {
				max_num_obs_in_ival[t] = ival_obs_cnt[t][i];
			}
		}
		if (max_num_obs_in_ival[t] > overall_max_num_obs_in_ival) {
			overall_max_num_obs_in_ival = max_num_obs_in_ival[t];
		}
	}
	LOG_MSG("InitIntervals: ");
	for (int t=0; t<ts; t++) {
		LOG_MSG(wxString::Format("time %d:", t));
		//LOG_MSG(wxString::Format(""));
		LOG_MSG(wxString::Format("min_ival_val: %f", min_ival_val[t]));
		LOG_MSG(wxString::Format("max_ival_val: %f", max_ival_val[t]));
		for (int i=0; i<cur_intervals; i++) {
			LOG_MSG(wxString::Format("ival_obs_cnt[%d][%d] = %d",
									 t, i, ival_obs_cnt[t][i]));
		}
	}
}

void HistogramCanvas::UpdateIvalSelCnts()
{
	int ts = obs_id_to_ival.shape()[0];	
	HighlightState::EventType type = highlight_state->GetEventType();
	if (type == HighlightState::unhighlight_all) {
		for (int t=0; t<ts; t++) {
			for (int i=0; i<cur_intervals; i++) {
				ival_obs_sel_cnt[t][i] = 0;
			}
		}
	} else if (type == HighlightState::delta) {
		std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
		std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
		int nh_cnt = highlight_state->GetTotalNewlyHighlighted();
		int nuh_cnt = highlight_state->GetTotalNewlyUnhighlighted();
		
		for (int i=0; i<nh_cnt; i++) {
			for (int t=0; t<ts; t++) {
				ival_obs_sel_cnt[t][obs_id_to_ival[t][nh[i]]]++;
			}
		}
		for (int i=0; i<nuh_cnt; i++) {
			for (int t=0; t<ts; t++) {
				ival_obs_sel_cnt[t][obs_id_to_ival[t][nuh[i]]]--;
			}
		}
	} else if (type == HighlightState::invert) {
		for (int t=0; t<ts; t++) {
			for (int i=0; i<cur_intervals; i++) {
				ival_obs_sel_cnt[t][i] = 
					ival_obs_cnt[t][i] - ival_obs_sel_cnt[t][i];
			}
		}
	}
}

void HistogramCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	if (total_hover_obs == 0) {
		sb->SetStatusText("");
		return;
	}
	int t = var_info[0].time;
	int ival = hover_obs[0];
	wxString s;
	double ival_min = (ival == 0) ? min_ival_val[t] : ival_breaks[t][ival-1];
	double ival_max = ((ival == cur_intervals-1) ?
					   max_ival_val[t] : ival_breaks[t][ival]);
	s << "bin: " << ival+1 << ", range: [" << ival_min << ", " << ival_max;
	s << (ival == cur_intervals-1 ? "]" : ")");
	s << ", #obs: " << ival_obs_cnt[t][ival];
	double p = 100.0*((double) ival_obs_cnt[t][ival])/((double) num_obs);
	s << ", %tot: ";
	s << wxString::Format("%.1f", p);
	s << "%, #sel: " << ival_obs_sel_cnt[t][ival];
	double sd = data_stats[t].sd_with_bessel;
	double mean = data_stats[t].mean;
	double sd_d = 0;
	if (ival_max < mean && sd > 0) {
		sd_d = (ival_max - mean)/sd;
	} else if (ival_min > mean && sd > 0) {
		sd_d = (ival_min - mean)/sd;
	}
	s << ", sd from mean: " << GenUtils::DblToStr(sd_d, 3);

	sb->SetStatusText(s);
}


IMPLEMENT_CLASS(HistogramFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(HistogramFrame, TemplateFrame)
	EVT_ACTIVATE(HistogramFrame::OnActivate)
END_EVENT_TABLE()

HistogramFrame::HistogramFrame(wxFrame *parent, Project* project,
								 const std::vector<GeoDaVarInfo>& var_info,
								 const std::vector<int>& col_ids,
								 const wxString& title,
								 const wxPoint& pos,
								 const wxSize& size,
								 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering HistogramFrame::HistogramFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new HistogramCanvas(this, this, project,
										   var_info, col_ids,
										   wxDefaultPosition,
										   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
		
	Show(true);
	LOG_MSG("Exiting HistogramFrame::HistogramFrame");
}

HistogramFrame::~HistogramFrame()
{
	LOG_MSG("In HistogramFrame::~HistogramFrame");
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void HistogramFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In HistogramFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("HistogramFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void HistogramFrame::MapMenus()
{
	LOG_MSG("In HistogramFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_HISTOGRAM_NEW_VIEW_MENU_OPTIONS");
	((HistogramCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((HistogramCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void HistogramFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("HistogramFrame::UpdateOptionMenuItems: Options "
				"menu not found");
	} else {
		((HistogramCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void HistogramFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void HistogramFrame::update(FramesManager* o)
{
	LOG_MSG("In HistogramFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
}

void HistogramFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void HistogramFrame::OnShowAxes(wxCommandEvent& event)
{
	LOG_MSG("In HistogramFrame::OnShowAxes");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void HistogramFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	LOG_MSG("In HistogramFrame::OnDisplayStatistics");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void HistogramFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	LOG_MSG("In HistogramFrame::OnDisplayStatistics");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->HistogramIntervals();
}
