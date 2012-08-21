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

#include <set>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "../logger.h"
#include "../GeoDaConst.h"
#include "../TemplateCanvas.h"
#include "ThemeUtilities.h"

struct UniqueValElem {
	UniqueValElem(double v, int f, int l): val(v), first(f), last(l) {}
	double val; // value
	int first; // index of first occurrance
	int last; // index of last occurrance
};

/** clears uv_mapping and resizes as needed */
void create_unique_val_mapping(std::vector<UniqueValElem>& uv_mapping,
							   const std::vector<double>&v)
{
	uv_mapping.clear();
	uv_mapping.push_back(UniqueValElem(v[0], 0, 0));
	int cur_ind = 0;
	for (int i=0, iend=v.size(); i<iend; i++) {
		if (uv_mapping[cur_ind].val != v[i]) {
			uv_mapping[cur_ind].last = i-1;
			cur_ind++;
			uv_mapping.push_back(UniqueValElem(v[i], i, i));
		}
	}
}

/** Assume that b.size() <= N-1 */
void pick_rand_breaks(std::vector<int>& b, int N)
{
	int num_breaks = b.size();
	if (num_breaks > N-1) return;
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::uniform_01<boost::mt19937> X(rng);
	
	std::set<int> s;
	while (s.size() != num_breaks) s.insert(1 + (N-1)*X());
	int cnt=0;
	for (std::set<int>::iterator it=s.begin(); it != s.end(); it++) {
		b[cnt++] = *it;
	}
	std::sort(b.begin(), b.end());
}

// translate unique value breaks into normal breaks given unique value mapping
void unique_to_normal_breaks(const std::vector<int>& u_val_breaks,
							 const std::vector<UniqueValElem>& u_val_mapping,
							 std::vector<int>& n_breaks)
{
	if (n_breaks.size() != u_val_breaks.size()) {
		n_breaks.resize(u_val_breaks.size());
	}
	for (int i=0, iend=u_val_breaks.size(); i<iend; i++) {
		n_breaks[i] = u_val_mapping[u_val_breaks[i]].first;
	}	
}

/** Assume input b and v is sorted.  If not, can sort
 with std::sort(v.begin(), v.end())
 We assume that b and v are sorted in ascending order and are
 valid (ie, no break indicies out of range and all categories
 have at least one value.
 gssd is the global sum of squared differences from the mean */
double calc_gvf(const std::vector<int>& b, const std::vector<double>& v,
				double gssd)
{
	int N = v.size();
	int num_cats = b.size()+1;
	double tssd=0; // total sum of local sums of squared differences
	for (int i=0; i<num_cats; i++) {
		int s = (i == 0) ? 0 : b[i-1];
		int t = (i == num_cats-1) ? N : b[i];
		
		double m=0; // local mean
		double ssd=0; // local sum of squared differences (variance)
		for (int j=s; j<t; j++) m += v[j];
		m /= ((double) t-s);
		for (int j=s; j<t; j++) ssd += (v[j]-m)*(v[j]-m);
		tssd += ssd;
	}
	
	return 1-(tssd/gssd);
}

/** Update Categories based on num_cats and num_time_vals.
 	var is assumed to be sorted.
	num_cats is only used by themes where the user enters the number of
    categories.  Note: LISA and Getis-Ord map themes are not supported
    by this function.
 */
