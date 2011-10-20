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

#include <vector>
#include <wx/xrc/xmlres.h>
#include "../GeneralWxUtils.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../logger.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "GStatCoordinator.h"
#include "GetisOrdMapView.h"

extern Selection gSelection;
extern GeoDaEventType gEvent;
extern MyFrame* frame;

BEGIN_EVENT_TABLE(GetisOrdMapFrame, wxFrame)
	EVT_ACTIVATE(GetisOrdMapFrame::OnActivate)
    EVT_CLOSE(GetisOrdMapFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), GetisOrdMapFrame::OnMenuClose)
END_EVENT_TABLE()


GetisOrdMapFrame::GetisOrdMapFrame(wxFrame *parent, Project* project,
								   GStatCoordinator* gs_coord_s,
								   GMapType map_type_s,
								   bool row_standardize_weights,
								   const wxString& title, const wxPoint& pos,
								   const wxSize& size, const long style)
	: MapFrame(project->shp_fname.GetFullPath(),
			   parent, project, title, pos, size, style),
	gs_coord(gs_coord_s), map_type(map_type_s),
	row_standardize(row_standardize_weights)
{
	old_style = true;
	canvas->isLisaMap = false;
	canvas->isGetisOrdMap = true;

	is_gi = ( map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
			 map_type == Gi_sig_perm || map_type == Gi_sig_norm );
	is_clust = ( map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
				map_type == GiStar_clus_perm || map_type == GiStar_clus_norm );
	is_perm = ( map_type == Gi_clus_perm || map_type == Gi_sig_perm ||
			   map_type == GiStar_clus_perm || map_type == GiStar_sig_perm );
	
	SetTitle(GetTitle());
	
	// Make default highlight color for polygon maps black
	if (canvas->m_type == ShapeFileTypes::POLYGON) {
		canvas->highlight_color = *wxBLACK;
	}
	canvas->MapFrameSignificanceFilter = gs_coord->significance_filter;
	
	UpdateCategories();
	gs_coord->registerObserver(this);
	
	Show(true);
}

GetisOrdMapFrame::~GetisOrdMapFrame()
{
	LOG_MSG("Entering GetisOrdMapFrame::~GetisOrdMapFrame");
	gs_coord->removeObserver(this);
	if (HasCapture()) ReleaseMouse();
	LOG_MSG("Exiting GetisOrdMapFrame::~GetisOrdMapFrame");
}

wxString GetisOrdMapFrame::GetTitle()
{
	wxString new_title;
	new_title << (is_gi ? "Gi " : "Gi* ");
	new_title << (is_clust ? "Cluster" : "Significance") << " Map ";
	new_title << "(" << gs_coord->weight_name << "): ";
	new_title << gs_coord->field_name;
	if (is_perm) new_title << wxString::Format(", pseudo p (%d perm)",
											   gs_coord->permutations);
	if (!is_perm) new_title << ", normal p";
	new_title << (row_standardize ? ", row-standardized W" :
				  ", binary W");
	return new_title;
}

