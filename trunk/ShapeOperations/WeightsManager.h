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

#ifndef __GEODA_WEIGHTS_MANAGER_H__
#define __GEODA_WEIGHTS_MANAGER_H__

#include <vector>
class GeodaWeight;
class GalWeight;
class GwtWeight;
class GalElement;
class GwtElement;
class ProgressDlg;

/** WeightsManager is a manager for all of the currently opened weights files
 associated with a Project instance. */
class WeightsManager
{
public:	
	WeightsManager(int obs);
	virtual ~WeightsManager();
	bool clean();
	int GetNumObservations() { return observations; }
	GeodaWeight* GetWeight(int pos);
	GeodaWeight* GetCurrWeight();
	GalWeight* GetGalWeight(int pos);
	GwtWeight* GetGwtWeight(int pos);
	bool IsGalWeight(int pos);
	bool IsGwtWeight(int pos);
	bool AddWeightFile(GeodaWeight* weight, bool set_as_default = false);
	int GetNumWeights() { return num_weights; }
	wxString GetWFilename(int pos);
	int GetCurrWeightInd() { return current_weight; }
	bool SetCurrWeightInd(int pos);
	bool IsDefaultWeight() { return is_default_weight_set; }
	void SetDefaultWeight(bool s) { is_default_weight_set = s; }
	wxString GetCurrWFilename();
	wxString GetCurrWeightTitle();
	bool IsWSymmetric(int pos);
	void SetWSymmetric(int pos, bool symmetric);
	bool IsWSymmetricValid(int pos);
	void SetWSymmetricValid(int pos, bool valid);
	
	static bool CheckWeightSymmetry(GeodaWeight* w, ProgressDlg* p_dlg=0);
	static void DumpWeight(GeodaWeight* w);
	
private:
	static bool CheckGalSymmetry(GalWeight* w, ProgressDlg* p_dlg=0);
	static bool CheckGwtSymmetry(GwtWeight* w, ProgressDlg* p_dlg=0);
	static void DumpGal(GalWeight* w);
	static void DumpGwt(GwtWeight* w);
	
	std::vector<GeodaWeight*> weights;
	int num_weights; // number of non-null weights
	int current_weight; // current weight index, -1 indicates none
	bool is_default_weight_set; // true if current_weight is to be
	// used as the default weight.
	int observations;	
};

#endif
