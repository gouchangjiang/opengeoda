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
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../DialogTools/MapQuantileDlg.h"
#include "../DialogTools/SelectWeightDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../FramesManager.h"
#include "../logger.h"
#include "../OpenGeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../ShapeOperations/WeightsManager.h"
#include "MapNewView.h"

IMPLEMENT_CLASS(MapNewCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(MapNewCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	//EVT_KEY_DOWN(TemplateCanvas::OnKeyDown)
END_EVENT_TABLE()

MapNewCanvas::MapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
						   Project* project_s,
						   ThemeUtilities::ThemeType theme_type_s,
						   SmoothingType smoothing_type_s,
						   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, true, true),
project(project_s), num_obs(project_s->GetNumRecords()),
num_time_vals(1),
highlight_state(project_s->highlight_state),
data(0), var_info(0),
grid_base(project_s->GetGridBase()),
theme_type(ThemeUtilities::no_theme), smoothing_type(no_smoothing),
gal_weight(0),
is_rate_smoother(false), full_map_redraw_needed(true),
display_mean_centers(false), display_centroids(false)
{
	using namespace Shapefile;
	LOG_MSG("Entering MapNewCanvas::MapNewCanvas");
	template_frame = t_frame;
	
	selectable_fill_color =
		GeoDaConst::map_default_fill_colour;
	
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25;
	virtual_screen_marg_left = 25;
	virtual_screen_marg_right = 25;	
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
		
	double scale_x, scale_y, trans_x, trans_y;
	MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
		shps_orig_xmax, shps_orig_ymax,
		virtual_screen_marg_top, virtual_screen_marg_bottom,
		virtual_screen_marg_left, virtual_screen_marg_right,
		GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(),
		fixed_aspect_ratio_mode, fit_to_window_mode,
		&scale_x, &scale_y, &trans_x, &trans_y, 0, 0,
		&current_shps_width, &current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;

	if (project->main_data.header.shape_type == Shapefile::POINT) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GeoDaConst::map_default_highlight_colour;
	}
	
	use_category_brushes = true;
	if (!ChangeMapType(theme_type_s, smoothing_type_s)) {
		// The user possibly clicked cancel.  Try again with
		// themeless map
		ChangeMapType(ThemeUtilities::no_theme, no_smoothing);
	}
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting MapNewCanvas::MapNewCanvas");
}

MapNewCanvas::~MapNewCanvas()
{
	LOG_MSG("Entering MapNewCanvas::~MapNewCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting MapNewCanvas::~MapNewCanvas");
}

void MapNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering MapNewCanvas::DisplayRightClickMenu");
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting MapNewCanvas::DisplayRightClickMenu");
}

void MapNewCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
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

	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}


void MapNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
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
								  theme_type == ThemeUtilities::equal_intervals);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
								  theme_type == ThemeUtilities::natural_breaks);
	
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_RAWRATE"),
								  smoothing_type == raw_rate);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
								  smoothing_type == excess_risk);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
								  smoothing_type == empirical_bayes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
								  smoothing_type == spatial_rate);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
								  smoothing_type == spatial_empirical_bayes);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  selectable_shps_type != points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  selectable_shps_type != points);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  display_mean_centers);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  display_centroids);
}

wxString MapNewCanvas::GetCanvasTitle()
{
	wxString v;
	if (var_info.size() == 1) v << GetNameWithTime(0);
	if (var_info.size() == 2) {
		if (smoothing_type == raw_rate) {
			v << "Raw Rate ";
		} else if (smoothing_type == excess_risk) {
			// Excess Risk smoothing is a special case that comes with
			// its own theme.  See below.
			v << "";
		} else if (smoothing_type == empirical_bayes) {
			v << "EBS-Smoothed ";
		} else if (smoothing_type == spatial_rate) {
			v << "SRS-Smoothed ";
		} else if (smoothing_type == spatial_empirical_bayes) {
			v << "SEBS-Smoothed ";
		}
		v << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	}
	
	wxString s;
	if (theme_type == ThemeUtilities::excess_risk_theme) {
		// Excess Risk smoothing map is a special case in that it is a
		// type of smoothing, but is also its own theme.  Any theme associated
		// with Excess Risk is ignored
		s << "Excess Risk Map: " << v;
	}
	else if (theme_type == ThemeUtilities::no_theme) {
		s << "Map";
	} else {
		s << ThemeUtilities::ThemeTypeToString(theme_type) << ": " << v;
	}
	
	return s;
}

