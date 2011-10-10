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

#ifndef __GEODA_CENTER_GEN_UTILS_H__
#define __GEODA_CENTER_GEN_UTILS_H__

#include <wx/string.h>
#include <wx/gdicmn.h> // for wxPoint / wxRealPoint
#include <vector>
#include <string>

class wxDC;

struct DataPoint {
    double vertical, horizontal;
    DataPoint(const double h= 0, const double v= 0)
	: vertical(v), horizontal(h) {};
	DataPoint& operator=(const DataPoint& s) {
		vertical = s.vertical; horizontal = s.horizontal; return *this;}
};

struct SampleStatistics {
	SampleStatistics() : sample_size(0), min(0), max(0), mean(0),
    var_with_bessel(0), var_without_bessel(0),
    sd_with_bessel(0), sd_without_bessel(0) {}
	SampleStatistics(const std::vector<double>& data);
	void CalculateFromSample(const std::vector<double>& data);
	std::string ToString();
	
	int sample_size;
	double min;
	double max;
	double mean;
	double var_with_bessel;
	double var_without_bessel;
	double sd_with_bessel;
	double sd_without_bessel;
	
	static double CalcMin(const std::vector<double>& data);
	static double CalcMax(const std::vector<double>& data);
	static double CalcMean(const std::vector<double>& data);
};

struct SimpleLinearRegression {
	SimpleLinearRegression() : covariance(0), correlation(0), alpha(0),
		beta(0), r_squared(0), std_err_of_estimate(0), std_err_of_beta(0),
		std_err_of_alpha(0), t_score_alpha(0), t_score_beta(0),
		p_value_alpha(0), p_value_beta(0),
		valid(false), valid_correlation(false),
		valid_std_err(false) {}
	SimpleLinearRegression(const std::vector<double>& X,
						   const std::vector<double>& Y,
						   double meanX, double meanY,
						   double varX, double varY);
	void CalculateRegression(const std::vector<double>& X,
							 const std::vector<double>& Y,
							 double meanX, double meanY,
							 double varX, double varY);
	static double TScoreTo2SidedPValue(double tscore, int df);
	std::string ToString();
	
	double covariance;
	double correlation;
	double alpha;
	double beta;
	double r_squared;
	double std_err_of_estimate;
	double std_err_of_beta;
	double std_err_of_alpha;
	double t_score_alpha;
	double t_score_beta;
	double p_value_alpha;
	double p_value_beta;
	bool valid;
	bool valid_correlation;
	bool valid_std_err;
};

struct AxisScale {
	AxisScale() : data_min(0), data_max(0),
    scale_min(0), scale_max(0), scale_range(0), tic_inc(0), p(0) {}
	AxisScale(double data_min_s, double data_max_s);
	AxisScale(const AxisScale& s);
	virtual AxisScale& operator=(const AxisScale& s);
	virtual ~AxisScale() {}
	void CalculateScale(double data_min_s, double data_max_s);
	std::string ToString();
	
	double data_min;
	double data_max;
	double scale_min;
	double scale_max;
	double scale_range;
	double tic_inc;
	int p; // power of ten to scale significant digit
	std::vector<double>tics; // numerical tic values
	std::vector<std::string>tics_str; // tics in formated string representation
};


namespace GenUtils {
	void DeviationFromMean(int nObs, double* data);
	void DeviationFromMean(std::vector<double>& data);
	bool StandardizeData(int nObs, double* data);
	bool StandardizeData(std::vector<double>& data);
	template<class T> const T& max(const T& x, const T& y);
	template<class T> const T& min(const T& x, const T& y);
	void extension(char *ofn, const char *fname, const char *ext);
	wxString swapExtension(const wxString& fname, const wxString& ext);
	char* substr(char* s1, char* s2);
	bool substr(const wxString& s1, const wxString& s2); 
	wxString GetFileDirectory(const wxString& path);
	wxString GetFileName(const wxString& path);
	wxString GetFileExt(const wxString& path);
	wxString GetTheFileTitle(const wxString& path);
	void String2Upper(char * ch);
	wxInt32 Reverse(const wxInt32 &val);
	long ReverseInt(const int &val);
	void SkipTillNumber(std::istream &s);
	void longToString(const long d, char* Id, const int base);
	double distance(const wxRealPoint& p1, const wxRealPoint& p2);
	double distance(const wxRealPoint& p1, const wxPoint& p2);
	double distance(const wxPoint& p1, const wxRealPoint& p2);
	double distance(const wxPoint& p1, const wxPoint& p2);
	void DrawSmallCirc(wxDC* dc, int x, int y, int radius,
					   const wxColour& color);
	void strToInt64(const char *str, wxInt64 *val);
	void strToInt64(const wxString& str, wxInt64 *val);
	bool validInt(const char* str);
	bool validInt(const wxString& str);
	bool isEmptyOrSpaces(const char *str);
	bool isEmptyOrSpaces(const wxString& str);
}

/** Old code used by LISA functions */
class OgSet {
private:
    int size;
	int current;
    int* buffer;
    char* flags;
public:
	OgSet(const int sz) : size(sz), current(0) {
		buffer = new int [ size ];
		flags = new char [ size ];
		memset(flags, '\x0', size);
	}
	virtual ~OgSet() {
		if (buffer) delete [] buffer; buffer = 0;
		if (flags) delete [] flags; flags = 0;
		size = current = 0;
	}
    bool Belongs( const int elt) const {
		return flags[elt] != 0; }; // true if the elt belongs to the set
    void Push(const int elt) {
		// insert element in the set, if it is not yet inserted
		if (flags[elt] == 0)  {
			buffer[ current++ ] = elt;
			flags[elt] = 'i';  // check elt in
        }
    }
    int Pop() { // remove element from the set
		if (current == 0) return -1; // formerly GeoDaConst::EMPTY
        int rtn= buffer[ --current ];
        flags[rtn]= '\x0';   // check it out
        return rtn;
    }
    int Size() const { return current; }
	void Reset() {
		memset(flags, '\x0', size);
		current = 0;
	}
};

/*
 * Template Definitions
 *
 * Note: Template Definitions must not be compiled independently.
 *       Put all template definitions below, and all non-template
 *       definitions in the GenUtils.cpp file.  If a template
 *       definition is put in GenUtils.cpp, there will be a linking
 *       error.
 *
 */

template<class T> const T& GenUtils::max(const T& x, const T& y)
{
	return (x<y) ? y : x;
}

template<class T> const T& GenUtils::min(const T& x, const T& y)
{
	return (x<y) ? x : y;
}

#endif
