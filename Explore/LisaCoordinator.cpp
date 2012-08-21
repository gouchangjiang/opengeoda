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

#include <wx/filename.h>
#include <wx/stopwatch.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/Randik.h"
#include "../logger.h"
#include "LisaCoordinatorObserver.h"
#include "LisaCoordinator.h"

LisaWorkerThread::LisaWorkerThread(int obs_start_s, int obs_end_s,
								   LisaCoordinator* lisa_coord_s,
								   wxMutex* worker_list_mutex_s,
								   wxCondition* worker_list_empty_cond_s,
								   std::list<wxThread*> *worker_list_s,
								   int thread_id_s)
: wxThread(),
obs_start(obs_start_s), obs_end(obs_end_s),
lisa_coord(lisa_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

LisaWorkerThread::~LisaWorkerThread()
{
}

wxThread::ExitCode LisaWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("LisaWorkerThread %d started", thread_id));

	// call work for assigned range of observations
	lisa_coord->CalcPseudoP_range(obs_start, obs_end);
	
	wxMutexLocker lock(*worker_list_mutex);
	// remove ourself from the list
	worker_list->remove(this);
	// if empty, signal on empty condition since only main thread
	// should be waiting on this condition
	LOG_MSG(wxString::Format("LisaWorkerThread %d finished", thread_id));
	if (worker_list->empty()) {
		LOG_MSG("worker_list is empty, so signaling main thread");
		worker_list_empty_cond->Signal();
	}
	
	return NULL;
}

/** 
 Since the user has the ability to synchronise either variable over time,
 we must be able to reapply weights and recalculate lisa values as needed.
 
 1. We will have original data as complete space-time data for both variables
 
 2. From there we will work from info in var_info for both variables.  Must
    determine number of time_steps for canvas.
 
 3. Adjust data1(2)_vecs sizes and initialize from data.
 
 3.5. Resize localMoran, sigLocalMoran, sigCat, and cluster arrays
 
 4. If rates, then calculate rates for working_data1
 
 5. Standardize working_data1 (and 2 if bivariate)
 
 6. Compute LISA for all time-stesp and save in localMoran sp/time array
 
 7. Calc Pseudo P for all time periods.  Results saved in sigLocalMoran,
    sigCat and cluster arrays
 
 8. Notify clients that values have been updated.
   
 */

LisaCoordinator::LisaCoordinator(const GalWeight* gal_weights_s,
								 DbfGridTableBase* grid_base,
								 const std::vector<GeoDaVarInfo>& var_info_s,
								 const std::vector<int>& col_ids,
								 LisaType lisa_type_s,
								 bool calc_significances_s)
: W(gal_weights_s->gal),
weight_name(wxFileName(gal_weights_s->wflnm).GetName()),
num_obs(grid_base->GetNumberRows()),
permutations(499),
lisa_type(lisa_type_s),
calc_significances(calc_significances_s),
isBivariate(lisa_type_s == bivariate),
var_info(var_info_s),
data(var_info_s.size())
{
	SetSignificanceFilter(1);
	for (int i=0; i<var_info.size(); i++) {
		grid_base->GetColData(col_ids[i], data[i]);
	}
	InitFromVarInfo();
}


LisaCoordinator::~LisaCoordinator()
{
	LOG_MSG("In LisaCoordinator::~LisaCoordinator");
	DeallocateVectors();
}

void LisaCoordinator::DeallocateVectors()
{
	for (int i=0; i<lags_vecs.size(); i++) {
		if (lags_vecs[i]) delete [] lags_vecs[i];
	}
	lags_vecs.clear();
	for (int i=0; i<local_moran_vecs.size(); i++) {
		if (local_moran_vecs[i]) delete [] local_moran_vecs[i];
	}
	local_moran_vecs.clear();
	for (int i=0; i<sig_local_moran_vecs.size(); i++) {
		if (sig_local_moran_vecs[i]) delete [] sig_local_moran_vecs[i];
	}
	sig_local_moran_vecs.clear();
	for (int i=0; i<sig_cat_vecs.size(); i++) {
		if (sig_cat_vecs[i]) delete [] sig_cat_vecs[i];
	}
	sig_cat_vecs.clear();
	for (int i=0; i<cluster_vecs.size(); i++) {
		if (cluster_vecs[i]) delete [] cluster_vecs[i];
	}
	cluster_vecs.clear();
	for (int i=0; i<data1_vecs.size(); i++) {
		if (data1_vecs[i]) delete [] data1_vecs[i];
	}
	data1_vecs.clear();
	for (int i=0; i<data2_vecs.size(); i++) {
		if (data2_vecs[i]) delete [] data2_vecs[i];
	}
	data2_vecs.clear();
}