wxString MapNewCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;	
}

void MapNewCanvas::OnSaveCategories()
{
	wxString t_name = ThemeUtilities::ThemeTypeToString(theme_type);
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
	SaveCategories(title, label, "CATEGORIES");	
}

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification including any needed data smoothing. */
bool MapNewCanvas::ChangeMapType(ThemeUtilities::ThemeType new_map_theme,
								 SmoothingType new_map_smoothing)
{
	// We only ask for variables when changing from no_theme themed or
	// smoothed (with theme).
	
	if (new_map_smoothing == excess_risk) {
		new_map_theme = ThemeUtilities::excess_risk_theme;
	}
	
	int new_num_vars;
	if (new_map_smoothing != no_smoothing) {
		new_num_vars = 2;
	} else if (new_map_theme != ThemeUtilities::no_theme) {
		new_num_vars = 1;
	} else {
		new_num_vars = 0;
	}
	
	int num_vars = var_info.size();
	
	if (new_num_vars == 0) {
		var_info.clear();
	} else if (new_num_vars == 1) {
		if (num_vars == 0) {
			VariableSettingsDlg dlg(project, VariableSettingsDlg::univariate,
									false);
			if (dlg.ShowModal() != wxID_OK) return false;
			var_info.resize(1);
			data.resize(1);
			for (int i=0; i<1; i++) {
				var_info[i] = dlg.var_info[i];
				grid_base->GetColData(dlg.col_ids[i], data[i]);
			}
		} else if (num_vars == 1) {
			// reuse current variable settings and values
		} else { // num_vars == 2
			// reuse first variable setting and value
			var_info.resize(1);
			data.resize(1);
		}
	} else if (new_num_vars == 2) {
		// user needs to choose new map themes
		
		if (new_map_smoothing == spatial_rate ||
			new_map_smoothing == spatial_empirical_bayes) {
			// check for weights file
			if (!project->GetWManager()->IsDefaultWeight()) {
				SelectWeightDlg dlg(project, this);
				if (dlg.ShowModal()!= wxID_OK) return false;
			}
			int w_ind = project->GetWManager()->GetCurrWeightInd();
			gal_weight = project->GetWManager()->GetGalWeight(w_ind);
		}
		
		wxString title;
		if (new_map_smoothing == raw_rate) {
			title = "Raw Rate Smoothed Variable Settings";
		} else if (new_map_smoothing == excess_risk) {
			title = "Excess Risk Map Variable Settings";
		} else if (new_map_smoothing == empirical_bayes) {
			title = "Empirical Bayes Smoothed Variable Settings";
		} else if (new_map_smoothing == spatial_rate) {
			title = "Spatial Rate Smoothed Variable Settings";
		} else if (new_map_smoothing == spatial_empirical_bayes) {
			title = "Empirical Spatial Rate Smoothed Variable Settings";
		}

		VariableSettingsDlg::VarType v_type=VariableSettingsDlg::rate_smoothed; 
		if (new_map_smoothing == excess_risk) {
			v_type = VariableSettingsDlg::bivariate;
		}
		VariableSettingsDlg dlg(project, v_type, false, title,
								"Event Variable", "Base Variable");
		if (dlg.ShowModal() != wxID_OK) return false;
		var_info.clear();
		data.clear();
		var_info.resize(2);
		data.resize(2);
		for (int i=0; i<2; i++) {
			var_info[i] = dlg.var_info[i];
			grid_base->GetColData(dlg.col_ids[i], data[i]);
		}
		if (new_map_smoothing == excess_risk) {
			new_map_theme = ThemeUtilities::excess_risk_theme;
		} else if (dlg.m_theme == 0) {
			new_map_theme = ThemeUtilities::quantile;
		} else if (dlg.m_theme == 1) {
			new_map_theme = ThemeUtilities::percentile;
		} else if (dlg.m_theme == 2) {
			new_map_theme = ThemeUtilities::hinge_15;
		} else if (dlg.m_theme == 3) {
			new_map_theme = ThemeUtilities::hinge_30;
		} else if (dlg.m_theme == 4) {
			new_map_theme = ThemeUtilities::stddev;
		} else if (dlg.m_theme == 5) {
			new_map_theme = ThemeUtilities::natural_breaks;
		} else if (dlg.m_theme == 6) {
			new_map_theme = ThemeUtilities::equal_intervals;
		}
	}
	
	theme_type = new_map_theme;
	smoothing_type = new_map_smoothing;
	VarInfoAttributeChange();	
	CreateAndUpdateCategories();
	PopulateCanvas();
	return true;
}


