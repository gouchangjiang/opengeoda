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

#ifndef __GEODA_CENTER_LISA_SCATTER_PLOT_VIEW_H__
#define __GEODA_CENTER_LISA_SCATTER_PLOT_VIEW_H__

#include "../GeoDaConst.h"
#include "ScatterNewPlotView.h"
#include "LisaCoordinatorObserver.h"

class LisaCoordinator;

class LisaScatterPlotCanvas : public ScatterNewPlotCanvas
{
	DECLARE_CLASS(LisaScatterPlotCanvas)
public:	
	LisaScatterPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
						  Project* project,
						  LisaCoordinator* lisa_coordinator,
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = wxDefaultSize);
	virtual ~LisaScatterPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TitleOrTimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	void ShowRandomizationDialog(int permutation);
	void SaveMoranI();
	
protected:
	virtual void PopulateCanvas();
	virtual void PopCanvPreResizeShpsHook();
	LisaCoordinator* lisa_coord;
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Empirical Bayes Rate Smoothing
	std::vector<GeoDaVarInfo> sp_var_info;
	std::vector<GeoDaVarInfo> var_info_orig;
	
	DECLARE_EVENT_TABLE()
};

class LisaScatterPlotFrame : public ScatterNewPlotFrame,
	public LisaCoordinatorObserver
{
	DECLARE_CLASS(LisaScatterPlotFrame)
public:
    LisaScatterPlotFrame(wxFrame *parent, Project* project,
					LisaCoordinator* lisa_coordinator,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GeoDaConst::scatterplot_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LisaScatterPlotFrame();
	
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
	void RandomizationP(int numPermutations);  
	
	void OnSaveMoranI(wxCommandEvent& event);
	
	void update(LisaCoordinator* o);
	
protected:
	LisaCoordinator* lisa_coord;
	
    DECLARE_EVENT_TABLE()
};


#endif
