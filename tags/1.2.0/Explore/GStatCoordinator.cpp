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

#include <boost/math/distributions/normal.hpp> // for normal_distribution
#include <algorithm>
#include <functional>
#include <map>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../GenUtils.h"
#include "../ShapeOperations/Randik.h"
#include "../logger.h"
#include "GetisOrdMapNewView.h"
#include "GStatCoordinator.h"

/*
 Example data: x = {2, 3, 3.2, 5, 8, 7}
 locations = {(10, 10), (20, 10), (40, 10), (15, 20), (30, 20), (30, 30)}
 distances:
	 0      1      2      3      4      5
 0  0.00  10.00  30.00  11.18  22.36  28.28
 1 10.00   0.00  20.00  11.18  14.14  22.36 
 2 30.00  20.00   0.00  26.93  14.14  22.36
 3 11.18  11.18  26.93   0.00  15.00  18.03
 4 22.36  14.14  14.14  15.00   0.00  10.00
 5 28.28  22.36  22.46  18.03  10.00   0.00
 
 W for threshold distance = 15
 0: 1 3
 1: 0 3 4
 2: 4
 3: 0 1 4
 4: 1 2 3 5
 5: 4
 
 W_i = [1, 1, 1, 1, 1]
 
 G_i = (lag_i)/(x_star-x_i)
  = {0.152671756, 0.198412698, 0.32, 0.186781609, 0.225247525, 0.377358491}
 
 n = 6
 ExG = E[G*_i] = 1/(n-1) = 0.2
 ExGstar = E[G_i] = 1/n = 1.6666666
 x_star = 2 + 3 + 3.2 + 5 + 8 + 7 = 28.2
 x_sstar = 2^2 + 3^2 + (3.2)^2 + 5^2 + 8^2 + 7^2 = 161.24
 x_hat = x_star/n = 28.2/6 = 4.7
 s2 = x_sstar/n - (x_hat)^2 = 4.783333
 s = sqrt(s^2) = 2.18708329
 
 x_hat_i = {5.24, 5.04, 5, 4.64, 4.04, 4.24}
 s2_i = (x_sstar-x_i^2)/(n-1) - (x_hat_i)^2
   = {3.9904, 5.0464, 5.2, 5.7184, 3.1264, 4.4704}
 Var[G_i] = (1/(n-1)^2) * (s_i/x_hat_i)^2
  = {0.00581318105, 0.00794658604, 0.00832, 0.0106242568, 0.00766199392,
      0.00994660021  }
 z_i = (G_i - E[G_i]) / sqrt(Var[G_i])
  = {-0.620745338, -0.0178061189, 1.31558703,
     -0.128241714, 0.288434967, 1.77833941}
 
 */


GStatWorkerThread::GStatWorkerThread(int obs_start_s, int obs_end_s,
								   GStatCoordinator* gstat_coord_s,
								   wxMutex* worker_list_mutex_s,
								   wxCondition* worker_list_empty_cond_s,
								   std::list<wxThread*> *worker_list_s,
								   int thread_id_s)
: wxThread(),
obs_start(obs_start_s), obs_end(obs_end_s),
gstat_coord(gstat_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

GStatWorkerThread::~GStatWorkerThread()
{
}

wxThread::ExitCode GStatWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("GStatWorkerThread %d started", thread_id));
	
	// call work for assigned range of observations
	gstat_coord->CalcPseudoP_range(obs_start, obs_end);
	
	wxMutexLocker lock(*worker_list_mutex);
	// remove ourself from the list
	worker_list->remove(this);
	// if empty, signal on empty condition since only main thread
	// should be waiting on this condition
	LOG_MSG(wxString::Format("GStatWorkerThread %d finished", thread_id));
	if (worker_list->empty()) {
		LOG_MSG("worker_list is empty, so signaling main thread");
		worker_list_empty_cond->Signal();
	}
	
	return NULL;
}


GStatCoordinator::GStatCoordinator(const GalWeight* gal_weights_s,
								   DbfGridTableBase* grid_base,
								   const std::vector<GeoDaVarInfo>& var_info_s,
								   const std::vector<int>& col_ids,
								   bool row_standardize_weights)
