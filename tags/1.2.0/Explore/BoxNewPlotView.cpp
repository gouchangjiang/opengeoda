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
#include "../DialogTools/MapQuantileDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../OpenGeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "BoxNewPlotView.h"

IMPLEMENT_CLASS(BoxNewPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(BoxNewPlotCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int BoxNewPlotCanvas::MAX_BOX_PLOTS = 100;
const double BoxNewPlotCanvas::plot_height_const = 100;
const double BoxNewPlotCanvas::left_pad_const = 0;
const double BoxNewPlotCanvas::right_pad_const = 0;
const double BoxNewPlotCanvas::plot_width_const = 10;
const double BoxNewPlotCanvas::plot_gap_const = 20;

/** Box Plot view is somewhat different than other views because support
 the display of multiple box plots on the screen at the same time.  For
 this reason, data_sorted and hinge_stats will store data for all
 time periods when possible and will not resize when global time
 synch is turned off.  Any class than inherits from this class, such
 as Lisa Box Plot, will have to override this behaviour if they want
 to support multiple variables */

BoxNewPlotCanvas::BoxNewPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
								   Project* project_s,
								   const std::vector<GeoDaVarInfo>& v_info,
								   const std::vector<int>& col_ids,
								   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), var_info(v_info), num_obs(project_s->GetNumRecords()),
num_time_vals(1), data(v_info.size()),
highlight_state(project_s->highlight_state),
vert_axis(0), display_stats(false), show_axes(true),
hinge_15(true)
{
	using namespace Shapefile;
	LOG_MSG("Entering BoxNewPlotCanvas::BoxNewPlotCanvas");
	template_frame = t_frame;	
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	sel_scratch.resize(num_obs);
	grid_base->GetColData(col_ids[0], data[0]);
	int data0_times = data[0].shape()[0];
	hinge_stats.resize(data0_times);
	data_stats.resize(data0_times);
	data_sorted.resize(data0_times);
	for (int t=0; t<data0_times; t++) {
		data_sorted[t].resize(num_obs);
		for (int i=0; i<num_obs; i++) {
			data_sorted[t][i].first = data[0][t][i];
			data_sorted[t][i].second = i;
		}
		std::sort(data_sorted[t].begin(), data_sorted[t].end(),
				  GeoDa::dbl_int_pair_cmp_less);
		hinge_stats[t].CalculateHingeStats(data_sorted[t]);
		data_stats[t].CalculateFromSample(data_sorted[t]);
	}

	max_plots = GenUtils::min<int>(MAX_BOX_PLOTS, var_info[0].is_time_variant ?
								   project->GetGridBase()->GetTimeSteps() : 1);
	cur_num_plots = max_plots;
	cur_first_ind = var_info[0].time_min;
	cur_last_ind = var_info[0].time_max;
	
	// NOTE: define Box Plot defaults
	selectable_fill_color = GeoDaConst::map_default_fill_colour;
	highlight_color = GeoDaConst::highlight_color;
	
	fixed_aspect_ratio_mode = false;
	use_category_brushes = false;
	selectable_shps_type = mixed;
	
	ref_var_index = var_info[0].is_time_variant ? 0 : -1;
	VarInfoAttributeChange();
	PopulateCanvas();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting BoxNewPlotCanvas::BoxNewPlotCanvas");
}

BoxNewPlotCanvas::~BoxNewPlotCanvas()
{
	LOG_MSG("Entering BoxNewPlotCanvas::~BoxNewPlotCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting BoxNewPlotCanvas::~BoxNewPlotCanvas");
}

void BoxNewPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering BoxNewPlotCanvas::DisplayRightClickMenu");
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_BOX_NEW_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting BoxNewPlotCanvas::DisplayRightClickMenu");
}

void BoxNewPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
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
	
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	{
		wxString s;
		s << "Fixed scale over time";
		wxMenuItem* mi =
		menu2->AppendCheckItem(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1, s, s);
		mi->Check(var_info[0].fixed_scale);
	}
	
	wxMenu* menu3 = new wxMenu(wxEmptyString);
	{
		int mppv = GeoDaConst::max_plots_per_view_menu_items;
		int mp = GenUtils::min<int>(max_plots, mppv);
		for (int i=0; i<mp-1; i++) {
			wxString s;
			s << i+1;
			wxMenuItem* mi =
			menu3->AppendCheckItem(GeoDaConst::ID_PLOTS_PER_VIEW_1+i, s, s);
			mi->Check(i+1 == cur_num_plots);
		}
		if (max_plots > 10) {
			wxString s;
			s << "Other...";
			wxMenuItem* mi =
			menu3->AppendCheckItem(GeoDaConst::ID_PLOTS_PER_VIEW_OTHER, s, s);
		}
		wxString s;
		if (project->GetGridBase()->GetTimeSteps() <= MAX_BOX_PLOTS) {
			s << "All";
		} else {
			s << MAX_BOX_PLOTS << " (max plots allowed)";
		}
		wxMenuItem* mi =
		menu3->AppendCheckItem(GeoDaConst::ID_PLOTS_PER_VIEW_ALL, s, s);
		mi->Check(cur_num_plots == max_plots);
	}
	
	menu->Prepend(wxID_ANY, "Number of Box Plots", menu3,
				  "Number of Box Plots");
	menu->Prepend(wxID_ANY, "Scale Options", menu2, "Scale Options");
	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

void BoxNewPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_15"),
								  hinge_15);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_30"),
								  !hinge_15);
	
	if (var_info[0].is_time_variant) {
		GeneralWxUtils::CheckMenuItem(menu,
									  GeoDaConst::ID_TIME_SYNC_VAR1,
									  var_info[0].sync_with_global_time);
		GeneralWxUtils::CheckMenuItem(menu,
									  GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
									  var_info[0].fixed_scale);
		int mppv = GeoDaConst::max_plots_per_view_menu_items;
		int mp = GenUtils::min<int>(max_plots, mppv);
		for (int i=0; i<mp-1; i++) {
			GeneralWxUtils::CheckMenuItem(menu,
										  GeoDaConst::ID_PLOTS_PER_VIEW_1+i,
										  i+1 == cur_num_plots);
		}
		GeneralWxUtils::CheckMenuItem(menu,
									  GeoDaConst::ID_PLOTS_PER_VIEW_ALL,
									  cur_num_plots == max_plots);
	}
}

void BoxNewPlotCanvas::DetermineMouseHoverObjects()
{
	total_hover_obs = 0;
	const double r2 = GeoDaConst::my_point_click_radius;
	
	for (int i=0; i<num_obs; i++) sel_scratch[i] = false;
	for (int t=0; t<cur_num_plots; t++) {
		for (int i=0; i<num_obs; i++) {
			sel_scratch[i] = sel_scratch[i] ||
				GenUtils::distance_sqrd(selectable_shps[t*num_obs + i]->center,
										sel1) <= 16.5;
		}
	}
	for (int i=0; i<num_obs && total_hover_obs<max_hover_obs; i++) {
		if (sel_scratch[i]) hover_obs[total_hover_obs++] = i;
	}
}

