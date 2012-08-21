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

#ifndef __GEODA_CENTER_LISA_COORDINATOR_H__
#define __GEODA_CENTER_LISA_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../GenUtils.h"
#include "../ShapeOperations/GalWeight.h"

class LisaCoordinatorObserver;
class LisaCoordinator;
typedef boost::multi_array<double, 2> d_array_type;

class LisaWorkerThread : public wxThread
{
public:
	LisaWorkerThread(int obs_start, int obs_end,
					 LisaCoordinator* lisa_coord,
					 wxMutex* worker_list_mutex,
					 wxCondition* worker_list_empty_cond,
					 std::list<wxThread*> *worker_list,
					 int thread_id);
	virtual ~LisaWorkerThread();
	virtual void* Entry();  // thread execution starts here

	int obs_start;
	int obs_end;
	int thread_id;
	
	LisaCoordinator* lisa_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class LisaCoordinator
{
public:
	enum LisaType { univariate, bivariate, eb_rate_standardized }; // #9
	
	LisaCoordinator(const GalWeight* gal_weights,
					DbfGridTableBase* grid_base,
					const std::vector<GeoDaVarInfo>& var_info,
					const std::vector<int>& col_ids,
					LisaType lisa_type, bool calc_significances = true);
	virtual ~LisaCoordinator();
	
	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }

	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
	void SetSignificanceFilter(int filter_id);
	int GetSignificanceFilter() { return significance_filter; }
	int permutations; // any number from 9 to 99999, 499 will be default

protected:
	// The following seven are just temporary pointers into the corresponding
	// space-time data arrays below
	double* lags;
	double*	localMoran;		// The LISA
	double* sigLocalMoran;	// The significances / pseudo p-vals
	// The significance category, generated from sigLocalMoran and
	// significance cuttoff values below.  When saving results to Table,
	// only results below significance_cuttoff are non-zero, but sigCat
	// results themeslves never change.
	//0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	int* sigCat;
	// not-sig=0 HH=1, LL=2, HL=3, LH=4, isolate=5, undef=6.  Note: value of
	// 0 never appears in cluster itself, it only appears when
	// saving results to the Table and indirectly in the map legend
	int* cluster;
	double* data1;
	double* data2;
	
public:
	std::vector<double*> lags_vecs;
	std::vector<double*> local_moran_vecs;
	std::vector<double*> sig_local_moran_vecs;
	std::vector<int*> sig_cat_vecs;
	std::vector<int*> cluster_vecs;
	std::vector<double*> data1_vecs;
	std::vector<double*> data2_vecs;
	
	const GalElement* W;
	wxString weight_name;
	bool isBivariate;
	LisaType lisa_type;
	
	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	// These two variables should be empty for LisaMapNewCanvas
	std::vector<d_array_type> data; // data[variable][time][obs]
	
	// All LisaMapNewCanvas objects synchronize themselves
	// from the following 6 variables.
	int ref_var_index;
	std::vector<GeoDaVarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	bool GetHasIsolates(int time) { return has_isolates[time]; }
	bool GetHasUndefined(int time) { return has_undefined[time]; }
	
	void registerObserver(LisaCoordinatorObserver* o);
	void removeObserver(LisaCoordinatorObserver* o);
	void notifyObservers();
	/** The list of registered observer objects. */
	std::list<LisaCoordinatorObserver*> observers;
	
	void CalcPseudoP();
	void CalcPseudoP_range(int obs_start, int obs_end);

	void InitFromVarInfo();
	void VarInfoAttributeChange();

protected:
	void DeallocateVectors();
	void AllocateVectors();
	
	void CalcPseudoP_threaded();
	void CalcLisa();
	void StandardizeData();
	std::vector<bool> has_undefined;
	std::vector<bool> has_isolates;
	bool row_standardize;
	bool calc_significances; // if false, then p-vals will never be needed
};

#endif