/** allocate based on var_info and num_time_vals **/
void LisaCoordinator::AllocateVectors()
{
	int tms = num_time_vals;
	lags_vecs.resize(tms);
	local_moran_vecs.resize(tms);
	sig_local_moran_vecs.resize(tms);
	sig_cat_vecs.resize(tms);
	cluster_vecs.resize(tms);
	data1_vecs.resize(tms);
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
	for (int i=0; i<tms; i++) {
		lags_vecs[i] = new double[num_obs];
		local_moran_vecs[i] = new double[num_obs];
		if (calc_significances) {
			sig_local_moran_vecs[i] = new double[num_obs];
			sig_cat_vecs[i] = new int[num_obs];
		}
		cluster_vecs[i] = new int[num_obs];
		data1_vecs[i] = new double[num_obs];
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
	}
	
	if (lisa_type == bivariate) {
		data2_vecs.resize((var_info[1].time_max - var_info[1].time_min) + 1);
		for (int i=0; i<data2_vecs.size(); i++) {
			data2_vecs[i] = new double[num_obs];
		}
	}
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void LisaCoordinator::InitFromVarInfo()
{
	DeallocateVectors();
	
	num_time_vals = 1;
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	ref_var_index = -1;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant &&
			var_info[i].sync_with_global_time) {
			num_time_vals = (var_info[i].time_max - var_info[i].time_min) + 1;
			is_any_sync_with_global_time = true;
			ref_var_index = i;
			break;
		}
	}
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) {
			is_any_time_variant = true;
			break;
		}
	}
	
	AllocateVectors();
	
	if (lisa_type == univariate || lisa_type == bivariate) {
		for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
			int d1_t = t - var_info[0].time_min;
			for (int i=0; i<num_obs; i++) data1_vecs[d1_t][i] = data[0][t][i];
		}
		if (lisa_type == bivariate) {
			for (int t=var_info[1].time_min; t<=var_info[1].time_max; t++) {
				int d2_t = t - var_info[1].time_min;
				for (int i=0; i<num_obs; i++) {
					data2_vecs[d2_t][i] = data[1][t][i];
				}
			}
		}
	} else { // lisa_type == eb_rate_standardized
		std::vector<bool> undef_res(num_obs, false);
		double* smoothed_results = new double[num_obs];
		double* E = new double[num_obs]; // E corresponds to var_info[0]
		double* P = new double[num_obs]; // P corresponds to var_info[1]
		// we will only fill data1 for eb_rate_standardized and
		// further lisa calcs will treat as univariate
		for (int t=0; t<num_time_vals; t++) {
			int v0_t = var_info[0].time_min;
			if (var_info[0].is_time_variant &&
				var_info[0].sync_with_global_time) {
				v0_t += t;
			}
			for (int i=0; i<num_obs; i++) E[i] = data[0][v0_t][i];
			int v1_t = var_info[1].time_min;
			if (var_info[1].is_time_variant &&
				var_info[1].sync_with_global_time) {
				v1_t += t;
			}
			for (int i=0; i<num_obs; i++) P[i] = data[1][v1_t][i];
			bool success = GeoDaAlgs::RateStandardizeEB(num_obs, P, E,
														smoothed_results,
														undef_res);
			if (!success) {
				map_valid[t] = false;
				map_error_message[t] << "Emprical Bayes Rate ";
				map_error_message[t] << "Standardization failed.";
			} else {
				for (int i=0; i<num_obs; i++) {
					data1_vecs[t][i] = smoothed_results[i];
				}
			}
		}
		if (smoothed_results) delete [] smoothed_results;
		if (E) delete [] E;
		if (P) delete [] P;
	}
	
	StandardizeData();
	CalcLisa();
	if (calc_significances) CalcPseudoP();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void LisaCoordinator::VarInfoAttributeChange()
{
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	//GeoDa::PrintVarInfoVector(var_info);
}

void LisaCoordinator::StandardizeData()
{
	for (int t=0; t<data1_vecs.size(); t++) {
		GenUtils::StandardizeData(num_obs, data1_vecs[t]);
	}
	if (isBivariate) {
		for (int t=0; t<data2_vecs.size(); t++) {
			GenUtils::StandardizeData(num_obs, data2_vecs[t]);
		}
	}
}