void GetisOrdMapFrame::UpdateCategories()
{
	if (is_clust) {
		m_str[0] = "Not Significant";
		canvas->colors.at(0) = wxColour(240,240,240);
		m_str[1] = "High";
		canvas->colors.at(1) = wxColour(255,0,0);
		m_str[2] = "Low";
		canvas->colors.at(2) = wxColour(0,0,255);
		m_str[3] = "Neighborless";
		canvas->colors.at(3) = wxColour(140,140,140);
		m_str[4] = "Undefined";
		canvas->colors.at(4) = wxColour(70,70,70);
		
		if (gs_coord->GetHasUndefined()) {
			canvas->numClasses = 5;
			numBreaks = 5;
		} else if (gs_coord->GetHasIsolates()) {
			canvas->numClasses = 4;
			numBreaks = 4;
		} else {
			canvas->numClasses = 3;
			numBreaks = 3;
		}
	} else {
		m_str[0] = "Not Significant";
		canvas->colors.at(0) = wxColour(240,240,240);
		m_str[1] = "p = 0.05";
		canvas->colors.at(1) = wxColour(75,255,80);
		m_str[2] = "p = 0.01";
		canvas->colors.at(2) = wxColour(6,196,11);
		m_str[3] = "p = 0.001";
		canvas->colors.at(3) = wxColour(3,116,6);
		m_str[4] = "p = 0.0001";
		canvas->colors.at(4) = wxColour(1,70,3);
		m_str[5] = "Neighborless";
		canvas->colors.at(5) = wxColour(140,140,140);
		m_str[6] = "Undefined";
		canvas->colors.at(6) = wxColour(70,70,70);
		
		if (gs_coord->GetHasUndefined()) {
			canvas->numClasses = 7;
			numBreaks = 7;
		} else if (gs_coord->GetHasIsolates()) {
			canvas->numClasses = 6;
			numBreaks = 6;
		} else {
			canvas->numClasses = 5;
			numBreaks = 5;
		}
	}
	
	std::vector<double> empty_p(0);
	std::vector<double>& p = empty_p;
	std::vector<double>& z = is_gi ? gs_coord->z : gs_coord->z_star;
	
	if (map_type == Gi_clus_perm || map_type == Gi_sig_perm) {
		p = gs_coord->pseudo_p;
	} else if (map_type == Gi_clus_norm || map_type == Gi_sig_norm) {
		p = gs_coord->p;
	} else if (map_type == GiStar_clus_perm || map_type == GiStar_sig_perm) {
		p = gs_coord->pseudo_p_star;
	} else if (map_type == GiStar_clus_norm || map_type == GiStar_sig_norm) {
		p = gs_coord->p_star;
	}
	
	double cuttoff = gs_coord->significance_cutoff;
	if (is_clust) {
		for (int i=0, iend=p.size(); i<iend; i++) {
			if (gs_coord->W[i].Size() == 0) {
				canvas->c_id.at(i) = 3; // isolate
			} else if (!gs_coord->G_defined[i]) {
				canvas->c_id.at(i) = 4; // undefined
			} else if (p[i] <= cuttoff) {
				if (z[i] > 0) {
					canvas->c_id.at(i) = 1; // high
				} else {
					canvas->c_id.at(i) = 2; // low
				}
			} else {
				canvas->c_id.at(i) = 0; // not significant
			}
		}
	} else {
		for (int i=0, iend=p.size(); i<iend; i++) {
			if (gs_coord->W[i].Size() == 0) {
				canvas->c_id.at(i) = 5; // isolate
			} else if (!gs_coord->G_defined[i]) {
				canvas->c_id.at(i) = 6; // undefined
			} else if (p[i] <= cuttoff) {
				if (p[i] <= 0.0001) {
					canvas->c_id.at(i) = 4;
				} else if (p[i] <= 0.001) {
					canvas->c_id.at(i) = 3;
				} else if (p[i] <= 0.01) {
					canvas->c_id.at(i) = 2;
				} else { // p[i] <= 0.05
					canvas->c_id.at(i) = 1;
				}
			} else {
				canvas->c_id.at(i) = 0; // not significant
			}
		}
	}	
}

void GetisOrdMapFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void GetisOrdMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In GetisOrdMapFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("GetisOrdMapFrame", GetTitle());
	}
    if ( event.GetActive() && canvas ) canvas->SetFocus();
}

void GetisOrdMapFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In GetisOrdMapFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void GetisOrdMapFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In GetisOrdMapFrame::OnMenuClose");
	Close();
}

