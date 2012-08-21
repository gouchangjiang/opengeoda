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

#ifndef __GEODA_CENTER_SHP_2_GWT_H__
#define __GEODA_CENTER_SHP_2_GWT_H__

#include "GwtWeight.h"
#include "../Thiessen/VorDataType.h"
#include <wx/string.h>
#include <vector>

class GwtElement;

bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* &B, bool mean_center,
			   const wxString& boundingFileName);
bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* B, bool mean_center);

double ComputeCutOffPoint(const std::vector<double>& x,
						  const std::vector<double>& y,
						  int method, bool mean_center);

double ComputeMaxDistance(int Records, const std::vector<double>& x,
						  const std::vector<double>& y, int method);

GwtElement* DynKNN(const std::vector<double>& x, const std::vector<double>& y,
				   int k, int method);

GwtElement* shp2gwt(int Obs, std::vector<double>& x, std::vector<double>& y,
					const double threshold, const int degree,
					int method);

bool WriteGwt(const GwtElement *g,
			  const wxString& ifname, const wxString& ofname, 
			  const wxString& vname, const std::vector<wxInt64>& id_vec,
			  const int degree, bool gl);



#endif

