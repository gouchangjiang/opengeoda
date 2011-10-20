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

#ifndef __GEODA_CENTER_GETIS_ORD_MAP_VIEW_H__
#define __GEODA_CENTER_GETIS_ORD_MAP_VIEW_H__

#include "../mapview.h"

class GStatCoordinator;

class GetisOrdMapFrame: public MapFrame
{
public:
	enum GMapType {
		Gi_clus_perm = 0,		// G_i, cluster, permutation
		Gi_clus_norm = 1,		// G_i, cluster, normal_dist
		Gi_sig_perm = 2,		// G_i, significance, permutation
		Gi_sig_norm = 3,		// G_i, significance, normal_dist
		GiStar_clus_perm = 4,	// G_i_star, cluster, permutation
		GiStar_clus_norm = 5,	// G_i_star, cluster, normal_dist
		GiStar_sig_perm = 6,	// G_i_star, significance, permutation
		GiStar_sig_norm = 7,	// G_i_star, significance, normal_dist
	};
	
    GetisOrdMapFrame(wxFrame *parent, Project* project,
					 GStatCoordinator* gs_coord,
					 GMapType map_type,
					 bool row_standardize_weights,
					 const wxString& title, const wxPoint& pos,
					 const wxSize& size, const long style);
    virtual ~GetisOrdMapFrame();
	
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

	void OnSaveGetisOrd(wxCommandEvent& event);

	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	
	wxString GetTitle();
	GMapType map_type;
	void UpdateCategories();
	void update(GStatCoordinator* o);
	
private:
	GStatCoordinator* gs_coord;
	bool is_gi; // true = Gi, false = GiStar
	bool is_clust; // true = cluster map, false = significance map
	bool is_perm; // true = pseudo-p-val, false = normal distribution p-val
	bool row_standardize; // true = row standardize, false = binary
	
    DECLARE_EVENT_TABLE()
};

#endif
