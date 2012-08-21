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

#include <algorithm>
#include <cmath>
#include <vector>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <boost/foreach.hpp>
#include <boost/multi_array.hpp>
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../OpenGeoDa.h"
#include "../logger.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "LisaCoordinator.h"
#include "LisaScatterPlotView.h"

IMPLEMENT_CLASS(LisaScatterPlotCanvas, ScatterNewPlotCanvas)
BEGIN_EVENT_TABLE(LisaScatterPlotCanvas, ScatterNewPlotCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

LisaScatterPlotCanvas::LisaScatterPlotCanvas(wxWindow *parent,
											 TemplateFrame* t_frame,
											 Project* project,
											 LisaCoordinator* lisa_coordinator,
											 const wxPoint& pos,
											 const wxSize& size)
: ScatterNewPlotCanvas(parent, t_frame, project, pos, size),
lisa_coord(lisa_coordinator),
is_bi(lisa_coordinator->lisa_type == LisaCoordinator::bivariate),
is_rate(lisa_coordinator->lisa_type == LisaCoordinator::eb_rate_standardized)
{
	LOG_MSG("Entering LisaScatterPlotCanvas::LisaMapNewCanvas");
	
	fixed_aspect_ratio_mode = true;
	// must set var_info from LisaCoordinator initially in order to get
	// intial times for each variable.
	sp_var_info.resize(2);
	var_info = lisa_coord->var_info;
	var_info_orig = var_info;
	SyncVarInfoFromCoordinator();
	
	CreateCategoriesAllCanvasTms(1, 1);
	SetCategoryColor(0, 0, 
					 GeoDaConst::scatterplot_regression_excluded_color);
	for (int i=0; i<num_obs; i++) AppendIdToCategory(0, 0, i);
	// For LisaScatterPlot, all time steps have the exact same
	// trivial categorization.
	SetCurrentCanvasTmStep(0);
	
	//CreateCategoriesAllCanvasTms(1, num_time_vals); // 1 = #cats
	//for (int t=0; t<num_time_vals; t++) {
	//	SetCategoryColor(t, 0, 
	//					 GeoDaConst::scatterplot_regression_excluded_color);
	//	for (int i=0; i<num_obs; i++) {
	//		AppendIdToCategory(t, 0, i);
	//	}
	//}
	// For LisaScatterPlot, all time steps have the exact same
	// trivial categorization.
	//SetCurrentCanvasTmStep(0);
	//if (ref_var_index != -1) {
	//	SetCurrentCanvasTmStep(var_info[ref_var_index].time
	//						   - var_info[ref_var_index].time_min);
	//}
	
	//CreateAndUpdateCategories();
	PopulateCanvas();
	
	LOG_MSG("Exiting LisaScatterPlotCanvas::LisaMapNewCanvas");
}

LisaScatterPlotCanvas::~LisaScatterPlotCanvas()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::~LisaScatterPlotCanvas");
	LOG_MSG("Exiting LisaScatterPlotCanvas::~LisaScatterPlotCanvas");
}

void LisaScatterPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering LisaScatterPlotCanvas::DisplayRightClickMenu");
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISA_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting LisaScatterPlotCanvas::DisplayRightClickMenu");
}

void LisaScatterPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant && (i==0 || is_bi || is_rate)) {
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
    */
	 
	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

wxString LisaScatterPlotCanvas::GetCanvasTitle()
{
	wxString s;
	wxString v0(var_info_orig[0].name);
	if (var_info_orig[0].is_time_variant) {
		v0 << " (" << project->GetGridBase()->
			GetTimeString(var_info_orig[0].time);
		v0 << ")";
	}
	wxString v1;
	if (is_bi || is_rate) {
		v1 << var_info_orig[1].name;
		if (var_info_orig[1].is_time_variant) {
			v1 << " (" << project->GetGridBase()->
				GetTimeString(var_info_orig[1].time);
			v1 << ")";
		}
	}	
	wxString w(lisa_coord->weight_name);
	if (is_bi) {
		s << "Bivariate Moran's I (" << w << "): ";
		s << v0 << " and lagged " << v1;
	} else if (is_rate) {
		s << "Emp Bayes Rate Std Moran's I (" << w << "): ";
		s << v0 << " / " << v1;
	} else {
		s << "Moran's I (" << w << "): " << v0;
	}
	return s;
}


/** This virtual function will be called by the Scatter Plot base class
 to determine x and y axis labels. */
wxString LisaScatterPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	// Different behaviour depending on whether univariate, bivariate or
	// rate-adjusted.
		
	wxString v0(var_info_orig[0].name);
	if (var_info_orig[0].is_time_variant) {
		v0 << " (" << project->GetGridBase()->
			GetTimeString(var_info_orig[0].time);
		v0 << ")";
	}
	wxString v1;
	if (is_bi || is_rate) {
		v1 << var_info_orig[1].name;
		if (var_info_orig[1].is_time_variant) {
			v1 << " (" << project->GetGridBase()->
				GetTimeString(var_info_orig[1].time);
			v1 << ")";
		}
	}
	wxString s0;
	wxString s1;
	if (!is_rate) {
		s0 << v0;
	} else {
		s0 << v0 << " / " << v1;
	}
	if (is_bi) {
		s1 << v1;
	} else if (is_rate) {
		s1 << v0 << " / " << v1;
	} else {
		s1 << v0;
	}
	
	if (var == 1) return "lagged " + s1;
	return s0;
}

void LisaScatterPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ScatterNewPlotCanvas::SetCheckMarks(menu);
}

void LisaScatterPlotCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::TitleOrTimeChange");
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
	var_info_orig = var_info;
	sp_var_info[0].time = var_info[ref_var_index].time;
	sp_var_info[1].time = var_info[ref_var_index].time;
	//SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting LisaScatterPlotCanvas::TitleOrTimeChange");
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void LisaScatterPlotCanvas::SyncVarInfoFromCoordinator()
{
	using namespace boost;
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = lisa_coord->var_info;
	for (int t=0; t<var_info.size(); t++) var_info[t].time = my_times[t];
	is_any_time_variant = lisa_coord->is_any_time_variant;
	is_any_sync_with_global_time = lisa_coord->is_any_sync_with_global_time;
	ref_var_index = lisa_coord->ref_var_index;
	num_time_vals = lisa_coord->num_time_vals;
	
	//Now, we need to also update sp_var_info appropriately.
	// both sp_var_info objects will have the same range of time values.
	// we should base them off of the reference time value, or else
	// just variable 1.
	int t_ind = (ref_var_index == -1) ? 0 : ref_var_index;
	sp_var_info[0] = var_info[t_ind];
	sp_var_info[1] = var_info[t_ind];
	
	x_data.resize(extents[lisa_coord->num_time_vals][lisa_coord->num_obs]);
	y_data.resize(extents[lisa_coord->num_time_vals][lisa_coord->num_obs]);
	for (int t=0; t<lisa_coord->num_time_vals; t++) {
		double x_min = lisa_coord->data1_vecs[t][0];
		double x_max = x_min;
		double y_min = lisa_coord->lags_vecs[t][0];
		double y_max = y_min;
		for (int i=0; i<lisa_coord->num_obs; i++) {
			//LOG_MSG(wxString::Format("data1_vecs[%d][%d] = %f", t, i,
			//						 lisa_coord->data1_vecs[t][i]));
			x_data[t][i] = lisa_coord->data1_vecs[t][i];
			y_data[t][i] = lisa_coord->lags_vecs[t][i];
			if (x_data[t][i] < x_min) {
				x_min = x_data[t][i];
			} else if (x_data[t][i] > x_max) {
				x_max = x_data[t][i];
			}
			if (y_data[t][i] < y_min) {
				y_min = y_data[t][i];
			} else if (y_data[t][i] > y_max) {
				y_max = y_data[t][i];
			}
		}
		double mag = std::max(std::max(fabs(x_min), fabs(x_max)),
							  std::max(fabs(y_min), fabs(y_max)));
		sp_var_info[0].min[sp_var_info[0].time_min+t] = -mag;
		sp_var_info[0].max[sp_var_info[0].time_min+t] = mag;
		sp_var_info[1].min[sp_var_info[1].time_min+t] = -mag;
		sp_var_info[1].max[sp_var_info[1].time_min+t] = mag;
	}
	for (int i=0; i<sp_var_info.size(); i++) {
		sp_var_info[i].min_over_time =
			sp_var_info[i].min[sp_var_info[i].time_min];
		sp_var_info[i].max_over_time =
			sp_var_info[i].max[sp_var_info[i].time_min];
		for (int t=sp_var_info[i].time_min; t<=sp_var_info[i].time_max; t++) {
			if (sp_var_info[i].min[t] < sp_var_info[i].min_over_time) {
				sp_var_info[i].min_over_time = sp_var_info[i].min[t];
			}
			if (sp_var_info[i].max[t] > sp_var_info[i].max_over_time) {
				sp_var_info[i].max_over_time = sp_var_info[i].max[t];
			}
		}
	}
}

void LisaScatterPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In LisaScatterPlotCanvas::TimeSyncVariableToggle");
	lisa_coord->var_info[var_index].sync_with_global_time =
		!lisa_coord->var_info[var_index].sync_with_global_time;
	lisa_coord->var_info[0].time = var_info[0].time;
	if (is_bi || is_rate) {
		lisa_coord->var_info[1].time = var_info[1].time;
	}
	lisa_coord->VarInfoAttributeChange();
	lisa_coord->InitFromVarInfo();
	lisa_coord->notifyObservers();
}

void LisaScatterPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In LisaScatterPlotCanvas::FixedScaleVariableToggle");
	// Not implemented for now.
	//lisa_coord->var_info[var_index].fixed_scale =
	//	!lisa_coord->var_info[var_index].fixed_scale;
	//lisa_coord->var_info[0].time = var_info[0].time;
	//if (is_bi || is_rate) {
	//	lisa_coord->var_info[1].time = var_info[1].time;
	//}
	//lisa_coord->VarInfoAttributeChange();
	//lisa_coord->notifyObservers();
}

void LisaScatterPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::PopulateCanvas");
	
	// need to modify var_info temporarily for PopulateCanvas since
	var_info_orig = var_info;
	var_info = sp_var_info;
	ScatterNewPlotCanvas::PopulateCanvas();
	var_info = var_info_orig;
	
	LOG_MSG("Exiting LisaScatterPlotCanvas::PopulateCanvas");
}

void LisaScatterPlotCanvas::PopCanvPreResizeShpsHook()
{
	wxString s("Moran's I: ");
	s << regressionXY.beta;
	MyText* morans_i_text = new MyText(s, *GeoDaConst::small_font,
									   wxRealPoint(50, 100), 0,
									   MyText::h_center, MyText::v_center,
									   0, -15);
	morans_i_text->setPen(*GeoDaConst::scatterplot_reg_pen);
	foreground_shps.push_back(morans_i_text);
}

void LisaScatterPlotCanvas::ShowRandomizationDialog(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	std::vector<double> raw_data1(num_obs);
	int xt = var_info_orig[0].time-var_info_orig[0].time_min;
	for (int i=0; i<num_obs; i++) {
		raw_data1[i] = lisa_coord->data1_vecs[xt][i];
	}
	if (is_bi) {
		std::vector<double> raw_data2(num_obs);
		int yt = var_info_orig[1].time-var_info_orig[1].time_min;
		for (int i=0; i<num_obs; i++) {
			raw_data2[i] = lisa_coord->data2_vecs[yt][i];
		}
		RandomizationDlg dlg(raw_data1, raw_data2, lisa_coord->W, permutation,
							 0);
		dlg.ShowModal();
	} else {
		RandomizationDlg dlg(raw_data1, lisa_coord->W, permutation, 0);
		dlg.ShowModal();
	}
}

void LisaScatterPlotCanvas::SaveMoranI()
{
	wxString title = "Save Results: Moran's I";
	std::vector<double> std_data(num_obs);
	std::vector<double> lag(num_obs);
	
	int xt = sp_var_info[0].time-sp_var_info[0].time_min;
	int yt = sp_var_info[1].time-sp_var_info[1].time_min;
	
	for (int i=0; i<num_obs; i++) {
		std_data[i] = x_data[xt][i];
		lag[i] = y_data[yt][i];
	}
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &std_data;
	data[0].label = "Standardized Data";
	data[0].field_default = "MORAN_STD";
	data[0].type = GeoDaConst::double_type;
	data[1].d_val = &lag;
	data[1].label = "Spatial Lag";
	data[1].field_default = "MORAN_LAG";
	data[1].type = GeoDaConst::double_type;
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

IMPLEMENT_CLASS(LisaScatterPlotFrame, ScatterNewPlotFrame)
BEGIN_EVENT_TABLE(LisaScatterPlotFrame, ScatterNewPlotFrame)
	EVT_ACTIVATE(LisaScatterPlotFrame::OnActivate)
END_EVENT_TABLE()

LisaScatterPlotFrame::LisaScatterPlotFrame(wxFrame *parent, Project* project,
										   LisaCoordinator* lisa_coordinator,
										   const wxPoint& pos,
										   const wxSize& size, const long style)
: ScatterNewPlotFrame(parent, project, pos, size, style),
lisa_coord(lisa_coordinator)
{
	LOG_MSG("Entering LisaScatterPlotFrame::LisaScatterPlotFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	template_canvas = new LisaScatterPlotCanvas(this, this, project,
												lisa_coord,
												wxDefaultPosition,
												wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	lisa_coord->registerObserver(this);
	Show(true);
	
	LOG_MSG("Exiting LisaScatterPlotFrame::LisaScatterPlotFrame");
}

LisaScatterPlotFrame::~LisaScatterPlotFrame()
{
	LOG_MSG("In LisaScatterPlotFrame::~LisaScatterPlotFrame");
	lisa_coord->removeObserver(this);
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void LisaScatterPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In LisaScatterPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("LisaScatterPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LisaScatterPlotFrame::MapMenus()
{
	LOG_MSG("In LisaScatterPlotFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISA_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	((ScatterNewPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((ScatterNewPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
		GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void LisaScatterPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("LisaScatterPlotFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((LisaScatterPlotCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void LisaScatterPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LisaScatterPlotFrame::RanXPer(int perm)
{
	((LisaScatterPlotCanvas*) template_canvas)->ShowRandomizationDialog(perm);
}

void LisaScatterPlotFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void LisaScatterPlotFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void LisaScatterPlotFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void LisaScatterPlotFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void LisaScatterPlotFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		RanXPer(num);
	}
}

void LisaScatterPlotFrame::OnSaveMoranI(wxCommandEvent& event)
{
	LisaScatterPlotCanvas* lc = (LisaScatterPlotCanvas*) template_canvas;
	lc->SaveMoranI();
}

/** Called by LisaCoordinator to notify that state has changed.  State changes
 can include:
 - variable sync change and therefore all lisa categories have changed
 - significance level has changed and therefore categories have changed
 - new randomization for p-vals and therefore categories have changed */
void LisaScatterPlotFrame::update(LisaCoordinator* o)
{
	LisaScatterPlotCanvas* lc = (LisaScatterPlotCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
}

