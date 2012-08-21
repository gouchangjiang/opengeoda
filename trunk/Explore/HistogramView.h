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

#ifndef __GEODA_CENTER_HISTOGRAM_VIEW_H__
#define __GEODA_CENTER_HISTOGRAM_VIEW_H__

#include <vector>
#include <boost/multi_array.hpp>
#include <wx/menu.h>
#include "ThemeUtilities.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GeoDaConst.h"
#include "../GenUtils.h"
#include "../Generic/MyShape.h"

class HistogramCanvas;
class HistogramFrame;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<int, 2> i_array_type;

class HistogramCanvas : public TemplateCanvas {
	DECLARE_CLASS(HistogramCanvas)	
public:
	HistogramCanvas(wxWindow *parent, TemplateFrame* t_frame,
					 Project* project,
					 const std::vector<GeoDaVarInfo>& var_info,
					 const std::vector<int>& col_ids,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize);
	virtual ~HistogramCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HighlightState* o);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
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
	
	void DisplayStatistics(bool display_stats);
	void ShowAxes(bool show_axes);
	
	bool IsDisplayStats() { return display_stats; }
	bool IsShowAxes() { return show_axes; }
	
	void HistogramIntervals();
	void InitIntervals();
	void UpdateIvalSelCnts();
protected:
	virtual void UpdateStatusBar();

	Project* project;
	HighlightState* highlight_state;
	int num_obs;
	int num_time_vals;
	int ref_var_index;
	std::vector<GeoDaVarInfo> var_info;
	std::vector<GeoDa::dbl_int_pair_vec_type> data_sorted;
	std::vector<SampleStatistics> data_stats;
	std::vector<HingeStats> hinge_stats;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	MyAxis* x_axis;
	MyAxis* y_axis;
	bool scale_x_over_time;
	bool scale_y_over_time;
	std::vector<bool> sel_scratch;

	bool show_axes;
	bool display_stats;
	
	double data_min_over_time;
	double data_max_over_time;
	d_array_type ival_breaks; // size = time_steps * cur_num_intervals-1
	std::vector<double> ival_size; // size = time_steps
	std::vector<double> ival_ref_pt; // size = time_steps
	std::vector<double> min_ival_val; // size = time_steps
	std::vector<double> max_ival_val; // size = time_steps
	std::vector<double> max_num_obs_in_ival; // size = time_steps
	double overall_max_num_obs_in_ival;
	int cur_intervals;
	i_array_type ival_obs_cnt; // size = time_steps * cur_num_intervals
	i_array_type ival_obs_sel_cnt;  // size = time_steps * cur_num_intervals
	i_array_type obs_id_to_ival; // size = time_steps * num_obs
	std::vector< std::vector<std::list<int> > > ival_to_obs_ids;
	
	int max_intervals; // min of num_obs and MAX_INTERVALS
	static const int MAX_INTERVALS;
	static const int default_intervals;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	DECLARE_EVENT_TABLE()
};


class HistogramFrame : public TemplateFrame {
    DECLARE_CLASS(HistogramFrame)
public:
    HistogramFrame(wxFrame *parent, Project* project,
					const std::vector<GeoDaVarInfo>& var_info,
					const std::vector<int>& col_ids,
					const wxString& title = "Histogram",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GeoDaConst::boxplot_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~HistogramFrame();
	
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
	void OnHistogramIntervals(wxCommandEvent& event);
protected:
	
    DECLARE_EVENT_TABLE()
};


#endif