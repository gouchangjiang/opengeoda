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

#ifndef __GEODA_CENTER_GEODA_WEIGHTS_H__
#define __GEODA_CENTER_GEODA_WEIGHTS_H__

#include <wx/string.h>

class GeodaWeight {
public:
	GeodaWeight() : symmetry_checked(false), num_obs(0) {}
	virtual ~GeodaWeight() {}
	enum WeightType { gal_type, gwt_type };
	WeightType weight_type;
	wxString wflnm; // filename
	bool symmetry_checked; // indicates validity of is_symmetric bool
	bool is_symmetric; // true iff matrix is symmetric
	int num_obs;
	virtual bool HasIsolates() { return true; } // implement in subclasses
};

#endif