// The following function assumes that the set of selectable objects
// being selected against are all points.  Since all MyShape objects
// define a center point, this is also the default function for
// all MyShape selectable objects.
void BoxNewPlotCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	//LOG_MSG("Entering BoxNewPlotCanvas::UpdateSelectionPoints");
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	for (int i=0; i<num_obs; i++) sel_scratch[i] = false;
	
	if (pointsel) { // a point selection
		for (int t=0; t<cur_num_plots; t++) {
			for (int i=0; i<num_obs; i++) {
				sel_scratch[i] = sel_scratch[i] ||
				selectable_shps[t*num_obs + i]->pointWithin(sel1);
			}
		}
	} else {
		// assume brush is a rectangle
		wxRegion rect(wxRect(sel1, sel2));
		for (int t=0; t<cur_num_plots; t++) {
			for (int i=0; i<num_obs; i++) {
				sel_scratch[i] = sel_scratch[i] ||
					(rect.Contains(selectable_shps[t*num_obs + i]->center) !=
					 wxOutRegion);
			}
		}
	}
	for (int i=0; i<num_obs; i++) {
		if (!shiftdown) {
			if (sel_scratch[i]) {
				if (!hs[i]) nh[total_newly_selected++] = i;
			} else {
				if (hs[i]) nuh[total_newly_unselected++] = i;
			}
		} else { // do not unhighlight if not in intersection region
			if (sel_scratch[i] && !hs[i]) {
				nh[total_newly_selected++] = i;
			}
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	//LOG_MSG("Exiting BoxNewPlotCanvas::UpdateSelectionPoints");
}

void BoxNewPlotCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	LOG_MSG("In BoxNewPlotCanvas::DrawSelectableShapes");
	int radius = GeoDaConst::my_point_click_radius;
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		int min_IQR = hinge_stats[t].min_IQR_ind;
		int max_IQR = hinge_stats[t].max_IQR_ind;
		int ind_base = (t-cur_first_ind)*num_obs;
		dc.SetPen(GeoDaConst::boxplot_point_color);
		dc.SetBrush(*wxWHITE_BRUSH);
		for (int i=0; i<min_IQR; i++) {
			int ind = ind_base + data_sorted[t][i].second;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		for (int i=max_IQR+1; i<num_obs; i++) {
			int ind = ind_base + data_sorted[t][i].second;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		int iqr_s = GenUtils::max<double>(min_IQR, 0);
		int iqr_t = GenUtils::min<double>(max_IQR, num_obs-1);
		dc.SetPen(GeoDaConst::boxplot_q1q2q3_color);
		dc.SetBrush(GeoDaConst::boxplot_q1q2q3_color);
		for (int i=iqr_s; i<=iqr_t; i++) {
			int ind = ind_base + data_sorted[t][i].second;
			MyRectangle* rec = (MyRectangle*) selectable_shps[ind];
			dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
							 rec->upper_right.x - rec->lower_left.x,
							 rec->upper_right.y - rec->lower_left.y);
		}
	}	
}

void BoxNewPlotCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	int radius = 3;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	dc.SetBrush(highlight_color);
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		int min_IQR = hinge_stats[t].min_IQR_ind;
		int max_IQR = hinge_stats[t].max_IQR_ind;
		int ind_base = (t-cur_first_ind)*num_obs;
		dc.SetPen(*wxRED_PEN);
		for (int i=0; i<min_IQR; i++) {
			if (!hs[data_sorted[t][i].second]) continue;
			int ind = ind_base + data_sorted[t][i].second;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		for (int i=max_IQR+1; i<num_obs; i++) {
			if (!hs[data_sorted[t][i].second]) continue;
			int ind = ind_base + data_sorted[t][i].second;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		int iqr_s = GenUtils::max<double>(min_IQR, 0);
		int iqr_t = GenUtils::min<double>(max_IQR, num_obs-1);
		dc.SetPen(highlight_color);
		for (int i=iqr_s; i<=iqr_t; i++) {
			if (!hs[data_sorted[t][i].second]) continue;
			int ind = ind_base + data_sorted[t][i].second;
			MyRectangle* rec = (MyRectangle*) selectable_shps[ind];
			dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
							 rec->upper_right.x - rec->lower_left.x,
							 rec->upper_right.y - rec->lower_left.y);
		}
	}
}

/** Override of TemplateCanvas method. */
void BoxNewPlotCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering BoxNewPlotCanvas::update");
	
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	
	HighlightState::EventType type = highlight_state->GetEventType();
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
		
	Refresh();
	
	LOG_MSG("Entering BoxNewPlotCanvas::update");	
}

