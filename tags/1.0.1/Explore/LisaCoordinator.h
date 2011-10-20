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
#include <wx/string.h>
#include <wx/thread.h>
#include "../ShapeOperations/GalWeight.h"

class LisaCoordinatorObserver;
class LisaCoordinator;

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
	LisaCoordinator(const GalWeight* gal_weights,
					double* data1, double* data2,
					const wxString& field1_name,
					const wxString& field2_name,
					bool isBivariate);
	virtual ~LisaCoordinator();

	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }

	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
	void SetSignificanceFilter(int filter_id);
	int permutations; // any number from 10 to 10000, 500 will be default

	int tot_obs; // total # obs including neighborless obs

	double*	localMoran;		// The LISA
	double* sigLocalMoran;	// The significances / pseudo p-vals
	int* sigCat; // The significance category, according to cut-off
	int* cluster; // not-sig=0 HH=1, LL=2, HL=3, LH=4, isolate=5, undef=6

	wxString weight_name;
	const GalWeight* gal_weights;
	const GalElement* W;
	double* data1;
	double* data2;
	wxString field1_name;
	wxString field2_name;
	bool isBivariate;
	
	bool GetHasIsolates() { return has_isolates; }
	bool GetHasUndefined() { return has_undefined; }
	
	void registerObserver(LisaCoordinatorObserver* o);
	void removeObserver(LisaCoordinatorObserver* o);
	void notifyObservers();
	/** The list of registered observer objects. */
	std::list<LisaCoordinatorObserver*> observers;
	
	void CalcPseudoP();
	void CalcPseudoP_range(int obs_start, int obs_end);
private:
	void CalcPseudoP_threaded();
	void CalcLisa();
	void StandardizeData();
	bool has_undefined;
	bool has_isolates;
	bool row_standardize;
};

#endif
