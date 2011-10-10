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
#include "LisaCoordinator.h"
#include "LisaMapView.h"

extern Selection gSelection;
extern GeoDaEventType gEvent;
extern MyFrame* frame;

BEGIN_EVENT_TABLE(LisaMapFrame, wxFrame)
	EVT_ACTIVATE(LisaMapFrame::OnActivate)
    EVT_CLOSE(LisaMapFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), LisaMapFrame::OnMenuClose)
END_EVENT_TABLE()


LisaMapFrame::LisaMapFrame(wxFrame *parent, Project* project,
						   LisaCoordinator* lisa_coordinator,
						   bool isClusterMap,
						   bool isBivariate,
						   bool isEBRate,
						   const wxString& title, const wxPoint& pos,
						   const wxSize& size, const long style)
: MapFrame(project->shp_fname.GetFullPath(),
		   parent, project, title, pos, size, style),
	lisa_coord(lisa_coordinator),
	is_clust(isClusterMap), is_bi(isBivariate), is_rate(isEBRate)
{
	old_style = true;
	canvas->isLisaMap = true;
	canvas->isGetisOrdMap = false;
	
	SetTitle(GetTitle());
	
	// Make default highlight color for polygon maps black
	if (canvas->m_type == ShapeFileTypes::POLYGON) {
		canvas->highlight_color = *wxBLACK;
	}
	canvas->MapFrameSignificanceFilter = lisa_coord->significance_filter;
	
	UpdateCategories();
	lisa_coord->registerObserver(this);
	
	Show(true);
}

LisaMapFrame::~LisaMapFrame()
{
	LOG_MSG("Entering LisaMapFrame::~LisaMapFrame");
	lisa_coord->removeObserver(this);
	if (HasCapture()) ReleaseMouse();
	LOG_MSG("Entering LisaMapFrame::~LisaMapFrame");
}

wxString LisaMapFrame::GetTitle()
{
	wxString lisa_t;
	if (is_clust && !is_bi) lisa_t = " LISA Cluster Map";
	if (is_clust && is_bi) lisa_t = " BiLISA Cluster Map";
	if (!is_clust && !is_bi) lisa_t = " LISA Significance Map";
	if (!is_clust && is_bi) lisa_t = " BiLISA Significance Map";

	wxString field_t;
	if (is_bi) {
		field_t = lisa_coord->field1_name + " w/ "
		+ lisa_coord->field2_name;
	} else {
		field_t = "I_" + lisa_coord->field1_name;
	}
	if (is_rate) {
		field_t = "EB Rate: " + lisa_coord->field1_name + " / "
		+ lisa_coord->field2_name;
	}
	
	wxString ret;
	ret << lisa_t << ": " << lisa_coord->weight_name << ", ";
	ret << field_t << " (" << lisa_coord->permutations << " perm)";
	return ret;
}