: W(gal_weights_s->gal),
weight_name(wxFileName(gal_weights_s->wflnm).GetName()),
row_standardize(row_standardize_weights),
num_obs(grid_base->GetNumberRows()),
permutations(499),
var_info(var_info_s),
data(var_info_s.size())
{
	SetSignificanceFilter(1);
	for (int i=0; i<var_info.size(); i++) {
		grid_base->GetColData(col_ids[i], data[i]);
	}
	InitFromVarInfo();
	
	maps.resize(8);
	for (int i=0, iend=maps.size(); i<iend; i++) {
		maps[i] = (GetisOrdMapNewFrame*) 0;
	}
}

GStatCoordinator::~GStatCoordinator()
{
	LOG_MSG("In GStatCoordinator::~GStatCoordinator");
	DeallocateVectors();
}

void GStatCoordinator::DeallocateVectors()
{
	for (int i=0; i<G_vecs.size(); i++) if (G_vecs[i]) delete [] G_vecs[i];
	G_vecs.clear();

	for (int i=0; i<G_defined_vecs.size(); i++) {
		if (G_defined_vecs[i]) delete [] G_defined_vecs[i];
	}
	G_defined_vecs.clear();

	for (int i=0; i<G_star_vecs.size(); i++) {
		if (G_star_vecs[i]) delete [] G_star_vecs[i];
	}
	G_star_vecs.clear();

	for (int i=0; i<z_vecs.size(); i++) if (z_vecs[i]) delete [] z_vecs[i];
	z_vecs.clear();

	for (int i=0; i<p_vecs.size(); i++) if (p_vecs[i]) delete [] p_vecs[i];
	p_vecs.clear();
	
	for (int i=0; i<z_star_vecs.size(); i++) {
		if (z_star_vecs[i]) delete [] z_star_vecs[i];
	}
	z_star_vecs.clear();
	
	for (int i=0; i<p_star_vecs.size(); i++) {
		if (p_star_vecs[i]) delete [] p_star_vecs[i];
	}
	p_star_vecs.clear();
	
	for (int i=0; i<pseudo_p_vecs.size(); i++) {
		if (pseudo_p_vecs[i]) delete [] pseudo_p_vecs[i];
	}
	pseudo_p_vecs.clear();
	
	for (int i=0; i<pseudo_p_star_vecs.size(); i++) {
		if (pseudo_p_star_vecs[i]) delete [] pseudo_p_star_vecs[i];
	}
	pseudo_p_star_vecs.clear();
	
	for (int i=0; i<x_vecs.size(); i++) if (x_vecs[i]) delete [] x_vecs[i];
	x_vecs.clear();
}

