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

#include "GetisOrdMapNewView.h"

#include <vector>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../OpenGeoDa.h"
#include "../logger.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "GStatCoordinator.h"
#include "GetisOrdMapNewView.h"

IMPLEMENT_CLASS(GetisOrdMapNewCanvas, MapNewCanvas)
BEGIN_EVENT_TABLE(GetisOrdMapNewCanvas, MapNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

GetisOrdMapNewCanvas::GetisOrdMapNewCanvas(wxWindow *parent,
										   TemplateFrame* t_frame,
										   Project* project,
										   GStatCoordinator* gs_coordinator,
										   bool is_gi_s, bool is_clust_s,
										   bool is_perm_s,
										   bool row_standardize_s,
										   const wxPoint& pos,
										   const wxSize& size)
: MapNewCanvas(parent, t_frame, project, ThemeUtilities::no_theme,
			   no_smoothing, pos, size),
gs_coord(gs_coordinator),
is_gi(is_gi_s), is_clust(is_clust_s), is_perm(is_perm_s),
row_standardize(row_standardize_s)
{
	LOG_MSG("Entering GetisOrdMapNewCanvas::GetisOrdMapNewCanvas");
	
	if (is_clust) {
		theme_type = ThemeUtilities::getis_ord_categories;
	} else {
		theme_type = ThemeUtilities::getis_ord_significance;
	}
	
	// must set var_info times from GStatCoordinator initially
	var_info = gs_coord->var_info;
	CreateAndUpdateCategories();
	
	LOG_MSG("Exiting GetisOrdMapNewCanvas::GetisOrdMapNewCanvas");
}

GetisOrdMapNewCanvas::~GetisOrdMapNewCanvas()
{
	LOG_MSG("Entering GetisOrdMapNewCanvas::~GetisOrdMapNewCanvas");
	LOG_MSG("Exiting GetisOrdMapNewCanvas::~GetisOrdMapNewCanvas");
}

void GetisOrdMapNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering GetisOrdMapNewCanvas::DisplayRightClickMenu");
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting MapNewCanvas::DisplayRightClickMenu");
}

wxString GetisOrdMapNewCanvas::GetCanvasTitle()
{
	wxString new_title;
	new_title << (is_gi ? "Gi " : "Gi* ");
	new_title << (is_clust ? "Cluster" : "Significance") << " Map ";
	new_title << "(" << gs_coord->weight_name << "): ";
	new_title << GetNameWithTime(0);
	if (is_perm) new_title << wxString::Format(", pseudo p (%d perm)",
											   gs_coord->permutations);
	if (!is_perm) new_title << ", normal p";
	new_title << (row_standardize ? ", row-standardized W" :
				  ", binary W");
	return new_title;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by GetisOrd maps */
bool GetisOrdMapNewCanvas::ChangeMapType(ThemeUtilities::ThemeType new_theme,
										 SmoothingType new_smoothing)
{
	LOG_MSG("In GetisOrdMapNewCanvas::ChangeMapType");
	return false;
}

void GetisOrdMapNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapNewCanvas::SetCheckMarks(menu);
	
	int sig_filter = ((GetisOrdMapNewFrame*) template_frame)->
		GetGStatCoordinator()->GetSignificanceFilter();
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
								  sig_filter == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
								  sig_filter == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
								  sig_filter == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
								  sig_filter == 4);
}

void GetisOrdMapNewCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering GetisOrdMapNewCanvas::TitleOrTimeChange");
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
	LOG_MSG("Exiting GetisOrdMapNewCanvas::TitleOrTimeChange");
}

