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
#include <vector>

bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* &B, bool mean_center,
			   const wxString& boundingFileName);
bool ComputeXY(const wxString& fname, long* nPoints,
			   std::vector<double>& x, std::vector<double>& y,
			   myBox* B, bool mean_center);



#endif

