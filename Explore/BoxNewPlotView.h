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

#ifndef __GEODA_CENTER_BOX_NEW_PLOT_VIEW_H__
#define __GEODA_CENTER_BOX_NEW_PLOT_VIEW_H__

#include <boost/multi_array.hpp>
#include <wx/menu.h>
#include "ThemeUtilities.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GeoDaConst.h"
#include "../GenUtils.h"
#include "../Generic/MyShape.h"

class BoxNewPlotCanvas;
class BoxNewPlotFrame;
typedef boost::multi_array<double, 2> d_array_type;

class BoxNewPlotCanvas : public TemplateCanvas {
	DECLARE_CLASS(BoxNewPlotCanvas)	
public:
	BoxNewPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
					 Project* project,
					 const std::vector<GeoDaVarInfo>& var_info,
					 const std::vector<int>& col_ids,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize);
	virtual ~BoxNewPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HighlightState* o);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	virtual wxString GetNameWithTime(int var, int time);
	virtual wxString GetTimeString(int var, int time);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void DetermineMouseHoverObjects();
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
protected:
	virtual void PopulateCanvas();
	virtual void TitleOrTimeChange();
	void VarInfoAttributeChange();
	
public:
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	virtual void PlotsPerView(int plots_per_view);
	virtual void PlotsPerViewOther();
	virtual void PlotsPerViewAll();	
	void DisplayStatistics(bool display_stats);
	bool IsDisplayStats() { return display_stats; }
	void ShowAxes(bool show_axes);
	bool IsShowAxes() { return show_axes; }
	void Hinge15();
	void Hinge30();
	
protected:
	virtual void UpdateStatusBar();

	Project* project;
	HighlightState* highlight_state;
	int num_obs;
	int num_time_vals;
	int ref_var_index;
	std::vector<GeoDaVarInfo> var_info;
	std::vector<d_array_type> data;
	std::vector<GeoDa::dbl_int_pair_vec_type> data_sorted;
	std::vector<HingeStats> hinge_stats;
	std::vector<SampleStatistics> data_stats;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	AxisScale axis_scale;
	MyAxis* vert_axis;
	std::vector<bool> sel_scratch;

	bool show_axes;
	bool display_stats;
	bool hinge_15; // if true than Hinge == 1.5 else Hinge == 3.0
	
	// 1 <= cur_num_plots <= max_plots <= MAX_BOX_PLOTS
	int cur_num_plots; // number of plots actually shown
	int cur_first_ind;
	int cur_last_ind;
	int max_plots; // min of time_steps and MAX_BOX_PLOTS
	static const int MAX_BOX_PLOTS;
	static const double plot_height_const;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double plot_width_const;
	static const double plot_gap_const;
	
	DECLARE_EVENT_TABLE()
};


class BoxNewPlotFrame : public TemplateFrame {
    DECLARE_CLASS(BoxNewPlotFrame)
public:
    BoxNewPlotFrame(wxFrame *parent, Project* project,
					const std::vector<GeoDaVarInfo>& var_info,
					const std::vector<int>& col_ids,
					const wxString& title = "Box Plot",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GeoDaConst::boxplot_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~BoxNewPlotFrame();
	
public:
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	virtual void UpdateTitle();
	
	void OnShowAxes(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
protected:
	
    DECLARE_EVENT_TABLE()
};


#endif