void ThemeUtilities::SetThemeCategories(int num_time_vals, int num_cats,
						const ThemeType theme, const GeoDaVarInfo& var_info,
						const std::vector<GeoDa::dbl_int_pair_vec_type>& var,
						TemplateCanvas* tc,
						std::vector<bool>& cats_valid,
						std::vector<wxString>& cats_error_message)
{
	int num_obs = var[0].size();
	if (theme == ThemeUtilities::no_theme) {
		tc->CreateCategoriesAllCanvasTms(1, num_time_vals); // 1 = #cats
		for (int t=0; t<num_time_vals; t++) {
			tc->SetCategoryColor(t, 0, GeoDaConst::map_default_fill_colour);
		}
	} else if (theme == ThemeUtilities::quantile) {
		// user supplied number of categories
		tc->CreateCategoriesAllCanvasTms(num_cats, num_time_vals);
		tc->SetCategoryBrushesAllCanvasTms(1, num_cats, false);
	} else if (theme == ThemeUtilities::unique_values) {
		// number of categories based on number of unique values in data
		tc->CreateEmptyCategories(num_time_vals);
	} else if (theme == ThemeUtilities::natural_breaks) {
		// user supplied number of categories
		tc->CreateEmptyCategories(num_time_vals);
		// if there are fewer unique values than number of categories,
		// we will automatically reduce the number of categories to the
		// number of unique values.
	} else if (theme == ThemeUtilities::equal_intervals) {
		// user supplied number of categories
		tc->CreateEmptyCategories(num_time_vals);
		// if there is only one value, then we automatically reduce
		// the number of categories down to one.
	} else if (theme == ThemeUtilities::percentile ||
			   theme == ThemeUtilities::hinge_15 ||
			   theme == ThemeUtilities::hinge_30 ||
			   theme == ThemeUtilities::stddev ||
			   theme == ThemeUtilities::excess_risk_theme) {
		num_cats = 6;
		tc->CreateCategoriesAllCanvasTms(num_cats, num_time_vals);
		tc->SetCategoryBrushesAllCanvasTms(2, num_cats, false);
	}
		
	if (theme == ThemeUtilities::no_theme) {
		for (int t=0; t<num_time_vals; t++) {
			tc->SetCategoryLabel(t, 0, "");
			for (int i=0; i<num_obs; i++) tc->AppendIdToCategory(t, 0, i);
		}
		return;
	}
	
	if (num_cats > num_obs) {
		for (int t=0; t<num_time_vals; t++) {
			cats_valid[t] = false;
			cats_error_message[t] << "Error: Chosen theme requires more ";
			cats_error_message[t] << " cateogries than observations.";
		}
	} else if (theme == hinge_15 || theme == hinge_30) {
		std::vector<HingeStats> hinge_stats(num_time_vals);
		tc->SetCategoryBrushesAllCanvasTms(2, num_cats, false);
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			
			hinge_stats[t].CalculateHingeStats(var[t]);
			double extreme_lower = hinge_stats[t].extreme_lower_val_15;
			double extreme_upper = hinge_stats[t].extreme_upper_val_15;
			if (theme == hinge_30) {
				extreme_lower = hinge_stats[t].extreme_lower_val_30;
				extreme_upper = hinge_stats[t].extreme_upper_val_30;	
			}
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
				int cat_num = 0;
				if (val < extreme_lower) {
					cat_num = 0;
				} else if (val < hinge_stats[t].Q1) {
					cat_num = 1;
				} else if (val <= hinge_stats[t].Q2) {
					cat_num = 2;
				} else if (val <= hinge_stats[t].Q3) {
					cat_num = 3;
				} else if (val <= extreme_upper) {
					cat_num = 4;
				} else { // val > extreme_upper
					cat_num = 5;
				}
				tc->AppendIdToCategory(t, cat_num, ind);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "Lower outlier";
			labels[1] << "< 25%";
			labels[2] << "25% - 50%";
			labels[3] << "50% - 75%";
			labels[4] << "> 75%";
			labels[5] << "Upper outlier";
			for (int cat=0; cat<num_cats; cat++) {
				labels[cat] << " (" << tc->GetNumObsInCategory(t, cat) << ")";
				tc->SetCategoryLabel(t, cat, labels[cat]);
			}
		}
	} else if (theme == quantile) {
		if (num_cats == 1) {
			for (int t=0; t<num_time_vals; t++) {
				if (!cats_valid[t]) continue;
				for (int i=0, iend=var[t].size(); i<iend; i++) {
					tc->AppendIdToCategory(t, 0, var[t][i].second);
				}
				double low_v = var[t][0].first;
				double high_v = var[t][num_obs - 1].first;
				wxString s = wxString::Format("[%.4g:%.4g] (%d)",
											  low_v, high_v, num_obs);
				tc->SetCategoryLabel(t, 0, s);
			}
		} else {
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			std::vector<double> breaks(num_cats-1);
			int num_breaks = breaks.size();
			int num_breaks_lower = num_breaks/2;
			
			for (int t=0; t<num_time_vals; t++) {
				if (!cats_valid[t]) continue;
				for (int i=0, iend=breaks.size(); i<iend; i++) {
					breaks[i] = GeoDa::percentile(((i+1.0)*100.0)/
												  ((double) num_cats),
												  var[t]);
				}
				// Set default cat_min / cat_max values for when
				// category size is 0
				cat_min[0] = var[t][0].first;
				cat_max[0] = breaks[0];
				for (int i=1, iend=breaks.size(); i<iend; i++) {
					cat_min[i] = breaks[i-1];
					cat_max[i] = breaks[i];
				}
				cat_min[num_breaks] = breaks[num_breaks-1];
				cat_max[num_breaks] = var[t][num_obs-1].first;
				double val;
				int ind;
				for (int i=0, iend=var[t].size(); i<iend; i++) {
					val = var[t][i].first;
					ind = var[t][i].second;
					bool found = false;
					int cat = num_breaks; // last cat by default
					for (int j=0; j<num_breaks_lower; j++) {
						if (val < breaks[j]) {
							found = true;
							cat = j;
							break;
						}
					}
					if (!found) {
						for (int j=num_breaks_lower; j<num_breaks; j++) {
							if (val <= breaks[j]) {
								cat = j;
								break;
							}
						}
					}
					if (tc->GetNumObsInCategory(t, cat) == 0) {
						cat_min[cat] = val;
						cat_max[cat] = val;
					} else {
						if (val < cat_min[cat]) {
							cat_min[cat] = val;
						} else if (val > cat_max[cat]) {
							cat_max[cat] = val;
						}
					}
					tc->AppendIdToCategory(t, cat, ind);
				}
				wxString s;
				s = wxString::Format("[%.4g:%.4g] (%d)",
									 cat_min[0], cat_max[0],
									 tc->GetNumObsInCategory(t, 0));
				tc->SetCategoryLabel(t, 0, s);
				for (int i=1, iend=breaks.size(); i<iend; i++) {
					s = wxString::Format("[%.4g:%.4g] (%d)",
										 cat_min[i], cat_max[i],
										 tc->GetNumObsInCategory(t, i));
					tc->SetCategoryLabel(t, i, s);
				}
				s = wxString::Format("[%.4g:%.4g] (%d)",
									 cat_min[num_breaks], cat_max[num_breaks],
									 tc->GetNumObsInCategory(t, num_breaks));
				tc->SetCategoryLabel(t, num_breaks, s);
			}
		}
	}
	//else if (theme == quantile) {
	//	// size of each category
	//	std::vector<int> cat_size(num_cats);
	//	// first index of each category
	//	std::vector<int> cat_start(num_cats);
    //
	//	int quotient = num_obs / num_cats;
	//	int remainder = num_obs % num_cats;
	//	for (int i=0; i<num_cats; i++) cat_size[i] = quotient;
	//	for (int i=0; i<remainder; i++) cat_size[(num_cats-1)-i]++;
	//	for (int i=0, accum=0; i<num_cats; i++) {
	//		cat_start[i] = accum;
	//		accum += cat_size[i];
	//	}
	//	
	//	for (int t=0; t<num_time_vals; t++) {
	//		for (int i=1; i<num_cats && cats_valid[t]; i++) {
	//			if (var[t][cat_start[i]-1].first ==
	//				var[t][cat_start[i]].first) {
	//				cats_valid[t] = false;
	//				wxString s;
	//				double dub_val = var[t][cat_start[i]].first;
	//				s << "Error: A valid Quantile map cannot be created for ";
	//				s << " this field with " << num_cats << "\n";
	//				s << "categories since a duplicate value " << dub_val;
	//				s << " would occur in categories " << i-1 << " and ";
	//				s << i+1 << "\n.Try a different number of categories, or ";
	//				s << " choose a different map theme.";
	//				cats_error_message[t] = s;
	//			}
	//		}
	//		
	//		if (cats_valid[t]) {
	//			for (int i=0; i<num_cats; i++) {
	//				for (int j=cat_start[i], jend=cat_start[i]+cat_size[i];
	//					 j<jend; j++) {
	//				tc->AppendIdToCategory(t, i, var[t][j].second);
	//				}
	//				double low_v = var[t][cat_start[i]].first;
	//				double high_v = var[t][cat_start[i] + 
	//												   cat_size[i] - 1].first;
	//				wxString s = wxString::Format("[%.4g:%.4g] (%d)",
	//											  low_v, high_v, cat_size[i]);
	//				tc->SetCategoryLabel(t, i, s);
	//			}
	//		}
	//	}
	//}
	else if (theme == percentile) {
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			
			double p_1 = GeoDa::percentile(1, var[t]);
			double p_10 = GeoDa::percentile(10, var[t]);
			double p_50 = GeoDa::percentile(50, var[t]);
			double p_90 = GeoDa::percentile(90, var[t]);
			double p_99 = GeoDa::percentile(99, var[t]);
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
				int cat_num = 0;
				if (val < p_1) {
					cat_num = 0;
				} else if (val < p_10) {
					cat_num = 1;
				} else if (val <= p_50) {
					cat_num = 2;
				} else if (val <= p_90) {
					cat_num = 3;
				} else if (val <= p_99) {
					cat_num = 4;
				} else { // val > p_99
					cat_num = 5;
				}
				tc->AppendIdToCategory(t, cat_num, ind);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "< 1%";       // < 1%
			labels[1] << "1% - 10%";   // >= 1% && < 10%
			labels[2] << "10% - 50%";  // >= 10% && <= 50%
			labels[3] << "50% - 90%";  // > 50% && <= 90%
			labels[4] << "90% - 99%";  // > 90% && <= 99%
			labels[5] << "> 99%";      // > 99%
			for (int cat=0; cat<num_cats; cat++) {
				labels[cat] << " (" << tc->GetNumObsInCategory(t, cat) << ")";
				tc->SetCategoryLabel(t, cat, labels[cat]);
			}
		}
	} else if (theme == stddev) {
		std::vector<double> v(num_obs);
		SampleStatistics stats;
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			
			for (int i=0; i<num_obs; i++) v[i] = var[t][i].first;
			stats.CalculateFromSample(v);
			
			double SDm2 = stats.mean - 2.0 * stats.sd_with_bessel;
			double SDm1 = stats.mean - 1.0 * stats.sd_with_bessel;
			double mean = stats.mean;
			double SDp1 = stats.mean + 1.0 * stats.sd_with_bessel;
			double SDp2 = stats.mean + 2.0 * stats.sd_with_bessel;
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
				int cat_num = 0;
				if (val < SDm2) {
					cat_num = 0;
				} else if (val < SDm1) {
					cat_num = 1;
				} else if (val <= mean) {
					cat_num = 2;
				} else if (val <= SDp1) {
					cat_num = 3;
				} else if (val <= SDp2) {
					cat_num = 4;
				} else { // val > SDp2
					cat_num = 5;
				}
				tc->AppendIdToCategory(t, cat_num, ind);
			}
			std::vector<wxString> labels(num_cats);
			// < -2 sd
			labels[0] << "< " << GenUtils::DblToStr(SDm2);
			// >= -2 sd && < -1 sd
			labels[1] << GenUtils::DblToStr(SDm2) << " - ";
			labels[1] << GenUtils::DblToStr(SDm1);
			// >= -1 sd && <= mean
			labels[2] << GenUtils::DblToStr(SDm1) << " - " << mean;
			// > mean && <= 1 sd
			labels[3] << mean << " - " << GenUtils::DblToStr(SDp1);
			// > 1 sd && <= 2 sd
			labels[4] << GenUtils::DblToStr(SDp1) << " - ";
			labels[4] << GenUtils::DblToStr(SDp2);
			// > 2 sd
			labels[5] << "> " << GenUtils::DblToStr(SDp2);
			for (int cat=0; cat<num_cats; cat++) {
				labels[cat] << " (" << tc->GetNumObsInCategory(t, cat) << ")";
				tc->SetCategoryLabel(t, cat, labels[cat]);
			}
		}
	} else if (theme == unique_values) {
		// The Unique Values theme is somewhat different from the other themes
		// in that we calculate the number from the data itself.  We support
		// at most 10 unique values.
		
		std::vector< std::vector<double> > u_vals_map(num_time_vals);
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			u_vals_map[t].push_back(var[t][0].first);
			for (int i=0; i<num_obs; i++) {
				if (u_vals_map[t][u_vals_map[t].size()-1] !=
					var[t][i].first)
				{
					u_vals_map[t].push_back(var[t][i].first);
				}
			}
		}
		
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			if (u_vals_map[t].size() > max_num_classes) {
				// automatically use Natural Breaks when number of
				// unique values exceeds max_num_classes.  This will avoid
				// all error messages.
				SetNaturalBreaksCats(num_time_vals, max_num_classes,
									 var_info, var, tc, cats_valid, 3);
				return;
				
				// old code to print an error message when there number
				// of unique values exceeds max_num_classes.  This was
				// the only category that could generate an error message
				// other than the extreme case of there being fewer
				// observations than required categories.
				/*
				cats_valid[t] = false;
				wxString s;
				s << "Error: Unique Values Maps can display at most ";
				s << "10 unique values, but " << var_info.name;
				if (var_info.is_time_variant) {
					s << " (time period " << var_info.time_min + t + 1 << ")";
				}
				s << " has " << u_vals_map[t].size() << " different values.";
				cats_error_message[t] = s;
				 */
			} else {
				tc->SetCategoryBrushesAtCanvasTm(3, u_vals_map[t].size(),
												 false, t);
			}
		}
		
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			int t_num_cats = u_vals_map[t].size();
			int cur_cat = 0;
			for (int i=0; i<num_obs; i++) {
				if (u_vals_map[t][cur_cat] 
					!= var[t][i].first) cur_cat++;
				tc->AppendIdToCategory(t, cur_cat, var[t][i].second);
			}
			std::vector<wxString> labels(t_num_cats);
			for (int cat=0; cat<t_num_cats; cat++) {
				labels[cat] << u_vals_map[t][cat] << " (";
				labels[cat] << tc->GetNumObsInCategory(t, cat) << ")";
				tc->SetCategoryLabel(t, cat, labels[cat]);
			}
		}
	} else if (theme == natural_breaks) {
		SetNaturalBreaksCats(num_time_vals, num_cats, var_info, var,
							 tc, cats_valid);
	} else if (theme == equal_intervals) {
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			double min_val = var[t][0].first;
			double max_val = var[t][0].first;
			for (int i=0; i<num_obs; i++) {
				double val = var[t][i].first;
				if (val < min_val) {
					min_val = val;
				} else if (val > max_val) {
					max_val = val;
				}
			}
			
			if (min_val == max_val || num_cats == 1) {
				// Create just one category and continue
				tc->SetCategoryBrushesAtCanvasTm(1, 1, false, t);
				for (int i=0; i<num_obs; i++) {
					tc->AppendIdToCategory(t, 0, i);
				}
				wxString l;
				l << "[" << min_val << ":" << max_val << "] ";
				l << "(" << num_obs << ")";
				tc->SetCategoryLabel(t, 0, l);
				continue;
			}
			
			// we know that num_cats >= 2 and <= 10
			tc->SetCategoryBrushesAtCanvasTm(1, num_cats, false, t);
			
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			double range = max_val - min_val;
			double delta = range / (double) num_cats;
			for (int i=0; i<num_cats; i++) {
				cat_min[i] = min_val + ((double) i)*delta;
				cat_max[i] = min_val + (((double) i) + 1.0)*delta;
			}
			
			int last_cat = num_cats - 1;
			for (int i=0; i<num_obs; i++) {
				double val = var[t][i].first;
				int ind = var[t][i].second;
				int cat_num = last_cat; // last cat by default
				
				for (int j=0; j<num_cats && cat_num == last_cat; j++) {
					if (val >= cat_min[j] && val < cat_max[j]) cat_num = j;
				}
				tc->AppendIdToCategory(t, cat_num,
									   var[t][i].second);
			}
			
			for (int i=0; i<num_cats; i++) {
				wxString l;
				l << "[" << cat_min[i] << ":" << cat_max[i] << "] ";
				l << "(" << tc->GetNumObsInCategory(t, i) << ")";
				tc->SetCategoryLabel(t, i, l);
			}
		}
	} else if (theme == excess_risk_theme) {
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t]) continue;
			
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
				int cat_num = 0;
				if (val < 0.25) {
					cat_num = 0;
				} else if (val < 0.50) {
					cat_num = 1;
				} else if (val <= 1.00) {
					cat_num = 2;
				} else if (val <= 2.00) {
					cat_num = 3;
				} else if (val <= 4.00) {
					cat_num = 4;
				} else { // val > 4.00
					cat_num = 5;
				}
				tc->AppendIdToCategory(t, cat_num, ind);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "< 0.25";       // < 0.25
			labels[1] << "0.25 - 0.50";  // >= 0.25 && < 0.50
			labels[2] << "0.50 - 1.00";  // >= 0.50 && <= 1.00
			labels[3] << "1.00 - 2.00";  // > 1.00 && <= 2.00
			labels[4] << "2.00 - 4.00";  // > 2.00 && <= 4.00
			labels[5] << "> 4.00";       // > 4.00
			for (int cat=0; cat<num_cats; cat++) {
				labels[cat] << " (" << tc->GetNumObsInCategory(t, cat) << ")";
				tc->SetCategoryLabel(t, cat, labels[cat]);
			}
		}
	} else {
		for (int t=0; t<num_time_vals; t++) {
			cats_valid[t] = false;
			cats_error_message[t] = "Theme Not Implemented";
		}
	}
}