void LisaMapFrame::UpdateCategories()
{	
	if (is_clust) {
		m_str[0] = "Not Significant";
		canvas->colors.at(0) = wxColour(240,240,240);
		m_str[1] = "High-High";
		canvas->colors.at(1) = wxColour(255,0,0);
		m_str[2] = "Low-Low";
		canvas->colors.at(2) = wxColour(0,0,255);
		m_str[3] = "Low-High";
		canvas->colors.at(3) = wxColour(150,150,255);
		m_str[4] = "High-Low";
		canvas->colors.at(4) = wxColour(255,150,150);
		m_str[5] = "Neighborless";
		canvas->colors.at(5) = wxColour(140,140,140);
		m_str[6] = "Undefined";
		canvas->colors.at(6) = wxColour(70,70,70);
		
		if (lisa_coord->GetHasUndefined()) {
			canvas->numClasses = 7;
			numBreaks = 7;
		} else if (lisa_coord->GetHasIsolates()) {
			canvas->numClasses = 6;
			numBreaks = 6;
		} else {
			canvas->numClasses = 5;
			numBreaks = 5;
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
		
		if (lisa_coord->GetHasUndefined()) {
			canvas->numClasses = 7;
			numBreaks = 7;
		} else if (lisa_coord->GetHasIsolates()) {
			canvas->numClasses = 6;
			numBreaks = 6;
		} else {
			canvas->numClasses = 5;
			numBreaks = 5;
		}
	}
	
	double cuttoff = lisa_coord->significance_cutoff;
	double* p = lisa_coord->sigLocalMoran;
	int* cluster = lisa_coord->cluster;
	if (is_clust) {
		for (int i=0, iend=lisa_coord->tot_obs; i<iend; i++) {
			if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
				canvas->c_id.at(i) = 0; // not significant
			} else {
				canvas->c_id.at(i) = cluster[i];
			}
		}
	} else {
		for (int i=0, iend=lisa_coord->tot_obs; i<iend; i++) {
			if (cluster[i] == 5 || cluster[i] == 6) {
				// this works because significance and cluster categories
				// line up
				canvas->c_id.at(i) = cluster[i];
			} else if (p[i] > cuttoff) {
				canvas->c_id.at(i) = 0; // not significant
			} else {
				canvas->c_id.at(i) = lisa_coord->sigCat[i];
			}
		}
	}
}


void LisaMapFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void LisaMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In LisaMapFrame::OnActivate");
	canvas->gSelect1.x = -1; // invalidate first selection point
	if (event.GetActive()) {
		RegisterAsActive("LisaMapFrame", GetTitle());
	}
    if ( event.GetActive() && canvas ) canvas->SetFocus();
}

void LisaMapFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In LisaMapFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void LisaMapFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In LisaMapFrame::OnMenuClose");
	Close();
}

void LisaMapFrame::MapMenus()
{
	LOG_MSG("In LisaMapFrame::MapMenus");
	wxMenuBar* mb = frame->GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
						LoadMenu("ID_LISAMAP_VIEW_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateMenuCheckMarks(optMenu);
}

/** Called by LisaCoordinator to notify that state has changed */
void LisaMapFrame::update(LisaCoordinator* o)
{
	UpdateCategories();
	SetTitle(GetTitle());
	canvas->Refresh();
}

void LisaMapFrame::OnRanXPer(wxCommandEvent& event, int permutation)
{
	lisa_coord->permutations = permutation;
	lisa_coord->CalcPseudoP();
	lisa_coord->notifyObservers();
}

void LisaMapFrame::OnRan99Per(wxCommandEvent& event)
{
	OnRanXPer(event, 99);
}

void LisaMapFrame::OnRan199Per(wxCommandEvent& event)
{
	OnRanXPer(event, 199);
}

void LisaMapFrame::OnRan499Per(wxCommandEvent& event)
{
	OnRanXPer(event, 499);
}

void LisaMapFrame::OnRan999Per(wxCommandEvent& event)
{
	OnRanXPer(event, 999);  
}

void LisaMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	CPermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		OnRanXPer(event, num);
	}
}

void LisaMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 1) return;
	canvas->MapFrameSignificanceFilter = 1;
	lisa_coord->SetSignificanceFilter(1);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	lisa_coord->notifyObservers();
}

void LisaMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 2) return;
	canvas->MapFrameSignificanceFilter = 2;
	lisa_coord->SetSignificanceFilter(2);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	lisa_coord->notifyObservers();
}

void LisaMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 3) return;
	canvas->MapFrameSignificanceFilter = 3;
	lisa_coord->SetSignificanceFilter(3);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	lisa_coord->notifyObservers();
}

void LisaMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	if (canvas->MapFrameSignificanceFilter == 4) return;
	canvas->MapFrameSignificanceFilter = 4;
	lisa_coord->SetSignificanceFilter(4);
	UpdateMenuBarCheckMarks(frame->GetMenuBar());
	lisa_coord->notifyObservers();
}

