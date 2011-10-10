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

#ifndef __GEODA_CENTER_LISA_MAP_VIEW_H__
#define __GEODA_CENTER_LISA_MAP_VIEW_H__

#include "../mapview.h"
#include "../Explore/LisaCoordinatorObserver.h"

class LisaCoordinator;

class LisaMapFrame: public MapFrame, public LisaCoordinatorObserver
{
public:
    LisaMapFrame(wxFrame *parent, Project* project,
				 LisaCoordinator* lisa_coordinator,
				 bool isClusterMap,
				 bool isBivariate,
				 bool isEBRate,
				 const wxString& title, const wxPoint& pos,
				 const wxSize& size, const long style);
    virtual ~LisaMapFrame();

    void OnQuit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();

	void OnRanXPer(wxCommandEvent& event, int permutation);
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);

	void OnSigFilterX(wxCommandEvent& event, int filter);	
	void OnSigFilter05(wxCommandEvent& event);
	void OnSigFilter01(wxCommandEvent& event);
	void OnSigFilter001(wxCommandEvent& event);
	void OnSigFilter0001(wxCommandEvent& event);

	void OnSaveLisa(wxCommandEvent& event);

	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	
	wxString GetTitle();
	void UpdateCategories();
	void update(LisaCoordinator* o);
	
private:
	LisaCoordinator* lisa_coord;
	bool is_clust; // true = Cluster Map, false = Significance Map
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Emperical Bayes Rate Smoothing
	
    DECLARE_EVENT_TABLE()
};

#endif