/** This method assumes that v1 is already set and valid.  It will
 recreate all canvas objects as needed and refresh the canvas.
 Assumes that CreateAndUpdateCategories has already been called.
 All data analysis will have been done in CreateAndUpdateCategories
 already. */
void MapNewCanvas::PopulateCanvas()
{
	LOG_MSG("Entering MapNewCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();

	int canvas_ts = GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the map needs
	// to be resized.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {		
		if (full_map_redraw_needed) {
			CreateSelShpsFromProj(selectable_shps, project);
			full_map_redraw_needed = false;
			
			if (selectable_shps_type == polygons &&
				(display_mean_centers || display_centroids)) {
				MyPoint* p;
				wxPen cent_pen(wxColour(20, 20, 20));
				wxPen cntr_pen(wxColour(55, 55, 55));
				if (display_mean_centers) {
					const std::vector<MyPoint*>& c = project->GetMeanCenters();
					for (int i=0; i<num_obs; i++) {
						p = new MyPoint(*c[i]);
						p->setPen(cntr_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
				if (display_centroids) {
					const std::vector<MyPoint*>& c = project->GetCentroids();
					for (int i=0; i<num_obs; i++) {
						p = new MyPoint(*c[i]);
						p->setPen(cent_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
			}
		}
	} else {
		wxRealPoint cntr_ref_pnt(shps_orig_xmin +
								 (shps_orig_xmax-shps_orig_xmin)/2.0,
								 shps_orig_ymin+ 
								 (shps_orig_ymax-shps_orig_ymin)/2.0);
		MyText* txt_shp = new MyText(map_error_message[canvas_ts],
									 *GeoDaConst::medium_font, cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting MapNewCanvas::PopulateCanvas");
}

void MapNewCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering MapNewCanvas::TitleOrTimeChange");
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
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting MapNewCanvas::TitleOrTimeChange");
}

void MapNewCanvas::VarInfoAttributeChange()
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

/** Update Categories based on num_time_vals, num_cats and ref_var_index.
 This method populates cat_var_sorted from data array and performs any
 smoothing as needed, setting smoothing_valid vector as appropriate. */
void MapNewCanvas::CreateAndUpdateCategories()
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_valid[t] = true;
	map_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_error_message[t] = wxEmptyString;
	
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
	// We assume data has been initialized to correct data
	// for all time periods.
	
	double* P = 0;
	double* E = 0;
	double* smoothed_results = 0;
	std::vector<bool> undef_res(smoothing_type == no_smoothing ? 0 : num_obs);
	if (smoothing_type != no_smoothing) {
		P = new double[num_obs];
		E = new double[num_obs];
		smoothed_results = new double[num_obs];
	}
	
	cat_var_sorted.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
		
		if (smoothing_type != no_smoothing) {
			if (var_info[0].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][t+var_info[0].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][var_info[0].time][i];
				}
			}
			
			if (var_info[1].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][t+var_info[1].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][var_info[1].time][i];
				}
			}
			
			for (int i=0; i<num_obs; i++) {
				if (P[i] <= 0) {
					map_valid[t] = false;
					map_error_message[t] = "Error: Base values contain "
						" non-positive numbers. Cannot perform smoothing.";
					continue;
				}
			}
			
			if (!map_valid[t]) continue;
			
			if (smoothing_type == raw_rate) {
				GeoDaAlgs::RateSmoother_RawRate(num_obs, P, E,
												smoothed_results,
												undef_res);
			} else if (smoothing_type == excess_risk) {
				// Note: Excess Risk is a transformation, not a smoothing
				GeoDaAlgs::RateSmoother_ExcessRisk(num_obs, P, E,
												   smoothed_results,
												   undef_res);
			} else if (smoothing_type == empirical_bayes) {
				GeoDaAlgs::RateSmoother_EBS(num_obs, P, E,
											smoothed_results, undef_res);
			} else if (smoothing_type == spatial_rate) {
				GeoDaAlgs::RateSmoother_SRS(num_obs, gal_weight->gal, P, E,
											smoothed_results, undef_res);
			} else if (smoothing_type == spatial_empirical_bayes) {
				GeoDaAlgs::RateSmoother_SEBS(num_obs, gal_weight->gal, P, E,
											 smoothed_results, undef_res);
			}
		
			for (int i=0; i<num_obs; i++) {
				cat_var_sorted[t][i].first = smoothed_results[i];
				cat_var_sorted[t][i].second = i;
			}
		} else {
			for (int i=0; i<num_obs; i++) {
				cat_var_sorted[t][i].first =data[0][t+var_info[0].time_min][i];
				cat_var_sorted[t][i].second = i;
			}
		}
	}
	
	if (smoothing_type != no_smoothing) {
		if (P) delete [] P;
		if (E) delete [] E;
		if (smoothed_results) delete [] smoothed_results;
	}

	// Sort each vector in ascending order
	for (int t=0; t<num_time_vals; t++) {
		if (map_valid[t]) { // only sort data with valid smoothing
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
			title = "Quantile Map";
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
									   var_info[0], cat_var_sorted,
									   this, map_valid,
									   map_error_message);
	if (ref_var_index != -1) {
		SetCurrentCanvasTmStep(var_info[ref_var_index].time
							   - var_info[ref_var_index].time_min);
	}
}

void MapNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In MapNewCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	// Strictly speaking, should not have to repopulate map canvas
	// when time sync changes since scale of objects never changes. To keep
	// things simple, just redraw the whole canvas.
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void MapNewCanvas::DisplayMeanCenters()
{
	full_map_redraw_needed = true;
	display_mean_centers = !display_mean_centers;
	PopulateCanvas();
}

void MapNewCanvas::DisplayCentroids()
{
	full_map_redraw_needed = true;
	display_centroids = !display_centroids;
	PopulateCanvas();
}

/** Save Rates option should only be available when 
 smoothing_type != no_smoothing */
void MapNewCanvas::SaveRates()
{
	if (smoothing_type == no_smoothing) {
		wxString msg;
		msg << "No rates currently calculated to save.";
		wxMessageDialog dlg (this, msg, "Information",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	
	std::vector<SaveToTableEntry> data(1);
	
	std::vector<double> dt(num_obs);
	int t = GetCurrentCanvasTmStep();
    for (int i=0; i<num_obs; i++) {
		dt[cat_var_sorted[t][i].second] = cat_var_sorted[t][i].first;
	}
	data[0].type = GeoDaConst::double_type;
	data[0].d_val = &dt;
	data[0].label = "Rate";
	
	if (smoothing_type == raw_rate) {
		data[0].field_default = "R_RAWRATE";
	} else if (smoothing_type == excess_risk) {
		data[0].field_default = "R_EXCESS";
	} else if (smoothing_type == empirical_bayes) {
		data[0].field_default = "R_EBS";
	} else if (smoothing_type == spatial_rate) {
		data[0].field_default = "R_SPATRATE";
	} else if (smoothing_type == spatial_empirical_bayes) {
		data[0].field_default = "R_SPATEBS";
	} else {
		return;
	}

	wxString title = "Save Rates - ";
	title << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	
    SaveToTableDlg dlg(grid_base, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
    if (dlg.ShowModal() == wxID_OK);
}


void MapNewCanvas::UpdateStatusBar()
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
		//if (brushtype == rectangle) {
			//wxRealPoint pt1 = MousePntToObsPnt(sel1);
			//wxRealPoint pt2 = MousePntToObsPnt(sel2);
			//wxString xmin = GenUtils::DblToStr(GenUtils::min<double>(pt1.x,
			//														 pt2.x));
			//wxString xmax = GenUtils::DblToStr(GenUtils::max<double>(pt1.x,
			//														 pt2.x));
			//wxString ymin = GenUtils::DblToStr(GenUtils::min<double>(pt1.y,
			//														 pt2.y));
			//wxString ymax = GenUtils::DblToStr(GenUtils::max<double>(pt1.y,
			//														 pt2.y));
			//s << ", select rect:";
			//s << " [" << xmin << "," << xmax << "] and";
			//s << " [" << ymin << "," << ymax << "]";
		//}
	}
	sb->SetStatusText(s);
}

MapNewLegend::MapNewLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

MapNewLegend::~MapNewLegend()
{
    LOG_MSG("In MapNewLegend::~MapNewLegend");
}

IMPLEMENT_CLASS(MapNewFrame, TemplateFrame)
BEGIN_EVENT_TABLE(MapNewFrame, TemplateFrame)
	EVT_ACTIVATE(MapNewFrame::OnActivate)	
END_EVENT_TABLE()

MapNewFrame::MapNewFrame(wxFrame *parent, Project* project,
						 ThemeUtilities::ThemeType theme_type,
						 MapNewCanvas::SmoothingType smoothing_type,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
: TemplateFrame(parent, project, "Map", pos, size, style)
{
	LOG_MSG("Entering MapNewFrame::MapNewFrame");

	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	template_canvas = new MapNewCanvas(splitter_win, this, project,
									   theme_type, smoothing_type,
									   wxDefaultPosition,
									   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	
	template_legend = new MapNewLegend(splitter_win, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								  GeoDaConst::map_default_legend_width);
	
	Show(true);
	LOG_MSG("Exiting MapNewFrame::MapNewFrame");
}

MapNewFrame::MapNewFrame(wxFrame *parent, Project* project,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
: TemplateFrame(parent, project, "Map", pos, size, style)
{
	LOG_MSG("Entering MapNewFrame::MapNewFrame");
	LOG_MSG("Exiting MapNewFrame::MapNewFrame");
}

MapNewFrame::~MapNewFrame()
{
	LOG_MSG("In MapNewFrame::~MapNewFrame");
	DeregisterAsActive();
}

void MapNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In MapNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("MapNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void MapNewFrame::MapMenus()
{
	LOG_MSG("In MapNewFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	((MapNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void MapNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MapNewFrame::UpdateOptionMenuItems: Options menu not found");
	} else {
		((MapNewCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void MapNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void  MapNewFrame::update(FramesManager* o)
{
	LOG_MSG("In MapNewFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void MapNewFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void MapNewFrame::OnThemelessMap(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::no_theme, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::hinge_15, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::hinge_30, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::quantile, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::percentile, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::stddev, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::unique_values, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::natural_breaks, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::equal_intervals, MapNewCanvas::no_smoothing);
}

void MapNewFrame::OnSaveCategories(wxCommandEvent& event)
{
	((MapNewCanvas*) template_canvas)->OnSaveCategories();
}

void MapNewFrame::OnRawrate(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::no_theme, MapNewCanvas::raw_rate);
}

void MapNewFrame::OnExcessRisk(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::excess_risk_theme, MapNewCanvas::excess_risk);
}

void MapNewFrame::OnEmpiricalBayes(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::no_theme, MapNewCanvas::empirical_bayes);
}

void MapNewFrame::OnSpatialRate(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::no_theme, MapNewCanvas::spatial_rate);
}

void MapNewFrame::OnSpatialEmpiricalBayes(wxCommandEvent& event)
{
	ChangeMapType(ThemeUtilities::no_theme,
				  MapNewCanvas::spatial_empirical_bayes);
}

void MapNewFrame::OnSaveRates(wxCommandEvent& event)
{
	((MapNewCanvas*) template_canvas)->SaveRates();
}

bool MapNewFrame::ChangeMapType(ThemeUtilities::ThemeType new_map_theme,
								MapNewCanvas::SmoothingType new_map_smoothing)
{
	bool r=((MapNewCanvas*) template_canvas)->ChangeMapType(new_map_theme,
															new_map_smoothing);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Refresh();
	return r;
}

void MapNewFrame::OnDisplayMeanCenters()
{
	((MapNewCanvas*) template_canvas)->DisplayMeanCenters();
	UpdateOptionMenuItems();
}

void MapNewFrame::OnDisplayCentroids()
{
	((MapNewCanvas*) template_canvas)->DisplayCentroids();
	UpdateOptionMenuItems();
}