/** allocate based on var_info and num_time_vals **/
void GStatCoordinator::AllocateVectors()
{
	int tms = num_time_vals;
	G_vecs.resize(tms);
	G_defined_vecs.resize(tms);
	G_star_vecs.resize(tms);
	z_vecs.resize(tms);
	p_vecs.resize(tms);
	z_star_vecs.resize(tms);
	p_star_vecs.resize(tms);
	pseudo_p_vecs.resize(tms);
	pseudo_p_star_vecs.resize(tms);
	x_vecs.resize(tms);
	
	n.resize(tms, 0);
	x_star.resize(tms, 0);
	x_sstar.resize(tms, 0);
	ExG.resize(tms);
	ExGstar.resize(tms);
	mean_x.resize(tms);
	var_x.resize(tms);
	VarGstar.resize(tms);
	sdGstar.resize(tms);
	
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
	for (int i=0; i<tms; i++) {
		G_vecs[i] = new double[num_obs];
		G_defined_vecs[i] = new bool[num_obs];
		for (int j=0; j<num_obs; j++) G_defined_vecs[i][j] = true;
		G_star_vecs[i] = new double[num_obs];
		z_vecs[i] = new double[num_obs];
		p_vecs[i] = new double[num_obs];
		z_star_vecs[i] = new double[num_obs];
		p_star_vecs[i] = new double[num_obs];
		pseudo_p_vecs[i] = new double[num_obs];
		pseudo_p_star_vecs[i] = new double[num_obs];
		x_vecs[i] = new double[num_obs];
		
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
	}
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void GStatCoordinator::InitFromVarInfo()
{
	DeallocateVectors();
	
	num_time_vals = 1;
	is_any_time_variant = var_info[0].is_time_variant;
	is_any_sync_with_global_time = false;
	ref_var_index = -1;
	if (var_info[0].is_time_variant &&
		var_info[0].sync_with_global_time) {
		num_time_vals = (var_info[0].time_max - var_info[0].time_min) + 1;
		is_any_sync_with_global_time = true;
		ref_var_index = 0;
	}

	AllocateVectors();
	
	for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
		int d_t = t - var_info[0].time_min;
		for (int i=0; i<num_obs; i++) x_vecs[d_t][i] = data[0][t][i];
	}
	
	for (int t=0; t<num_time_vals; t++) {
		x = x_vecs[t];
		for (int i=0; i<num_obs; i++) {
			if ( W[i].Size() > 0 ) {
				n[t]++;
				x_star[t] += x[i];
				x_sstar[t] += x[i] * x[i];
			}
		}
		ExG[t] = 1.0/(n[t]-1); // same for all i when W is row-standardized
		ExGstar[t] = 1.0/n[t]; // same for all i when W is row-standardized
		mean_x[t] = x_star[t] / n[t]; // x hat (overall)
		var_x[t] = x_sstar[t]/n[t] - mean_x[t]*mean_x[t]; // s^2 overall
		
		// when W is row-standardized, VarGstar same for all i
		// same as s^2 / (n^2 mean_x ^2)
		VarGstar[t] = var_x[t] / (n[t]*n[t] * mean_x[t]*mean_x[t]);
		// when W is row-standardized, sdGstar same for all i
		sdGstar[t] = sqrt(VarGstar[t]);		
	}
	
	CalcGs();
	CalcPseudoP();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void GStatCoordinator::VarInfoAttributeChange()
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

/** The category vector c_val will be filled based on the current
 significance filter and significance values corresponding to specified
 canvas_time.  */
void GStatCoordinator::FillClusterCats(int canvas_time,
									   bool is_gi, bool is_perm,
									   std::vector<wxInt64>& c_val)
{
	int t = canvas_time;
	double* p_val = 0;
	if (is_gi && is_perm) p_val = pseudo_p_vecs[t];
	if (is_gi && !is_perm) p_val = p_vecs[t];
	if (!is_gi && is_perm) p_val = pseudo_p_star_vecs[t];
	if (!is_gi && !is_perm) p_val = p_star_vecs[t];
	double* z_val = is_gi ? z_vecs[t] : z_star_vecs[t];
	
	c_val.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		if (W[i].Size() == 0) {
			c_val[i] = 3; // isolate
		} else if (!G_defined_vecs[t][i]) {
			c_val[i] = 4; // undefined
		} else if (p_val[i] <= significance_cutoff) {
			c_val[i] = z_val[i] > 0 ? 1 : 2; // high = 1, low = 2
		} else {
			c_val[i] = 0; // not significant
		}
	}
}


/** Initialize Gi and Gi_star.  We handle either binary or row-standardized
 binary weights.  Weights with self-neighbors are handled correctly. */
