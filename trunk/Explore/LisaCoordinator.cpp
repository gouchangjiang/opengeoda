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
#include "../GenUtils.h"
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


LisaCoordinator::LisaCoordinator(const GalWeight* gal_weights_s,
								 double* data1_s, double* data2_s,
								 const wxString& field1_name_s,
								 const wxString& field2_name_s,
								 bool isBivariate_s)
: gal_weights(gal_weights_s),
W(gal_weights_s->gal),
weight_name(wxFileName(gal_weights_s->wflnm).GetFullName()),
field1_name(field1_name_s), field2_name(field2_name_s),
tot_obs(gal_weights_s->num_obs),
permutations(499),
isBivariate(isBivariate_s)
{	
	data1 = new double[tot_obs];
	data2 = new double[tot_obs];
	localMoran = new double[tot_obs];
	sigLocalMoran = new double[tot_obs];
	sigCat = new int[tot_obs];
	cluster = new int[tot_obs];
	for (int i=0; i<tot_obs; i++) data1[i] = data1_s[i];
	if (isBivariate) {
		for (int i=0; i<tot_obs; i++) data2[i] = data2_s[i];
	} else {
		data2 = 0;
	}
	for (int i=0; i<tot_obs; i++) {
		localMoran[i] = 0;
		sigLocalMoran[i] = 0;
		sigCat[i] = 0;
		cluster[i] = 0;
	}

	SetSignificanceFilter(1);
	StandardizeData();
	CalcLisa();
	CalcPseudoP();
}


LisaCoordinator::~LisaCoordinator()
{
	LOG_MSG("In LisaCoordinator::~LisaCoordinator");
	if (data1) delete [] data1;
	if (data2) delete [] data2;
	if (localMoran) delete [] localMoran;
	if (sigLocalMoran) delete [] sigLocalMoran;
	if (sigCat) delete [] sigCat;
	if (cluster) delete [] cluster;
}

void LisaCoordinator::StandardizeData()
{
	GenUtils::StandardizeData(tot_obs, data1);
	if (isBivariate) GenUtils::StandardizeData(tot_obs, data2);
}

void LisaCoordinator::CalcLisa()
{
	// assumes StandardizeData already called on data1 and data2
	has_undefined = false;
	has_isolates = false;
	
	GalElement* W = gal_weights->gal;
	for (int i=0; i<tot_obs; i++) {
		double Wdata = 0;
		if (isBivariate) {
			Wdata = W[i].SpatialLag(data2, true);
		} else {
			Wdata = W[i].SpatialLag(data1, true);
		}
		localMoran[i] = data1[i] * Wdata;
					
		// assign the cluster
		if (W[i].Size() > 0) {
			if (data1[i] > 0 && Wdata > 0) cluster[i] = 1;
			else if (data1[i] < 0 && Wdata < 0) cluster[i] = 2;
			else if (data1[i] > 0 && Wdata < 0) cluster[i] = 4;
			else cluster[i] = 3;
		} else {
			has_isolates = true;
			cluster[i] = 5; // neighborless
		}
	}
}

void LisaCoordinator::CalcPseudoP()
{
	LOG_MSG("Entering LisaCoordinator::CalcPseudoP");
	wxStopWatch sw;
	int nCPUs = wxThread::GetCPUCount();
	if (nCPUs <= 1) {
		LOG_MSG(wxString::Format("%d threading cores detected "
								 "so running single threaded", nCPUs));
		CalcPseudoP_range(0, tot_obs-1);
	} else {
		LOG_MSG(wxString::Format("%d threading cores detected, "
								 "running multi-threaded.", nCPUs));
		CalcPseudoP_threaded();
	}
	LOG_MSG(wxString::Format("Lisa on %d obs with %d perms took %ld ms",
							 tot_obs, permutations, sw.Time()));
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
	int work_chunk = tot_obs / nCPUs;
	int obs_start = 0;
	int obs_end = obs_start + work_chunk;
	
	bool is_thread_error = false;
	int quotient = tot_obs / nCPUs;
	int remainder = tot_obs % nCPUs;
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
		CalcPseudoP_range(0, tot_obs-1);
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
	OgSet workPermutation(tot_obs);
	Randik rng;
	int max_rand = tot_obs-1;
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
		if (permutations-countLarger < countLarger) { 
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