/** Update Categories based on info in GStatCoordinator */
void GetisOrdMapNewCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	CreateEmptyCategories(num_time_vals);
	
	std::vector<wxInt64> cluster;
	for (int t=0; t<num_time_vals; t++) {
		if (!map_valid[t]) break;
		
		int undefined_cat = -1;
		int isolates_cat = -1;
		int num_cats = 0;
		if (gs_coord->GetHasIsolates(t)) num_cats++;
		if (gs_coord->GetHasUndefined(t)) num_cats++;
		if (is_clust) {
			num_cats += 3;
		} else {
			num_cats += 6-gs_coord->GetSignificanceFilter();
		}
		CreateCategoriesAtCanvasTm(num_cats, t);
		
		if (is_clust) {
			SetCategoryLabel(t, 0, "Not Significant");
			SetCategoryColor(t, 0, wxColour(240, 240, 240));
			SetCategoryLabel(t, 1, "High");
			SetCategoryColor(t, 1, wxColour(255, 0, 0));
			SetCategoryLabel(t, 2, "Low");
			SetCategoryColor(t, 2, wxColour(0, 0, 255));
			if (gs_coord->GetHasIsolates(t) &&
				gs_coord->GetHasUndefined(t)) {
				isolates_cat = 3;
				undefined_cat = 4;
			} else if (gs_coord->GetHasUndefined(t)) {
				undefined_cat = 3;
			} else if (gs_coord->GetHasIsolates(t)) {
				isolates_cat = 3;
			}
		} else {
			// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
			int s_f = gs_coord->GetSignificanceFilter();
			SetCategoryLabel(t, 0, "Not Significant");
			SetCategoryColor(t, 0, wxColour(240, 240, 240));
			
			SetCategoryLabel(t, 5-s_f, "p = 0.0001");
			SetCategoryColor(t, 5-s_f, wxColour(1, 70, 3));
			if (s_f <= 3) {
				SetCategoryLabel(t, 4-s_f, "p = 0.001");
				SetCategoryColor(t, 4-s_f, wxColour(3, 116, 6));	
			}
			if (s_f <= 2) {
				SetCategoryLabel(t, 3-s_f, "p = 0.01");
				SetCategoryColor(t, 3-s_f, wxColour(6, 196, 11));	
			}
			if (s_f <= 1) {
				SetCategoryLabel(t, 2-s_f, "p = 0.05");
				SetCategoryColor(t, 2-s_f, wxColour(75, 255, 80));
			}
			if (gs_coord->GetHasIsolates(t) &&
				gs_coord->GetHasUndefined(t)) {
				isolates_cat = 6-s_f;
				undefined_cat = 7-s_f;
			} else if (gs_coord->GetHasUndefined(t)) {
				undefined_cat = 6-s_f;
			} else if (gs_coord->GetHasIsolates(t)) {
				isolates_cat = 6-s_f;
			}
		}
		if (undefined_cat != -1) {
			SetCategoryLabel(t, undefined_cat, "Undefined");
			SetCategoryColor(t, undefined_cat, wxColour(70, 70, 70));
		}
		if (isolates_cat != -1) {
			SetCategoryLabel(t, isolates_cat, "Neighborless");
			SetCategoryColor(t, isolates_cat, wxColour(140, 140, 140));
		}
		
		gs_coord->FillClusterCats(t, is_gi, is_perm, cluster);
		
		if (is_clust) {
			for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
				if (cluster[i] == 0) {
					AppendIdToCategory(t, 0, i); // not significant
				} else if (cluster[i] == 3) {
					AppendIdToCategory(t, isolates_cat, i);
				} else if (cluster[i] == 4) {
					AppendIdToCategory(t, undefined_cat, i);
				} else {
					AppendIdToCategory(t, cluster[i], i);
				}
			}
		} else {
			double* p_val = 0;
			if (is_gi && is_perm) p_val = gs_coord->pseudo_p_vecs[t];
			if (is_gi && !is_perm) p_val = gs_coord->p_vecs[t];
			if (!is_gi && is_perm) p_val = gs_coord->pseudo_p_star_vecs[t];
			if (!is_gi && !is_perm) p_val = gs_coord->p_star_vecs[t];
			int s_f = gs_coord->GetSignificanceFilter();
			for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
				if (cluster[i] == 0) {
					AppendIdToCategory(t, 0, i); // not significant
				} else if (cluster[i] == 3) {
					AppendIdToCategory(t, isolates_cat, i);
				} else if (cluster[i] == 4) {
					AppendIdToCategory(t, undefined_cat, i);
				} else if (p_val[i] <= 0.0001) {
					AppendIdToCategory(t, 5-s_f, i);
				} else if (p_val[i] <= 0.001) {
					AppendIdToCategory(t, 4-s_f, i);
				} else if (p_val[i] <= 0.01) {
					AppendIdToCategory(t, 3-s_f, i);
				} else if (p_val[i] <= 0.05) {
					AppendIdToCategory(t, 2-s_f, i);
				}
			}
		}
		for (int cat=0; cat<num_cats; cat++) {
			wxString s = GetCategoryLabel(t, cat);
			s << " (" << GetNumObsInCategory(t, cat) << ")";
			SetCategoryLabel(t, cat, s);
		}
	}
	
	if (ref_var_index != -1) {
		SetCurrentCanvasTmStep(var_info[ref_var_index].time
							   - var_info[ref_var_index].time_min);
	}
	PopulateCanvas();
}

void GetisOrdMapNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In GetisOrdMapNewCanvas::TimeSyncVariableToggle");
	gs_coord->var_info[var_index].sync_with_global_time =
	!gs_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		gs_coord->var_info[i].time = var_info[i].time;
	}
	gs_coord->VarInfoAttributeChange();
	gs_coord->InitFromVarInfo();
	gs_coord->notifyObservers();
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void GetisOrdMapNewCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = gs_coord->var_info;
	for (int t=0; t<var_info.size(); t++) var_info[t].time = my_times[t];
	is_any_time_variant = gs_coord->is_any_time_variant;
	is_any_sync_with_global_time = gs_coord->is_any_sync_with_global_time;
	ref_var_index = gs_coord->ref_var_index;
	num_time_vals = gs_coord->num_time_vals;
	map_valid = gs_coord->map_valid;
	map_error_message = gs_coord->map_error_message;
}

IMPLEMENT_CLASS(GetisOrdMapNewFrame, MapNewFrame)
	BEGIN_EVENT_TABLE(GetisOrdMapNewFrame, MapNewFrame)
	EVT_ACTIVATE(GetisOrdMapNewFrame::OnActivate)
END_EVENT_TABLE()

GetisOrdMapNewFrame::GetisOrdMapNewFrame(wxFrame *parent, Project* project,
										 GStatCoordinator* gs_coordinator,
										 GMapType map_type_s,
										 bool row_standardize_s,
										 const wxPoint& pos, const wxSize& size,
										 const long style)