void GStatCoordinator::CalcGs()
{
	using boost::math::normal; // typedef provides default type is double.
	// Construct a standard normal distribution std_norm_dist
	normal std_norm_dist; // default mean = zero, and s.d. = unity
	
	for (int t=0; t<num_time_vals; t++) {
		G = G_vecs[t];
		G_defined = G_defined_vecs[t];
		G_star = G_star_vecs[t];
		z = z_vecs[t];
		p = p_vecs[t];
		z_star = z_star_vecs[t];
		p_star = p_star_vecs[t];
		pseudo_p = pseudo_p_vecs[t];
		pseudo_p_star = pseudo_p_star_vecs[t];
		x = x_vecs[t];
		
		has_undefined[t] = false;
		has_isolates[t] = false;

		double n_expr = sqrt((n[t]-1)*(n[t]-1)*(n[t]-2));
		for (long i=0; i<num_obs; i++) {
			if ( W[i].size > 0 ) {
				double lag = 0;
				bool self_neighbor = false;
				for (int j=0; j<W[i].size; j++) {
					if (W[i].data[j] != i) {
						lag += x[W[i].data[j]];
					} else {
						self_neighbor = true;
					}
				}
				double Wi = self_neighbor ? W[i].size-1 : W[i].size;
				if (row_standardize) {
					lag /= W[i].size;
					Wi /= W[i].size;
				}
				double xd_i = x_star[t] - x[i];
				if (xd_i != 0) {
					G[i] = lag / xd_i;
				} else {
					G_defined[i] = false;
				}
				double x_hat_i = xd_i * ExG[t]; // (x_star - x[i])/(n-1)

				double ExGi = Wi/(n[t]-1);
				// location-specific variance
				double ss_i = ((x_sstar[t] - x[i]*x[i])/(n[t]-1)
							   - x_hat_i*x_hat_i);
				double sdG_i = sqrt(Wi*(n[t]-1-Wi)*ss_i)/(n_expr * x_hat_i);
				
				// compute z and one-sided p-val from standard-normal table
				if (G_defined[i]) {
					z[i] = (G[i] - ExGi)/sdG_i;
					if (z[i] >= 0) {
						p[i] = 1.0-cdf(std_norm_dist, z[i]);
					} else {
						p[i] = cdf(std_norm_dist, z[i]);
					}
				} else {
					has_undefined[t] = true;
				}
			} else {
				has_isolates[t] = true;
			}
		}
	
		if (x_star[t] == 0) {
			for (long i=0; i<num_obs; i++) G_defined[i] = false;
			has_undefined[t] = true;
			break;
		}
	
		if (row_standardize) {
			for (long i=0; i<num_obs; i++) {
				double lag = 0;
				bool self_neighbor = false;
				for (int j=0; j<W[i].size; j++) {
					if (W[i].data[j] == i) self_neighbor = true;
					lag += x[W[i].data[j]];
				}
				G_star[i] = self_neighbor ? lag/(W[i].size * x_star[t]) :
					(lag+x[i])/((W[i].size+1) * x_star[t]);
				z_star[i] = (G_star[i] - ExGstar[t])/sdGstar[t];
			}
		} else { // binary weights
			double n_expr_mean_x = n[t] * sqrt(n[t]-1) * mean_x[t];
			for (long i=0; i<num_obs; i++) {
				double lag = 0;
				bool self_neighbor = false;
				for (int j=0; j<W[i].size; j++) {
					if (W[i].data[j] == i) self_neighbor = true;
					lag += x[W[i].data[j]];
				}
				if (!self_neighbor) lag += x[i];
				G_star[i] = lag / x_star[t];
				double Wi = self_neighbor ? W[i].size : W[i].size+1;
				// location-specific mean
				double ExGi_star = Wi/n[t];
				// location-specific variance
				double sdG_i_star = sqrt(Wi*(n[t]-Wi)*var_x[t])/n_expr_mean_x;
				z_star[i] = (G_star[i] - ExGi_star)/sdG_i_star;
			}
		}
	
		for (long i=0; i<num_obs; i++) {
			// compute z and one-sided p-val from standard-normal table
			if (z_star[i] >= 0) {
				p_star[i] = 1.0-cdf(std_norm_dist, z_star[i]);
			} else {
				p_star[i] = cdf(std_norm_dist, z_star[i]);
			}
		}
	}
}

void GStatCoordinator::CalcPseudoP()
{
	LOG_MSG("Entering GStatCoordinator::CalcPseudoP");
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
		LOG_MSG(wxString::Format("Calculating GStat significances for time "
								 "period %d", t));

		G = G_vecs[t];
		G_defined = G_defined_vecs[t];
		G_star = G_star_vecs[t];
		z = z_vecs[t];
		p = p_vecs[t];
		z_star = z_star_vecs[t];
		p_star = p_star_vecs[t];
		pseudo_p = pseudo_p_vecs[t];
		pseudo_p_star = pseudo_p_star_vecs[t];
		x = x_vecs[t];
		x_star_t = x_star[t];
		
		if (nCPUs <= 1) {
			CalcPseudoP_range(0, num_obs-1);
		} else {
			CalcPseudoP_threaded();
		}
	}
	LOG_MSG(wxString::Format("GStat on %d obs with %d perms over %d "
							 "time periods took %ld ms",
							 num_obs, permutations, num_time_vals, sw.Time()));
	LOG_MSG("Exiting GStatCoordinator::CalcPseudoP");
}