void ThemeUtilities::SetNaturalBreaksCats(int num_time_vals, int num_cats,
						const GeoDaVarInfo& var_info,
						const std::vector<GeoDa::dbl_int_pair_vec_type>& var,
						TemplateCanvas* tc, std::vector<bool>& cats_valid,
						int color_type)
{
	int num_obs = var[0].size();
	// user supplied number of categories
	tc->CreateEmptyCategories(num_time_vals);
	// if there are fewer unique values than number of categories,
	// we will automatically reduce the number of categories to the
	// number of unique values.
	
	std::vector<double> v(num_obs);
	for (int t=0; t<num_time_vals; t++) {
		for (int i=0; i<num_obs; i++) v[i] = var[t][i].first;
		if (!cats_valid[t]) continue;
		std::vector<UniqueValElem> uv_mapping;
		create_unique_val_mapping(uv_mapping, v);
		int num_unique_vals = uv_mapping.size();
		int t_cats = GenUtils::min<int>(num_unique_vals, num_cats);
		
		double mean = 0;
		for (int i=0; i<num_obs; i++) mean += v[i];
		mean /= (double) num_obs;
		double gssd = 0;
		for (int i=0; i<num_obs; i++) gssd += (v[i]-mean)*(v[i]-mean);			
		
		std::vector<int> rand_b(t_cats-1);
		std::vector<int> best_breaks(t_cats-1);
		std::vector<int> uv_rand_b(t_cats-1);
		double max_gvf_found = 0;
		int max_gvf_ind = 0;
		// for 5000 permutations, 2200 obs, and 4 time periods, slow enough
		// make sure permutations is such that this total is not exceeded.
		double c = 5000*2200*4;
		int perms = c / ((double) num_time_vals * (double) num_obs);
		if (perms < 10) perms = 10;
		if (perms > 10000) perms = 10000;
		
		for (int i=0; i<perms; i++) {
			pick_rand_breaks(uv_rand_b, num_unique_vals);
			// translate uv_rand_b into normal breaks
			unique_to_normal_breaks(uv_rand_b, uv_mapping, rand_b);
			double new_gvf = calc_gvf(rand_b, v, gssd);
			if (new_gvf > max_gvf_found) {
				max_gvf_found = new_gvf;
				max_gvf_ind = i;
				best_breaks = rand_b;
			}
		}
		LOG(perms);
		LOG(max_gvf_ind);
		LOG(max_gvf_found);
		
		tc->SetCategoryBrushesAtCanvasTm(color_type, t_cats, false, t);
		
		for (int i=0, nb=best_breaks.size(); i<=nb; i++) {
			int ss = (i == 0) ? 0 : best_breaks[i-1];
			int tt = (i == nb) ? v.size() : best_breaks[i];
			for (int j=ss; j<tt; j++) {
				tc->AppendIdToCategory(t, i, var[t][j].second);
			}
			wxString l;
			l << "[" << var[t][ss].first;
			l << ":" << var[t][tt-1].first << "] ";
			l << "(" << tc->GetNumObsInCategory(t, i) << ")";
			tc->SetCategoryLabel(t, i, l);
		}
	}
}