: MapNewFrame(parent, project, pos, size, style),
gs_coord(gs_coordinator), map_type(map_type_s),
row_standardize(row_standardize_s)
{
	LOG_MSG("Entering GetisOrdMapNewFrame::GetisOrdMapNewFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
	
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	is_gi = (map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
			 map_type == Gi_sig_perm || map_type == Gi_sig_norm);
	is_clust = (map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
				map_type == GiStar_clus_perm || map_type == GiStar_clus_norm);
	is_perm = (map_type == Gi_clus_perm || map_type == Gi_sig_perm ||
			   map_type == GiStar_clus_perm || map_type == GiStar_sig_perm);
	
	template_canvas = new GetisOrdMapNewCanvas(splitter_win, this, project,
											   gs_coordinator,
											   is_gi, is_clust, is_perm,
											   row_standardize,
											   wxDefaultPosition,
											   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
	template_legend = new MapNewLegend(splitter_win, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								  GeoDaConst::map_default_legend_width);
	
	gs_coord->registerObserver(this);
	
	Show(true);
	LOG_MSG("Exiting GetisOrdMapNewFrame::GetisOrdMapNewFrame");
}

GetisOrdMapNewFrame::~GetisOrdMapNewFrame()
{
	LOG_MSG("In GetisOrdMapNewFrame::~GetisOrdMapNewFrame");
	gs_coord->removeObserver(this);
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void GetisOrdMapNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In GetisOrdMapNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("GetisOrdMapNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void GetisOrdMapNewFrame::MapMenus()
{
	LOG_MSG("In GetisOrdMapNewFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	((MapNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void GetisOrdMapNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("GetisOrdMapNewFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((GetisOrdMapNewCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void GetisOrdMapNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void GetisOrdMapNewFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	gs_coord->permutations = permutation;
	gs_coord->CalcPseudoP();
	gs_coord->notifyObservers();
}

void GetisOrdMapNewFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void GetisOrdMapNewFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void GetisOrdMapNewFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void GetisOrdMapNewFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void GetisOrdMapNewFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		RanXPer(num);
	}
}

void GetisOrdMapNewFrame::SetSigFilterX(int filter)
{
	if (filter == gs_coord->GetSignificanceFilter()) return;
	gs_coord->SetSignificanceFilter(filter);
	gs_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void GetisOrdMapNewFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void GetisOrdMapNewFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void GetisOrdMapNewFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void GetisOrdMapNewFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void GetisOrdMapNewFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	int t = template_canvas->GetCurrentCanvasTmStep();
	wxString title = "Save Results: ";
	title += is_gi ? "Gi" : "Gi*";
	title += "-stats, ";
	if (is_perm) title += wxString::Format("pseudo p (%d perm), ",
										   gs_coord->permutations);
	if (!is_perm) title += "normal p, ";
	title += row_standardize ? "row-standarized W" : "binary W";
	
	double* g_val_t = is_gi ? gs_coord->G_vecs[t] : gs_coord->G_star_vecs[t];
	std::vector<double> g_val(gs_coord->num_obs);
	for (int i=0; i<gs_coord->num_obs; i++) g_val[i] = g_val_t[i];
	wxString g_label = is_gi ? "G" : "G*";
	wxString g_field_default = is_gi ? "G" : "G_STR";
	
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(t, is_gi, is_perm, c_val);
	wxString c_label = "cluster category";
	wxString c_field_default = "C_ID";
	
	double* p_val_t = 0;
	std::vector<double> p_val(gs_coord->num_obs);
	double* z_val_t = is_gi ? gs_coord->z_vecs[t] : gs_coord->z_star_vecs[t];
	std::vector<double> z_val(gs_coord->num_obs);
	for (int i=0; i<gs_coord->num_obs; i++) z_val[i] = z_val_t[i];
	wxString p_label = is_perm ? "pseudo p-value" : "p-value";
	wxString p_field_default = is_perm ? "PP_VAL" : "P_VAL";
	
	if (map_type == Gi_clus_perm || map_type == Gi_sig_perm) {
		p_val_t = gs_coord->pseudo_p_vecs[t];
	} else if (map_type == Gi_clus_norm || map_type == Gi_sig_norm) {
		p_val_t = gs_coord->p_vecs[t];
	} else if (map_type == GiStar_clus_perm || map_type == GiStar_sig_perm) {
		p_val_t = gs_coord->pseudo_p_star_vecs[t];
	} else { // (map_type == GiStar_clus_norm || map_type == GiStar_sig_norm)
		p_val_t = gs_coord->p_star_vecs[t];
	}
	for (int i=0; i<gs_coord->num_obs; i++) p_val[i] = p_val_t[i];
	
	std::vector<SaveToTableEntry> data(is_perm ? 3 : 4);
	int data_i = 0;
	data[data_i].d_val = &g_val;
	data[data_i].label = g_label;
	data[data_i].field_default = g_field_default;
	data[data_i].type = GeoDaConst::double_type;
	data_i++;
	data[data_i].l_val = &c_val;
	data[data_i].label = c_label;
	data[data_i].field_default = c_field_default;
	data[data_i].type = GeoDaConst::long64_type;
	data_i++;
	if (!is_perm) {
		data[data_i].d_val = &z_val;
		data[data_i].label = "z-score";
		data[data_i].field_default = "Z_SCR";
		data[data_i].type = GeoDaConst::double_type;
		data_i++;
	}
	data[data_i].d_val = &p_val;
	data[data_i].label = p_label;
	data[data_i].field_default = p_field_default;
	data[data_i].type = GeoDaConst::double_type;
	data_i++;
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void GetisOrdMapNewFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->highlight_state;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (!hs[i] && elem[i]) {
			nh[total_newly_selected++] = i;
		} else if (hs[i] && !elem[i]) {
			nuh[total_newly_unselected++] = i;
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetEventType(HighlightState::delta);
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		highlight_state->notifyObservers();
	}
}

void GetisOrdMapNewFrame::OnSelectCores(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapNewFrame::OnSelectCores");
		
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);

	// add all cores to elem list.
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) elem[i] = true;
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting GetisOrdMapNewFrame::OnSelectCores");
}

void GetisOrdMapNewFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapNewFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = true;
			const GalElement& e = gs_coord->W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = false;
		}
	}
	CoreSelectHelper(elem);	
	
	LOG_MSG("Exiting GetisOrdMapNewFrame::OnSelectNeighborsOfCores");
}

void GetisOrdMapNewFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapNewFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = true;
			const GalElement& e = gs_coord->W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting GetisOrdMapNewFrame::OnSelectCoresAndNeighbors");
}

/** Called by GStatCoordinator to notify that state has changed.  State changes
 can include:
 - variable sync change and therefore all Gi categories have changed
 - significance level has changed and therefore categories have changed
 - new randomization for p-vals and therefore categories have changed */
void GetisOrdMapNewFrame::update(GStatCoordinator* o)
{
	GetisOrdMapNewCanvas* lc = (GetisOrdMapNewCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Refresh();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
}