void GStatCoordinator::CalcPseudoP_threaded()
{
	LOG_MSG("Entering GStatCoordinator::CalcPseudoP_threaded");
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
		
		GStatWorkerThread* thread =
		new GStatWorkerThread(a, b, this,
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
	
	LOG_MSG("Exiting GStatCoordinator::CalcPseudoP_threaded");
}

/** In the code that computes Gi and Gi*, we specifically checked for 
 self-neighbors and handled the situation appropriately.  For the
 permutation code, we will disallow self-neighbors. */
void GStatCoordinator::CalcPseudoP_range(int obs_start, int obs_end)
{
	OgSet workPermutation(num_obs);
	Randik rng;
	
	//const int DBGI = 4;
	//std::map<int,int> freq;
	//for (int j=0; j<num_obs; j++) freq[j] = 0;
	int max_rand = num_obs-1;
	for (long i=obs_start; i<=obs_end; i++) {
		const int numNeighsI = W[i].Size();
		const double numNeighsD = W[i].Size();
		if ( numNeighsI > 0 && G_defined[i]) { //only compute for non-isolates
			double xd_i = x_star_t - x[i]; // know != 0 since G_defined[i] true
			
			int countGLarger = 0;
			int countGStarLarger = 0;
			double permutedG = 0;
			double permutedGStar = 0;
			for (int perm=0; perm<permutations; perm++) {
				int rand = 0;
				while (rand < numNeighsI) {
					// computing 'perfect' permutation of given size
					const int  newRandom = (int) (rng.fValue() * max_rand);
					if (newRandom != i && !workPermutation.Belongs(newRandom))
					{
						//if (i == DBGI) freq[newRandom]++;
						//if (i == DBGI && perm < 20) m << newRandom << " ";
						workPermutation.Push(newRandom);
						rand++;
					}
				}
				
				double lag_i=0;
				// use permutation to compute the lags
				for (int j=0; j<numNeighsI; j++) {
					lag_i += x[workPermutation.Pop()];
				}
				
				if (row_standardize) {
					permutedG = lag_i / (numNeighsD * xd_i);
					permutedGStar = (lag_i+x[i]) / ((numNeighsD+1)*x_star_t);	
				} else { // binary weights
					// Wi = numNeighsD // assume no self-neighbors
					permutedG = lag_i / xd_i;
					permutedGStar = (lag_i+x[i]) / x_star_t;
				}
				//LOG(G[i]);
				//LOG(permutedG);
				//LOG(G_star[i]);
				//LOG(permutedGStar);
				
				//if (i == DBGI && perm < 20) {
				//	m << "Gi*: " << permutedGStar;
				//	LOG_MSG(m);
				//}
				
				if (permutedG >= G[i]) countGLarger++;
				if (permutedGStar >= G_star[i]) countGStarLarger++;
			}
			//if (i == DBGI) {
			//	for (int j=0; j<num_obs; j++) LOG(freq[j]);
			//	LOG(G_star[i]);
			//	LOG(countGStarLarger);
			//	LOG(permutations);
			//}
			// pick the smallest
			if (permutations-countGLarger < countGLarger) { 
				countGLarger=permutations-countGLarger;
			}
			pseudo_p[i] = (countGLarger + 1.0)/(permutations+1.0);
			//if (i == DBGI) LOG(pseudo_p[i]);
			
			if (permutations-countGStarLarger < countGStarLarger) { 
				countGStarLarger=permutations-countGStarLarger;
			}
			pseudo_p_star[i] = (countGStarLarger + 1.0)/(permutations+1.0);
			//if (i == DBGI) LOG(pseudo_p_star[i]);
		}
	}
}

void GStatCoordinator::SetSignificanceFilter(int filter_id)
{
	// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	if (filter_id < 1 || filter_id > 4) return;
	significance_filter = filter_id;
	if (filter_id == 1) significance_cutoff = 0.05;
	if (filter_id == 2) significance_cutoff = 0.01;
	if (filter_id == 3) significance_cutoff = 0.001;
	if (filter_id == 4) significance_cutoff = 0.0001;
}

void GStatCoordinator::registerObserver(GetisOrdMapNewFrame* o)
{
	maps[o->map_type] = o;
}

void GStatCoordinator::removeObserver(GetisOrdMapNewFrame* o)
{
	LOG_MSG("Entering GStatCoordinator::removeObserver");
	maps[o->map_type] = 0;
	int num_observers=0;
	for (int i=0, iend=maps.size(); i<iend; i++) if (maps[i]) num_observers++;
	LOG(num_observers);
	if (num_observers == 0) {
		LOG_MSG("No more observers left, so deleting self");
		delete this;
	}
	LOG_MSG("Exiting GStatCoordinator::removeObserver");
}

void GStatCoordinator::notifyObservers()
{
	for (int i=0, iend=maps.size(); i<iend; i++) {
		if (maps[i]) maps[i]->update(this);
	}
}