void GetisOrdMapFrame::MapMenus()
{
	LOG_MSG("In GetisOrdMapFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
						LoadMenu("ID_GETIS_ORD_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateMenuCheckMarks(optMenu);
}

/** Called by GStatCoordinator to notify that state has changed */
void GetisOrdMapFrame::update(GStatCoordinator* o)
{
	UpdateCategories();
	SetTitle(GetTitle());
	canvas->Refresh();
}

void GetisOrdMapFrame::OnRanXPer(wxCommandEvent& event, int permutation)
{
	gs_coord->permutations = permutation;
	gs_coord->CalcPseudoP();
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnRan99Per(wxCommandEvent& event)
{
	OnRanXPer(event, 99);
}

void GetisOrdMapFrame::OnRan199Per(wxCommandEvent& event)
{
	OnRanXPer(event, 199);
}

void GetisOrdMapFrame::OnRan499Per(wxCommandEvent& event)
{
	OnRanXPer(event, 499);
}

void GetisOrdMapFrame::OnRan999Per(wxCommandEvent& event)
{
	OnRanXPer(event, 999);  
}

void GetisOrdMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	CPermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		OnRanXPer(event, num);
	}
}

void GetisOrdMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 1) return;
	canvas->MapFrameSignificanceFilter = 1;
	gs_coord->SetSignificanceFilter(1);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 2) return;
	canvas->MapFrameSignificanceFilter = 2;
	gs_coord->SetSignificanceFilter(2);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 3) return;
	canvas->MapFrameSignificanceFilter = 3;
	gs_coord->SetSignificanceFilter(3);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 4) return;
	canvas->MapFrameSignificanceFilter = 4;
	gs_coord->SetSignificanceFilter(4);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	wxString title = "Save Results: ";
	title += is_gi ? "Gi" : "Gi*";
	title += "-stats, ";
	if (is_perm) title += wxString::Format("pseudo p (%d perm), ",
										   gs_coord->permutations);
	if (!is_perm) title += "normal p, ";
	title += row_standardize ? "row-standarized W" : "binary W";
	
	std::vector<double>& g_val = is_gi ? gs_coord->G : gs_coord->G_star;
	wxString g_label = is_gi ? "G" : "G*";
	wxString g_field_default = is_gi ? "G" : "G_STR";
	
	std::vector<wxInt64> c_val(gs_coord->tot_obs);
	wxString c_label = "cluster category";
	wxString c_field_default = "C_ID";
	
	std::vector<double> e;
	std::vector<double>& p_val = e;
	std::vector<double>& z_val = is_gi ? gs_coord->z : gs_coord->z_star;
	wxString p_label = is_perm ? "pseudo p-value" : "p-value";
	wxString p_field_default = is_perm ? "PP_VAL" : "P_VAL";
	
	if (map_type == Gi_clus_perm || map_type == Gi_sig_perm) {
		p_val = gs_coord->pseudo_p;
	} else if (map_type == Gi_clus_norm || map_type == Gi_sig_norm) {
		p_val = gs_coord->p;
	} else if (map_type == GiStar_clus_perm || map_type == GiStar_sig_perm) {
		p_val = gs_coord->pseudo_p_star;
	} else if (map_type == GiStar_clus_norm || map_type == GiStar_sig_norm) {
		p_val = gs_coord->p_star;
	}
	
	double cuttoff = gs_coord->significance_cutoff;
	for (int i=0, iend=p_val.size(); i<iend; i++) {
		if (gs_coord->W[i].Size() == 0) {
			c_val[i] = 3; // isolate
		} else if (!gs_coord->G_defined[i]) {
			c_val[i] = 4; // undefined
		} else if (p_val[i] <= cuttoff) {
			c_val[i] = z_val[i] > 0 ? 1 : 2; // high = 1, low = 2
		} else {
			c_val[i] = 0; // not significant
		}
	}
	
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
		data[data_i].d_val = is_gi ? &gs_coord->z : &gs_coord->z_star;
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

void GetisOrdMapFrame::OnSelectCores(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectCores");
	
	// Clear any selected objects and select all cores
	std::vector<bool> elem(gs_coord->gal_weights->num_obs, false);
	
	// add all cores to elem list
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if ((canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 2) ||
			(!is_clust && canvas->c_id.at(i) == 3)) {
			elem[i] = true;
		}
	}
	
	gSelection.Reset(true); // reset to empty
	gEvent = NEW_SELECTION;
	gSelection.Update();
	frame->UpdateWholeView(NULL);

	gEvent = NEW_SELECTION;
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectCores");
}

void GetisOrdMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectNeighborsOfCores");

	// Clear any selected objects and select all neighbors of cores
	std::vector<bool> elem(gs_coord->gal_weights->num_obs, false);	
	
	// add all cores and weights to elem list
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if ((canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 2) ||
			(!is_clust && canvas->c_id.at(i) == 3)) {
			elem[i] = true;
			GalElement& e = gs_coord->gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if ((canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 2) ||
			(!is_clust && canvas->c_id.at(i) == 3)) {
			elem[i] = false;
		}
	}

	gSelection.Reset(true); // reset to empty
	gEvent = NEW_SELECTION;
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	
	gEvent = NEW_SELECTION;
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectNeighborsOfCores");
}

void GetisOrdMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectCoresAndNeighbors");
	// Clear any selected objects and select all cores and
	// their neighbors
	std::vector<bool> elem(gs_coord->gal_weights->num_obs, false);
	
	// add all cores and neighbors to elem list
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if ((canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 2) ||
			(!is_clust && canvas->c_id.at(i) == 3)) {
			elem[i] = true;
			GalElement& e = gs_coord->gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	
	gEvent = NEW_SELECTION;
	gSelection.Reset(true);
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	
	gEvent = NEW_SELECTION;
	for (int i=0; i<gs_coord->gal_weights->num_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
		
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectCoresAndNeighbors");
}
