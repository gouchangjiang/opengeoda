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

#ifndef __GEODA_CENTER_LISA_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_LISA_MAP_NEW_VIEW_H__

#include "MapNewView.h"
#include "LisaCoordinatorObserver.h"

class LisaMapNewFrame;
class LisaMapNewCanvas;
class LisaCoordinator;

class LisaMapNewCanvas : public MapNewCanvas
{
	DECLARE_CLASS(LisaMapNewCanvas)
public:	
	LisaMapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
					 Project* project,
					 LisaCoordinator* lisa_coordinator,
					 ThemeUtilities::ThemeType theme_type,
					 bool isBivariate, bool isEBRate,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize);
	virtual ~LisaMapNewCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	virtual bool ChangeMapType(ThemeUtilities::ThemeType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TitleOrTimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
	
protected:
	LisaCoordinator* lisa_coord;
	bool is_clust; // true = Cluster Map, false = Significance Map
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Empirical Bayes Rate Smoothing
	
	DECLARE_EVENT_TABLE()
};

class LisaMapNewFrame : public MapNewFrame, public LisaCoordinatorObserver
{
	DECLARE_CLASS(LisaMapNewFrame)
public:
    LisaMapNewFrame(wxFrame *parent, Project* project,
					LisaCoordinator* lisa_coordinator,
					bool isClusterMap, bool isBivariate,
					bool isEBRate,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GeoDaConst::map_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LisaMapNewFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	void RanXPer(int permutation);
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);
	
	void SetSigFilterX(int filter);
	void OnSigFilter05(wxCommandEvent& event);
	void OnSigFilter01(wxCommandEvent& event);
	void OnSigFilter001(wxCommandEvent& event);
	void OnSigFilter0001(wxCommandEvent& event);
	
	void OnSaveLisa(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	
	void update(LisaCoordinator* o);
	
protected:
	void CoreSelectHelper(const std::vector<bool>& elem);
	LisaCoordinator* lisa_coord;
	
    DECLARE_EVENT_TABLE()
};

#endif