void LisaMapFrame::OnSaveLisa(wxCommandEvent& event)
{
	std::vector<SaveToTableEntry> data(3);
	std::vector<double> tempLocalMoran(lisa_coord->tot_obs);
	for (int i=0, iend=lisa_coord->tot_obs; i<iend; i++) {
		tempLocalMoran[i] = lisa_coord->localMoran[i];
	}
	data[0].d_val = &tempLocalMoran;
	data[0].label = "Lisa Indices";
	data[0].field_default = "LISA_I";
	data[0].type = GeoDaConst::double_type;

	double cuttoff = lisa_coord->significance_cutoff;
	double* p = lisa_coord->sigLocalMoran;
	int* cluster = lisa_coord->cluster;
	std::vector<wxInt64> clust(lisa_coord->tot_obs);
	for (int i=0, iend=lisa_coord->tot_obs; i<iend; i++) {
		if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
			clust[i] = 0; // not significant
		} else {
			clust[i] = cluster[i];
		}
	}
	data[1].l_val = &clust;
	data[1].label = "Clusters";
	data[1].field_default = "LISA_CL";
	data[1].type = GeoDaConst::long64_type;

	std::vector<double> sig(lisa_coord->tot_obs);
	for (int i=0, iend=lisa_coord->tot_obs; i<iend; i++) {
		sig[i] = p[i];
	}
	
	data[2].d_val = &sig;
	data[2].label = "Significances";
	data[2].field_default = "LISA_P";
	data[2].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data,
					   "Save Results: LISA",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void LisaMapFrame::OnSelectCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapFrame::OnSelectCores");
	
	// Clear any selected objects and select all cores
	std::vector<bool> elem(lisa_coord->tot_obs, false);
	
	// add all cores to elem list.
	// NOTE: the following logic only works because the map categories
	// for cluster and significance maps happen to line up.
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 4) {
			elem[i] = true;
		}
	}
	
	gSelection.Reset(true); // reset to empty
	gEvent = NEW_SELECTION;
	gSelection.Update();
	frame->UpdateWholeView(NULL);

	gEvent = NEW_SELECTION;
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	
	LOG_MSG("Exiting LisaMapFrame::OnSelectCores");
}

void LisaMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapFrame::OnSelectNeighborsOfCores");

	// Clear any selected objects and select all neighbors of cores
	std::vector<bool> elem(lisa_coord->tot_obs, false);	
	
	// add all cores and neighbors of cores to elem list
	// NOTE: the following logic only works because the map categories
	// for cluster and significance maps happen to line up.
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 4) {
			elem[i] = true;
			GalElement& e = lisa_coord->gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 4) {
			elem[i] = false;
		}
	}
	
	gSelection.Reset(true); // reset to empty
	gEvent = NEW_SELECTION;
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	
	gEvent = NEW_SELECTION;
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	
	LOG_MSG("Exiting LisaMapFrame::OnSelectNeighborsOfCores");
}

void LisaMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapFrame::OnSelectCoresAndNeighbors");
	// Clear any selected objects and select all cores and
	// their neighbors
	std::vector<bool> elem(lisa_coord->tot_obs, false);
	
	// add all cores and weights to elem list
	// NOTE: the following logic only works because the map categories
	// for cluster and significance maps happen to line up.
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (canvas->c_id.at(i) >= 1 && canvas->c_id.at(i) <= 4) {
			elem[i] = true;
			GalElement& e = lisa_coord->gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	
	gEvent = NEW_SELECTION;
	gSelection.Reset(true);
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	
	gEvent = NEW_SELECTION;
	for (int i=0; i<lisa_coord->tot_obs; i++) {
		if (elem[i]) gSelection.Push(i);
	}
	gSelection.Update();
	frame->UpdateWholeView(NULL);
	gSelection.Reset(true);
		
	LOG_MSG("Exiting LisaMapFrame::OnSelectCoresAndNeighbors");
}

