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

/**
 NOTE: GStatCoordinator and GetisOrdMapView implement the
 Observable/Observer interface.  However, we have chosen not to define
 a GStatCoordinatorObserver interface for GetisOrdMapView to implement
 because GStatCoordinator needs to know more details about the
 GetisOrdMapView instances that register with it.  In particular, we only
 allow at most 8 different GetisOrdMapView instances to be observers, and
 each instance must be a different type according to the options enumerated
 in GetisOrdMapView::GMapType.
 */

#ifndef __GEODA_CENTER_G_STAT_COORDINATOR_H__
#define __GEODA_CENTER_G_STAT_COORDINATOR_H__

#include <list>
#include <vector>
#include <wx/string.h>
#include <wx/thread.h>
#include "../ShapeOperations/GalWeight.h"

class GetisOrdMapFrame;
class GStatCoordinatorObserver;
class GStatCoordinator;

class GStatWorkerThread : public wxThread
{
public:
	GStatWorkerThread(int obs_start, int obs_end,
					 GStatCoordinator* gstat_coord,
					 wxMutex* worker_list_mutex,
					 wxCondition* worker_list_empty_cond,
					 std::list<wxThread*> *worker_list,
					 int thread_id);
	virtual ~GStatWorkerThread();
	virtual void* Entry();  // thread execution starts here

	int obs_start;
	int obs_end;
	int thread_id;
	
	GStatCoordinator* gstat_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class GStatCoordinator
{
public:
	GStatCoordinator(const GalWeight* gal_weights,
					 bool row_standardize_weights,
					 const wxString& field_name, const std::vector<double>& x);
	virtual ~GStatCoordinator();
	
	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }
		
	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
	void SetSignificanceFilter(int filter_id);
	int permutations; // any number from 10 to 10000, 500 will be default
	
	double n; // # non-neighborless observations
	// # total number of observations, including neighborless observations
	long tot_obs;
	double x_star; // sum of all x_i
	double x_sstar; // sum of all (x_i)^2
	
	std::vector<double> G;
	std::vector<bool> G_defined; // check for divide-by-zero
	std::vector<double> G_star;
	
	double ExG; // same for all i since we row-standardize W
	double ExGstar; // same for all i since we row-standardize W
	double mean_x; // x hat (overall)
	double var_x; // s^2 overall
	// since W is row-standardized, VarGstar same for all i
	// same as s^2 / (n^2 mean_x ^2)
	double VarGstar;
	// since W is row-standardized, sdGstar same for all i
	double sdGstar;
	
	// z-val corresponding to each G_i
	std::vector<double> z;
	// p-val from z_i using standard normal table
	std::vector<double> p;
	
	// z-val corresponding to each G_star_i
	std::vector<double> z_star;
	// p-val from z_i^star using standard normal table
	std::vector<double> p_star;
	
	std::vector<double> pseudo_p;
	std::vector<double> pseudo_p_star;
	
	wxString weight_name;
	const GalWeight* gal_weights;
	const GalElement* W;
	wxString field_name;
	std::vector<double> x;
	
	bool GetHasIsolates() { return has_isolates; }
	bool GetHasUndefined() { return has_undefined; }
	
	void registerObserver(GetisOrdMapFrame* o);
	void removeObserver(GetisOrdMapFrame* o);
	void notifyObservers();
	/** The array of registered observer objects. */
	std::vector<GetisOrdMapFrame*> maps;	
	
	void CalcPseudoP();
	void CalcPseudoP_range(int obs_start, int obs_end);
private:
	void CalcPseudoP_threaded();
	void CalcGs();
	bool has_undefined;
	bool has_isolates;
	bool row_standardize;
};

#endif