wxString BoxNewPlotCanvas::GetCanvasTitle()
{
	DbfGridTableBase* gb = project->GetGridBase();
	wxString s("Box Plot (Hinge=");
	if (hinge_15) s << "1.5): ";
	else s << "3.0): ";
	if (cur_first_ind == cur_last_ind) {
		s << GetNameWithTime(0);
	} else {
		s << var_info[0].name << " (";
		s << gb->GetTimeString(cur_first_ind) << "-";
		s << gb->GetTimeString(cur_last_ind) << ")";
	}
	return s;
}

wxString BoxNewPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

wxString  BoxNewPlotCanvas::GetNameWithTime(int var, int time)
{
	if (var < 0 || var >= var_info.size() || 
		time < 0 || time >= project->GetGridBase()->GetTimeSteps()) {
		return wxEmptyString;
	}
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(time);
		s << ")";
	}
	return s;
}

wxString  BoxNewPlotCanvas::GetTimeString(int var, int time)
{
	if (var < 0 || var >= var_info.size() || 
		time < 0 || time >= project->GetGridBase()->GetTimeSteps() ||
		!var_info[var].is_time_variant) {
		return wxEmptyString;
	}
	return project->GetGridBase()->GetTimeString(time);
}

void BoxNewPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering BoxNewPlotCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
		+ plot_width_const * cur_num_plots + 
		+ plot_gap_const * (cur_num_plots-1);

	shps_orig_xmin = x_min;
	shps_orig_xmax = x_max;
	shps_orig_ymin = 0;
	shps_orig_ymax = 100;
	
	MyShape* s = 0;
	int table_w=0, table_h=0;
	if (display_stats) {
		int cols = 1;
		int rows = 8;
		std::vector<wxString> vals(rows);
		vals[0] << "min";
		vals[1] << "max";
		vals[2] << "Q1";
		vals[3] << "median";
		vals[4] << "Q3";
		vals[5] << "IQR";
		vals[6] << "mean";
		vals[7] << "s.d.";
		std::vector<MyTable::CellAttrib> attribs(0); // undefined
		s = new MyTable(vals, attribs, rows, cols, *GeoDaConst::small_font,
						wxRealPoint(0, 0), MyText::h_center, MyText::top,
						MyText::right, MyText::v_center, 3, 10, -45, 30);
		background_shps.push_back(s);
		wxClientDC dc(this);
		((MyTable*) s)->GetSize(dc, table_w, table_h);
	}
	
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = display_stats ? 42+table_h : 35; // 35+130
	virtual_screen_marg_left = (display_stats || show_axes) ? 25+50 : 25;
	virtual_screen_marg_right = 25;
	
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
	
	double y_min = hinge_stats[cur_first_ind].extreme_lower_val_15;
	double y_max = hinge_stats[cur_first_ind].extreme_upper_val_15;
	int tstart = cur_first_ind;
	int tfinish = cur_last_ind;
	if (var_info[0].is_time_variant && var_info[0].sync_with_global_time
		&& var_info[0].fixed_scale) {
		tstart = 0;
		tfinish = hinge_stats.size()-1;
	}
	for (int t=tstart; t<=tfinish; t++) {
		double ext_upper = (hinge_15 ? hinge_stats[t].extreme_upper_val_15 :
							hinge_stats[t].extreme_upper_val_30);
		double ext_lower = (hinge_15 ? hinge_stats[t].extreme_lower_val_15 :
							hinge_stats[t].extreme_lower_val_30);
		if (ext_upper > y_max) y_max = ext_upper;
		if (ext_lower < y_min) y_min = ext_lower;
		if (var_info[0].max[t] > y_max) y_max = var_info[0].max[t];
		if (var_info[0].min[t] < y_min) y_min = var_info[0].min[t];
	}
	if (show_axes) {
		axis_scale = AxisScale(y_min, y_max);
		y_min = axis_scale.scale_min;
		y_max = axis_scale.scale_max;
		vert_axis = new MyAxis(var_info[0].name, axis_scale,
							   wxRealPoint(0,0), wxRealPoint(0, 100), -20, 0);
		background_shps.push_back(vert_axis);
	}
	// need to scale height data so that y_min and y_max are between 0 and 100
	double scaleY = 100.0 / (y_max-y_min);
	
	std::vector<double> orig_x_pos(cur_num_plots);
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		orig_x_pos[t-cur_first_ind] = left_pad_const + plot_width_const/2.0
		+ (t-cur_first_ind) * (plot_width_const + plot_gap_const);
	}
	
	selectable_shps.resize(num_obs * cur_num_plots);
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		double xM = orig_x_pos[t-cur_first_ind];
		double x0r = xM - plot_width_const/2.2;
		double x1r = xM + plot_width_const/2.2;
		double x0 = xM - plot_width_const/2.0;
		double y0 = (hinge_15 ? hinge_stats[t].extreme_lower_val_15 :
					 hinge_stats[t].extreme_lower_val_30);
		double x1 = xM + plot_width_const/2.0;
		double y1 = (hinge_15 ? hinge_stats[t].extreme_upper_val_15 :
					 hinge_stats[t].extreme_upper_val_30);
		
		if (display_stats) {
			// min max Q1 Q2 Q3 IQR mean sd skewness? kurtosis?
			// idea: could show a parallel real-time boxplot of
			// subset selection?  And could also show exluded as well then
			// show real-time statistics too.
			// want to show curve fitting.
			int cols = 1;
			int rows = 8;
			std::vector<wxString> vals(rows);
			vals[0] << GenUtils::DblToStr(hinge_stats[t].min_val, 4);
			vals[1] << GenUtils::DblToStr(hinge_stats[t].max_val, 4);
			vals[2] << GenUtils::DblToStr(hinge_stats[t].Q1, 4);
			vals[3] << GenUtils::DblToStr(hinge_stats[t].Q2, 4);
			vals[4] << GenUtils::DblToStr(hinge_stats[t].Q3, 4);
			vals[5] << GenUtils::DblToStr(hinge_stats[t].IQR, 4);
			vals[6] << GenUtils::DblToStr(data_stats[t].mean, 4);
			vals[7] << GenUtils::DblToStr(data_stats[t].sd_with_bessel, 4);

			std::vector<MyTable::CellAttrib> attribs(0); // undefined
			s = new MyTable(vals, attribs, rows, cols, *GeoDaConst::small_font,
							wxRealPoint(xM, 0), MyText::h_center, MyText::top,
							MyText::h_center, MyText::v_center, 3, 10, 0, 30);
			background_shps.push_back(s);
		}
		
		s = new MyPolyLine(xM-plot_width_const/3.0, (y0-y_min)*scaleY,
						   xM+plot_width_const/3.0, (y0-y_min)*scaleY);
		background_shps.push_back(s);
		s = new MyPolyLine(xM-plot_width_const/3.0, (y1-y_min)*scaleY,
						   xM+plot_width_const/3.0, (y1-y_min)*scaleY);
		background_shps.push_back(s);
		s = new MyPolyLine(orig_x_pos[t-cur_first_ind], (y0-y_min)*scaleY,
						   orig_x_pos[t-cur_first_ind], (y1-y_min)*scaleY);
		background_shps.push_back(s);
		//s = new MyRectangle(wxRealPoint(x0, (hinge_stats[t].Q1-y_min)*scaleY),
		//					wxRealPoint(x1, (hinge_stats[t].Q3-y_min)*scaleY));
		//background_shps.push_back(s);
		s = new MyCircle(wxRealPoint(xM, (data_stats[t].mean-y_min)*scaleY),
						 5.0);
		s->setPen(GeoDaConst::boxplot_point_color);
		s->setBrush(GeoDaConst::boxplot_mean_point_color);
		foreground_shps.push_back(s);
		double y0m = (hinge_stats[t].Q2-y_min)*scaleY - 0.2;
		double y1m = (hinge_stats[t].Q2-y_min)*scaleY + 0.2;
		s = new MyRectangle(wxRealPoint(x0, y0m), wxRealPoint(x1, y1m));
		s->setPen(GeoDaConst::boxplot_median_color);
		s->setBrush(GeoDaConst::boxplot_median_color);
		foreground_shps.push_back(s);
		
		wxString plot_label(var_info[0].is_time_variant ?
							GetTimeString(0, t) : GetNameWithTime(0));
		s = new MyText(plot_label, *GeoDaConst::small_font,
					   wxRealPoint(orig_x_pos[t-cur_first_ind], 0), 0,
					   MyText::h_center, MyText::v_center, 0, 18);
		background_shps.push_back(s);
		
		for (int i=0; i<hinge_stats[t].min_IQR_ind; i++) {
			double val = data_sorted[t][i].first;
			int ind = (t-cur_first_ind)*num_obs + data_sorted[t][i].second;
			//LOG_MSG(wxString::Format("data_sorted[%d][%d].first = %f", t, i,
			//						 data_sorted[t][i].first));
			selectable_shps[ind] =
				new MyPoint(orig_x_pos[t-cur_first_ind], (val-y_min) * scaleY);
			selectable_shps[ind]->setPen(GeoDaConst::boxplot_point_color);
			selectable_shps[ind]->setBrush(*wxWHITE_BRUSH);
		}
		for (int i=hinge_stats[t].max_IQR_ind+1; i<num_obs; i++) {
			double val = data_sorted[t][i].first;
			int ind = (t-cur_first_ind)*num_obs + data_sorted[t][i].second;
			//if (i<hinge_stats[t].max_IQR_ind+10) {
			//	LOG_MSG(wxString::Format("data_sorted[%d][%d].first = %f", t, i,
			//						 data_sorted[t][i].first));
			//	selectable_shps[ind] =
			//		new MyPoint(orig_x_pos[t-cur_first_ind] + 5,
			//					(val-y_min)*scaleY);
			//} else {
			selectable_shps[ind] =
				new MyPoint(orig_x_pos[t-cur_first_ind], (val-y_min) * scaleY);
			selectable_shps[ind]->setPen(GeoDaConst::boxplot_point_color);
			selectable_shps[ind]->setBrush(*wxWHITE_BRUSH);
			//}
		}
		if (hinge_stats[t].min_IQR_ind == hinge_stats[t].max_IQR_ind) {
			int ind = ((t-cur_first_ind)*num_obs +
					   data_sorted[t][hinge_stats[t].min_IQR_ind].second);
			double y0 = (hinge_stats[t].Q1 - y_min) * scaleY;
			double y1 = (hinge_stats[t].Q3 - y_min) * scaleY;
			selectable_shps[ind] = new MyRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			selectable_shps[ind]->setPen(GeoDaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GeoDaConst::boxplot_q1q2q3_color);
		} else {
			int minIQR = hinge_stats[t].min_IQR_ind;
			int maxIQR = hinge_stats[t].max_IQR_ind;

			int ind = (t-cur_first_ind)*num_obs + data_sorted[t][minIQR].second;
			double y0 = (hinge_stats[t].Q1 - y_min) * scaleY;
			double y1;
			if (minIQR > -1) {
				y1 = (((data_sorted[t][minIQR].first +
						data_sorted[t][minIQR+1].first)/2.0) - y_min)*scaleY;
			} else {
				y1 = (data_sorted[t][minIQR+1].first - y_min)*scaleY;
			}
			selectable_shps[ind] = new MyRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			//selectable_shps[ind]->pen = *wxGREEN_PEN;
			//selectable_shps[ind]->brush = *wxGREEN_BRUSH;
			selectable_shps[ind]->setPen(GeoDaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GeoDaConst::boxplot_q1q2q3_color);
			ind = (t-cur_first_ind)*num_obs + data_sorted[t][maxIQR].second;
			if (maxIQR < num_obs) {
				y0 = (((data_sorted[t][maxIQR].first +
						data_sorted[t][maxIQR-1].first)/2.0) - y_min)*scaleY;
			} else {
				y0 = (data_sorted[t][maxIQR-1].first - y_min)*scaleY;
			}
			y1 = (hinge_stats[t].Q3 - y_min) * scaleY;
			selectable_shps[ind] = new MyRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			//selectable_shps[ind]->pen = *wxCYAN_PEN;
			//selectable_shps[ind]->brush = *wxCYAN_BRUSH;
			selectable_shps[ind]->setPen(GeoDaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GeoDaConst::boxplot_q1q2q3_color);
			//LOG(data_sorted[t][maxIQR].first);
			//LOG(data_sorted[t][maxIQR-1].first);
			//LOG(hinge_stats[t].Q3);
			//LOG(maxIQR);
			
			for (int i=minIQR+1; i<maxIQR; i++) {
				ind = (t-cur_first_ind)*num_obs + data_sorted[t][i].second;
				y0 = (((data_sorted[t][i].first +
						data_sorted[t][i-1].first)/2.0) - y_min)*scaleY;
				y1 = (((data_sorted[t][i].first +
						data_sorted[t][i+1].first)/2.0) - y_min)*scaleY;
				selectable_shps[ind] = new MyRectangle(wxRealPoint(x0r, y0),
													   wxRealPoint(x1r, y1));
				selectable_shps[ind]->setPen(GeoDaConst::boxplot_q1q2q3_color);
				selectable_shps[ind]->setBrush(GeoDaConst::boxplot_q1q2q3_color);
				//if (ind % 2 == 0) {
				//	selectable_shps[ind]->pen = *wxBLUE_PEN;
				//	selectable_shps[ind]->brush = *wxBLUE_BRUSH;
				//} else {
				//	selectable_shps[ind]->pen = *wxRED_PEN;
				//	selectable_shps[ind]->brush = *wxRED_BRUSH;
				//}
			}
		}
	}
	
	ResizeSelectableShps();

	LOG_MSG("Exiting BoxNewPlotCanvas::PopulateCanvas");
}

void BoxNewPlotCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering BoxNewPlotCanvas::TitleOrTimeChange");
	if (!is_any_sync_with_global_time) return;
	
	var_info[0].time = project->GetGridBase()->curr_time_step;
	
	int time_steps = project->GetGridBase()->GetTimeSteps();
	int start = var_info[0].time - cur_num_plots/2;
	if (cur_num_plots % 2 == 0) start++;
	start = GenUtils::max(start, 0);
	start = GenUtils::min(start, time_steps-cur_num_plots);
	
	if (cur_first_ind == start) return;
	
	cur_first_ind = start;
	cur_last_ind = cur_first_ind + cur_num_plots - 1;

	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting BoxNewPlotCanvas::TitleOrTimeChange");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void BoxNewPlotCanvas::VarInfoAttributeChange()
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

void BoxNewPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In BoxNewPlotCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	PopulateCanvas();
}

void BoxNewPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In BoxNewPlotCanvas::FixedScaleVariableToggle");
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::PlotsPerView(int plots_per_view)
{
	if (plots_per_view == cur_num_plots) return;
	cur_num_plots = plots_per_view;
	int time_steps = project->GetGridBase()->GetTimeSteps();
	int start = var_info[0].time - cur_num_plots/2;
	if (cur_num_plots % 2 == 0) start++;
	start = GenUtils::max(start, 0);
	start = GenUtils::min(start, time_steps-cur_num_plots);
	cur_first_ind = start;
	cur_last_ind = cur_first_ind + cur_num_plots - 1;
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::PlotsPerViewOther()
{
	// ask user for custom number of plots.  This dialog only appears
	// when GetTimeSteps() > 10
	wxString title("Plots Per View"); 
	MapQuantileDlg dlg(this, 1, max_plots, 11, title, "Plots Per View");
	dlg.SetTitle(title);
	if (dlg.ShowModal() != wxID_OK) return;
	else PlotsPerView(dlg.classes);
}

void BoxNewPlotCanvas::PlotsPerViewAll()
{
	PlotsPerView(max_plots);
}

void BoxNewPlotCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::ShowAxes(bool show_axes_s)
{
	if (show_axes == show_axes_s) return;
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::Hinge15()
{
	if (hinge_15) return;
	hinge_15 = true;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::Hinge30()
{
	if (!hinge_15) return;
	hinge_15 = false;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxNewPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "obs " << hover_obs[0]+1;
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1;
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1;
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	} else if (mousemode == select &&
			   (selectstate == dragging || selectstate == brushing)) {
		s << "#selected=" << highlight_state->GetTotalHighlighted();
	}
	sb->SetStatusText(s);
}

IMPLEMENT_CLASS(BoxNewPlotFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(BoxNewPlotFrame, TemplateFrame)
	EVT_ACTIVATE(BoxNewPlotFrame::OnActivate)
END_EVENT_TABLE()

BoxNewPlotFrame::BoxNewPlotFrame(wxFrame *parent, Project* project,
								 const std::vector<GeoDaVarInfo>& var_info,
								 const std::vector<int>& col_ids,
								 const wxString& title,
								 const wxPoint& pos,
								 const wxSize& size,
								 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering BoxNewPlotFrame::BoxNewPlotFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new BoxNewPlotCanvas(this, this, project,
										   var_info, col_ids,
										   wxDefaultPosition,
										   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
		
	Show(true);
	LOG_MSG("Exiting BoxNewPlotFrame::BoxNewPlotFrame");
}

BoxNewPlotFrame::~BoxNewPlotFrame()
{
	LOG_MSG("In BoxNewPlotFrame::~BoxNewPlotFrame");
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void BoxNewPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In BoxNewPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("BoxNewPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void BoxNewPlotFrame::MapMenus()
{
	LOG_MSG("In BoxNewPlotFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_BOX_NEW_PLOT_VIEW_MENU_OPTIONS");
	((BoxNewPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((BoxNewPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void BoxNewPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("BoxNewPlotFrame::UpdateOptionMenuItems: Options "
				"menu not found");
	} else {
		((BoxNewPlotCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void BoxNewPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void BoxNewPlotFrame::update(FramesManager* o)
{
	LOG_MSG("In BoxNewPlotFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
}

void BoxNewPlotFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void BoxNewPlotFrame::OnShowAxes(wxCommandEvent& event)
{
	LOG_MSG("In BoxNewPlotFrame::OnShowAxes");
	BoxNewPlotCanvas* t = (BoxNewPlotCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void BoxNewPlotFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	LOG_MSG("In BoxNewPlotFrame::OnDisplayStatistics");
	BoxNewPlotCanvas* t = (BoxNewPlotCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void BoxNewPlotFrame::OnHinge15(wxCommandEvent& event)
{
	BoxNewPlotCanvas* t = (BoxNewPlotCanvas*) template_canvas;
	t->Hinge15();
	UpdateOptionMenuItems();
	UpdateTitle();
}

void BoxNewPlotFrame::OnHinge30(wxCommandEvent& event)
{
	BoxNewPlotCanvas* t = (BoxNewPlotCanvas*) template_canvas;
	t->Hinge30();
	UpdateOptionMenuItems();
	UpdateTitle();
}