/** assumes StandardizeData already called on data1 and data2 */
void LisaCoordinator::CalcLisa()
{
	for (int t=0; t<num_time_vals; t++) {
		data1 = data1_vecs[t];
		if (isBivariate) {
			data2 = data2_vecs[0];
			if (var_info[1].is_time_variant &&
				var_info[1].sync_with_global_time) data2 = data2_vecs[t];
		}
		lags = lags_vecs[t];
		localMoran = local_moran_vecs[t];
		cluster = cluster_vecs[t];
	
		has_undefined[t] = false;
		has_isolates[t] = false;
	
		for (int i=0; i<num_obs; i++) {
			double Wdata = 0;
			if (isBivariate) {
				Wdata = W[i].SpatialLag(data2, true);
			} else {
				Wdata = W[i].SpatialLag(data1, true);
			}
			lags[i] = Wdata;
			localMoran[i] = data1[i] * Wdata;
					
			// assign the cluster
			if (W[i].Size() > 0) {
				if (data1[i] > 0 && Wdata < 0) cluster[i] = 4;
				else if (data1[i] < 0 && Wdata > 0) cluster[i] = 3;
				else if (data1[i] < 0 && Wdata < 0) cluster[i] = 2;
				else cluster[i] = 1; //data1[i] > 0 && Wdata > 0
			} else {
				has_isolates[t] = true;
				cluster[i] = 5; // neighborless
			}
		}
	}
}

void LisaCoordinator::CalcPseudoP()
{
	LOG_MSG("Entering LisaCoordinator::CalcPseudoP");
	if (!calc_significances) return;
	wxStopWatch sw;
	int nCPUs = wxThread::GetCPUCount();
	
	// To ensure thread safety, only work on one time slice of data
	// at a time.  For each time period t:
	// 1. copy data for time period t into data1 and data2 arrays
	// 2. Perform multi-threaded computation
	// 3. copy results into results array
	
	if (nCPUs <= 1) {
		LOG_MSG(wxString::Format("%d threading cores detected "
								 "so running single threaded", nCPUs));
	} else {
		LOG_MSG(wxString::Format("%d threading cores detected, "
								 "running multi-threaded.", nCPUs));
	}
	
	for (int t=0; t<num_time_vals; t++) {
		LOG_MSG(wxString::Format("Calculating LISA significances for time "
								 "period %d", t));
		
		data1 = data1_vecs[t];
		if (isBivariate) {
			data2 = data2_vecs[0];
			if (var_info[1].is_time_variant &&
				var_info[1].sync_with_global_time) data2 = data2_vecs[t];
		}
		lags = lags_vecs[t];
		localMoran = local_moran_vecs[t];
		sigLocalMoran = sig_local_moran_vecs[t];
		sigCat = sig_cat_vecs[t];
		cluster = cluster_vecs[t];
		
		if (nCPUs <= 1) {
			CalcPseudoP_range(0, num_obs-1);
		} else {
			CalcPseudoP_threaded();
		}
	}
	LOG_MSG(wxString::Format("LISA on %d obs with %d perms over %d "
							 "time periods took %ld ms",
							 num_obs, permutations, num_time_vals, sw.Time()));
	LOG_MSG("Exiting LisaCoordinator::CalcPseudoP");
}

void LisaCoordinator::CalcPseudoP_threaded()
{
	LOG_MSG("Entering LisaCoordinator::CalcPseudoP_threaded");
	int nCPUs = wxThread::GetCPUCount();
	
	// mutext protects access to the worker_list
    wxMutex worker_list_mutex;
	// signals that worker_list is empty
	wxCondition worker_list_empty_cond(worker_list_mutex);
	worker_list_mutex.Lock(); // mutex should be initially locked
	
    // List of all the threads currently alive.  As soon as the thread
	// terminates, it removes itself from the list.
	std::list<wxThread*> worker_list;
	
	// divide up work according to number of observations
	// and number of CPUs
	int work_chunk = num_obs / nCPUs;
	int obs_start = 0;
	int obs_end = obs_start + work_chunk;
	
	bool is_thread_error = false;
	int quotient = num_obs / nCPUs;
	int remainder = num_obs % nCPUs;
	int tot_threads = (quotient > 0) ? nCPUs : remainder;
	
	for (int i=0; i<tot_threads && !is_thread_error; i++) {
		int a=0;
		int b=0;
		if (i < remainder) {
			a = i*(quotient+1);
			b = a+quotient;
		} else {
			a = remainder*(quotient+1) + (i-remainder)*quotient;
			b = a+quotient-1;
		}
		int thread_id = i+1;
		LOG_MSG(wxString::Format("thread %d: %d->%d", thread_id, a, b));
		
		LisaWorkerThread* thread =
			new LisaWorkerThread(a, b, this,
								 &worker_list_mutex,
								 &worker_list_empty_cond,
								 &worker_list, thread_id);
		if ( thread->Create() != wxTHREAD_NO_ERROR ) {
			LOG_MSG("Error: Can't create thread!");
			delete thread;
			is_thread_error = true;
		} else {
			worker_list.push_front(thread);
		}
	}
	if (is_thread_error) {
		LOG_MSG("Error: Could not spawn a worker thread, falling back "
				"to single-threaded pseudo-p calculation.");
		// fall back to single thread calculation mode
		CalcPseudoP_range(0, num_obs-1);
	} else {
		LOG_MSG("Starting all worker threads");
		std::list<wxThread*>::iterator it;
		for (it = worker_list.begin(); it != worker_list.end(); it++) {
			(*it)->Run();
		}
	
		while (!worker_list.empty()) {
			// wait until thread_list might be empty
			worker_list_empty_cond.Wait();
			// We have been woken up. If this was not a false
			// alarm (sprious signal), the loop will exit.
			LOG_MSG("work_list_empty_cond signaled");
		}
		LOG_MSG("All worker threads exited");
	}

	LOG_MSG("Exiting LisaCoordinator::CalcPseudoP_threaded");
}

