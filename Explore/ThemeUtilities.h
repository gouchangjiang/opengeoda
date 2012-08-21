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

#ifndef __GEODA_CENTER_THEME_UTILITIES_H__
#define __GEODA_CENTER_THEME_UTILITIES_H__

#include <vector>
#include <wx/string.h>
#include "../GenUtils.h"

class TemplateCanvas;

namespace ThemeUtilities {
	
	enum ThemeType { no_theme, hinge_15, hinge_30, quantile, percentile,
		stddev, excess_risk_theme, unique_values, natural_breaks,
		equal_intervals, lisa_categories, lisa_significance,
		getis_ord_categories, getis_ord_significance };
	
	void SetThemeCategories(int num_time_vals, int num_cats,
						const ThemeType theme, const GeoDaVarInfo& var_info,
						const std::vector<GeoDa::dbl_int_pair_vec_type>& var,
						TemplateCanvas* tc, std::vector<bool>& cats_valid,
						std::vector<wxString>& cats_error_message);
	
	void SetNaturalBreaksCats(int num_time_vals, int num_cats,
						const GeoDaVarInfo& var_info,
						const std::vector<GeoDa::dbl_int_pair_vec_type>& var,
						TemplateCanvas* tc, std::vector<bool>& cats_valid,
						int color_type = 1); // sequential by default
	
	wxString ThemeTypeToString(ThemeType theme_type);
	
	const int max_num_classes = 10;
}

#endif