wxString ThemeUtilities::ThemeTypeToString(ThemeType theme_type)
{
	if (theme_type == ThemeUtilities::no_theme) {
		return "Themeless";
	} else if (theme_type == ThemeUtilities::quantile) {
		return "Quantile";
	} else if (theme_type == ThemeUtilities::unique_values) {
		return "Unique Values";
	} else if (theme_type == ThemeUtilities::natural_breaks) {
		return "Natural Breaks";
	} else if (theme_type == ThemeUtilities::equal_intervals) {
		return "Equal Intervals";
	} else if (theme_type == ThemeUtilities::percentile) {
		return "Percentile";
	} else if (theme_type == ThemeUtilities::hinge_15) {
		return "Hinge=1.5";
	} else if (theme_type == ThemeUtilities::hinge_30) {
		return "Hinge=3.0";
	} else if (theme_type == ThemeUtilities::stddev) {
		return "Standard Deviation";
	} else if (theme_type == ThemeUtilities::excess_risk_theme) {
		return "Excess Risk";
	} else if (theme_type == ThemeUtilities::lisa_categories ||
			   ThemeUtilities::lisa_significance) {
		return "LISA";
	} else if (theme_type == ThemeUtilities::getis_ord_categories ||
			   ThemeUtilities::getis_ord_significance) {
		return "Getis-Ord";
	}
	return wxEmptyString;
}