void LisaCoordinator::CalcPseudoP_range(int obs_start, int obs_end)
{
	OgSet workPermutation(num_obs);
	Randik rng;
	int max_rand = num_obs-1;
	for (int cnt=obs_start; cnt<=obs_end; cnt++) {
		const int numNeighbors = W[cnt].Size();
		
		int countLarger = 0;
		for (int perm=0; perm<permutations; perm++) {
			int rand=0;
			while (rand < numNeighbors) {
				// computing 'perfect' permutation of given size
				int newRandom = (int) (rng.fValue() * max_rand);
				//int newRandom = X(rng);
				if (newRandom != cnt && !workPermutation.Belongs(newRandom))
				{
					workPermutation.Push(newRandom);
					rand++;
				}
			}
			double permutedLag=0;
			// use permutation to compute the lag
			// compute the lag for binary weights
			if (isBivariate) {
				for (int cp=0; cp<numNeighbors; cp++) {
					permutedLag += data2[workPermutation.Pop()];
				}
			} else {
				for (int cp=0; cp<numNeighbors; cp++) {
					permutedLag += data1[workPermutation.Pop()];
				}
			}
			
			//NOTE: we shouldn't have to row-standardize or
			// multiply by data1[cnt]
			if (numNeighbors) permutedLag /= numNeighbors;
			const double localMoranPermuted = permutedLag * data1[cnt];
			if (localMoranPermuted >= localMoran[cnt]) countLarger++;
		}
		// pick the smallest
		if (permutations-countLarger <= countLarger) { 
			countLarger = permutations-countLarger;
		}
		
		sigLocalMoran[cnt] = (countLarger+1.0)/(permutations+1);
		// 'significance' of local Moran
		if (sigLocalMoran[cnt] <= 0.0001) sigCat[cnt] = 4;
		else if (sigLocalMoran[cnt] <= 0.001) sigCat[cnt] = 3;
		else if (sigLocalMoran[cnt] <= 0.01) sigCat[cnt] = 2;
		else if (sigLocalMoran[cnt] <= 0.05) sigCat[cnt]= 1;
		else sigCat[cnt]= 0;
		
		// observations with no neighbors get marked as isolates
		if (numNeighbors == 0) {
			sigCat[cnt] = 5;
		}
	}
}

void LisaCoordinator::SetSignificanceFilter(int filter_id)
{
	// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	if (filter_id < 1 || filter_id > 4) return;
	significance_filter = filter_id;
	if (filter_id == 1) significance_cutoff = 0.05;
	if (filter_id == 2) significance_cutoff = 0.01;
	if (filter_id == 3) significance_cutoff = 0.001;
	if (filter_id == 4) significance_cutoff = 0.0001;
}

void LisaCoordinator::registerObserver(LisaCoordinatorObserver* o)
{
	observers.push_front(o);
}

void LisaCoordinator::removeObserver(LisaCoordinatorObserver* o)
{
	LOG_MSG("Entering LisaCoordinator::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0) {
		LOG_MSG("No more observers left so deleting self.");
		delete this;
	}
	LOG_MSG("Exiting LisaCoordinator::removeObserver");
}

void LisaCoordinator::notifyObservers()
{
	std::list<LisaCoordinatorObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		(*it)->update(this);
	}
}

