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

#include "GenUtils.h"
#include "GeoDaConst.h"
#include <math.h>
#include <cfloat>
#include <limits>
#include <boost/math/distributions/students_t.hpp>
#include <sstream>
#include <iomanip>
#include <wx/dc.h>
#include "logger.h"


using namespace std;

SampleStatistics::SampleStatistics(const std::vector<double>& data)
	: sample_size(0), min(0), max(0), mean(0),
	var_with_bessel(0), var_without_bessel(0),
	sd_with_bessel(0), sd_without_bessel(0)
{
	CalculateFromSample(data);
}

void SampleStatistics::CalculateFromSample(const std::vector<double>& data)
{
	sample_size = data.size();
	if (sample_size == 0) return;
	
	min = CalcMin(data);
	max = CalcMax(data);
	mean = CalcMean(data);
	
	double n = sample_size;
	double sum_squares = 0;
	for (int i=0, iend = data.size(); i<iend; i++) {
		sum_squares += data[i] * data[i];
	}
	
	var_without_bessel = (sum_squares/n) - (mean*mean);
	sd_without_bessel = sqrt(var_without_bessel);
	
	if (sample_size == 1) {
		var_with_bessel = var_without_bessel;
		sd_with_bessel = sd_without_bessel;
	} else {
		var_with_bessel = (n/(n-1)) * var_without_bessel;
		sd_with_bessel = sqrt(var_with_bessel);
	}
}

string SampleStatistics::ToString()
{
	ostringstream ss;
	ss << "sample_size = " << sample_size << endl;
	ss << "min = " << min << endl;
	ss << "max = " << max << endl;
	ss << "mean = " << mean << endl;
	ss << "var_with_bessel = " << var_with_bessel << endl;
	ss << "var_without_bessel = " << var_without_bessel << endl;
	ss << "sd_with_bessel = " << sd_with_bessel << endl;
	ss << "sd_without_bessel = " << sd_without_bessel << endl;
	return ss.str();
}

double SampleStatistics::CalcMin(const std::vector<double>& data)
{
	double min = std::numeric_limits<double>::max();
	for (int i=0, iend=data.size(); i<iend; i++) {
		if ( data[i] < min ) min = data[i];
	}
	return min;
}

double SampleStatistics::CalcMax(const std::vector<double>& data)
{
	double max = std::numeric_limits<double>::min();
	for (int i=0, iend=data.size(); i<iend; i++) {
		if ( data[i] > max ) max = data[i];
	}
	return max;
}

double SampleStatistics::CalcMean(const std::vector<double>& data)
{
	if (data.size() == 0) return 0;
	double total = 0;
	for (int i=0, iend=data.size(); i<iend; i++) {
		total += data[i];
	}
	return total / (double) data.size();
}

SimpleLinearRegression::SimpleLinearRegression(const std::vector<double>& X,
											   const std::vector<double>& Y,
											   double meanX, double meanY,
											   double varX, double varY)
	: covariance(0), correlation(0), alpha(0), beta(0), r_squared(0),
	std_err_of_estimate(0), std_err_of_beta(0), std_err_of_alpha(0),
	t_score_alpha(0), t_score_beta(0), p_value_alpha(0), p_value_beta(0),
	valid(false), valid_correlation(false), valid_std_err(false)
{
	CalculateRegression(X, Y, meanX, meanY, varX, varY);
}

void SimpleLinearRegression::CalculateRegression(const std::vector<double>& X,
												 const std::vector<double>& Y,
												 double meanX, double meanY,
												 double varX, double varY)
{
	LOG_MSG("Entering SimpleLinearRegression::CalculateRegression");
	LOG(meanX);
	LOG(meanY);
	LOG(varX);
	LOG(varY);
	if (X.size() != Y.size() || X.size() < 2 ) return;
	double expectXY = 0;
	for (int i=0, iend=X.size(); i<iend; i++) {
		expectXY += X[i]*Y[i];
	}
	expectXY /= (double) X.size();
	covariance = expectXY - meanX * meanY;
	LOG(expectXY);
	LOG(covariance);
	if (varX > 4*DBL_MIN) {
		beta = covariance / varX;
		alpha = meanY - beta * meanX;
		valid = true;
		LOG(beta);
	}
	double SS_tot = varY*Y.size();
	double SS_err = 0;
	double err=0;
	for (int i=0, iend=Y.size(); i<iend; i++) {
		err = Y[i] - (alpha + beta * X[i]);
		SS_err += err * err;
	}
	if (SS_err < 16*DBL_MIN) {
		r_squared = 1;
	} else {
		r_squared = 1 - SS_err / SS_tot;
	}
	
	if (Y.size()>2 && varX > 4*DBL_MIN) {
		std_err_of_estimate = SS_err/(Y.size()-2); // SS_err/(n-k-1), k=1
		std_err_of_estimate = sqrt(std_err_of_estimate);
		std_err_of_beta = std_err_of_estimate/sqrt(X.size()*varX);
		double sum_x_squared = 0;
		for (int i=0, iend=X.size(); i<iend; i++) {
			sum_x_squared += X[i] * X[i];
		}
		std_err_of_alpha = std_err_of_beta * sqrt(sum_x_squared / X.size());
		
		if (std_err_of_alpha >= 16*DBL_MIN) {
			t_score_alpha = alpha / std_err_of_alpha;
		} else {
			t_score_alpha = 100;
		}
		if (std_err_of_beta >= 16*DBL_MIN) {
			t_score_beta = beta	/ std_err_of_beta;
		} else {
			t_score_beta = 100;
		}
		p_value_alpha = TScoreTo2SidedPValue(t_score_alpha, X.size()-2);
		p_value_beta = TScoreTo2SidedPValue(t_score_beta, X.size()-2);
		
		LOG(std_err_of_estimate);
		LOG(std_err_of_beta);
		LOG(std_err_of_alpha);
		LOG(t_score_alpha);
		LOG(p_value_alpha);
		LOG(t_score_beta);
		LOG(p_value_beta);
		valid_std_err = true;
	}
	
	double d = sqrt(varX)*sqrt(varY);
	if (d > 4*DBL_MIN) {
		correlation = covariance / d;
		valid_correlation = true;
		LOG(correlation);
	}
	LOG_MSG("Exiting SimpleLinearRegression::CalculateRegression");
}

double SimpleLinearRegression::TScoreTo2SidedPValue(double tscore, int df)
{
	using namespace boost::math;
	students_t dist(df);
	// Cumulative Distribution Function evaluated at tscore
	if ( tscore >= 0) {
		return 2*(1.0-cdf(dist, tscore));
	} else {
		return 2*cdf(dist,tscore);
	}

}

string SimpleLinearRegression::ToString()
{
	ostringstream ss;
	ss << "covariance = " << covariance << endl;
	ss << "correlation = " << correlation << endl;
	ss << "alpha = " << alpha << endl;
	ss << "beta = " << beta << endl;
	ss << "r_squared = " << r_squared << endl;
	ss << "valid = " << (valid ? "true" : "false") << endl;
	ss << "valid_correlation = " << (valid_correlation ? "true" : "false")
		<< endl;
	return ss.str();
}

AxisScale::AxisScale(double data_min_s, double data_max_s)
: data_min(0), data_max(0), scale_min(0), scale_max(0),
scale_range(0), tic_inc(0), p(0)
{
	CalculateScale(data_min_s, data_max_s);
}

AxisScale::AxisScale(const AxisScale& s)
: data_min(s.data_min), data_max(s.data_max),
	scale_min(s.scale_min), scale_max(s.scale_max),
	scale_range(s.scale_range), tic_inc(s.tic_inc), p(s.p),
	tics(s.tics), tics_str(s.tics_str)
{
}

AxisScale& AxisScale::operator=(const AxisScale& s)
{
	data_min = s.data_min;
	data_max = s.data_max;
	scale_min = s.scale_min;
	scale_max = s.scale_max;
	scale_range = s.scale_range;
	tic_inc = s.tic_inc;
	p = s.p;
	tics = s.tics;
	tics_str = s.tics_str;
	return *this;
}

void AxisScale::CalculateScale(double data_min_s, double data_max_s)
{
	if (data_min_s <= data_max_s) {
		data_min = data_min_s;
		data_max = data_max_s;
	} else {
		data_min = data_max_s;
		data_max = data_min_s;	
	}
	
	double data_range = data_max - data_min;
	if ( data_range <= 2*DBL_MIN ) {
		scale_max = ceil((data_max + 0.05)*10)/10;
		scale_min = floor((data_min - 0.05)*10)/10;
		scale_range = scale_max - scale_min;
		p = 1;
		tic_inc = scale_range/2;
		tics.resize(3);
		tics_str.resize(3);
		tics[0] = scale_min;
		tics[1] = scale_min + tic_inc;
		tics[2] = scale_max;
	} else {
		p = floor(log10(data_range))-1;
		scale_max = ceil(data_max / pow((double)10,p)) * pow((double)10,p);
		scale_min = floor(data_min / pow((double)10,p)) * pow((double)10,p);
		scale_range = scale_max - scale_min;
		tic_inc = floor((scale_range / pow((double)10,p))/4)
			* pow((double)10,p);
		if (scale_min + tic_inc*5 <= scale_max + 2*DBL_MIN) {
			tics.resize(6);
			tics_str.resize(6);
		} else {
			tics.resize(5);
			tics_str.resize(5);
		}
		for (int i=0, iend=tics.size(); i<iend; i++) {
			tics[i] = scale_min + i*tic_inc;
		}
	}
	for (int i=0, iend=tics.size(); i<iend; i++) {
		ostringstream ss;
		ss << tics[i];
		tics_str[i] = ss.str();
	}
}

string AxisScale::ToString()
{
	ostringstream ss;
	ss << "data_min = " << data_min << endl;
	ss << "data_max = " << data_max << endl;
	ss << "scale_min = " << scale_min << endl;
	ss << "scale_max = " << scale_max << endl;
	ss << "scale_range = " << scale_range << endl;
	ss << "p = " << p << endl;
	ss << "tic_inc = " << tic_inc << endl;
	for (int i=0, iend=tics.size(); i<iend; i++) {
		ss << "tics[" << i << "] = " << tics[i];
		ss << ",  tics_str[" << i << "] = " << tics_str[i] << endl;
	}
	ss << "Exiting AxisScale::CalculateScale" << endl;
	return ss.str();
}

// NOTE: should take into account undefined values.
void GenUtils::DeviationFromMean(int nObs, double* data)
{
	if (nObs == 0) return;
	double sum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) sum += data[i];
	const double mean = sum / (double) nObs;
	for (int i=0, iend=nObs; i<iend; i++) data[i] -= mean;
}

void GenUtils::DeviationFromMean(std::vector<double>& data)
{
	LOG_MSG("Entering GenUtils::DeviationFromMean");
	if (data.size() == 0) return;
	double sum = 0.0;
	for (int i=0, iend=data.size(); i<iend; i++) sum += data[i];
	const double mean = sum / (double) data.size();
	LOG(mean);
	for (int i=0, iend=data.size(); i<iend; i++) data[i] -= mean;
	LOG_MSG("Exiting GenUtils::DeviationFromMean");
}

bool GenUtils::StandardizeData(int nObs, double* data)
{
	if (nObs <= 1) return false;
	GenUtils::DeviationFromMean(nObs, data);
	double ssum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (nObs-1.0));
	LOG(sd);
	if (sd == 0) return false;
	for (int i=0, iend=nObs; i<iend; i++) data[i] /= sd;
	return true;
}

bool GenUtils::StandardizeData(std::vector<double>& data)
{
	LOG_MSG("Entering GenUtils::StandardizeData");
	if (data.size() <= 1) return false;
	GenUtils::DeviationFromMean(data);
	double ssum = 0.0;
	for (int i=0, iend=data.size(); i<iend; i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (data.size()-1.0));
	LOG(sd);
	if (sd == 0) return false;
	for (int i=0, iend=data.size(); i<iend; i++) data[i] /= sd;
	LOG_MSG("Exiting GenUtils::StandardizeData");
	return true;
}

void GenUtils::extension(char *ofn, const char *fname, const char *ext)
{
	/* Creates file name from the root and extension. If extension is missing
	 leaves the file name as is. */
	strcpy(ofn, fname);
	if (ext)  
	{  /* case extension is given */
		char * ch= ofn;
		char * first = ofn;
		// Go to the end of string
		while (*ch) ++ch;
		--ch;
		while (ch > first  && *ch != '.') --ch;
		if (ch > first)
		{
			*ch= '\x0';
			strcat(ofn, ".");
			strcat(ofn, ext);
		}
		else
		{
			strcat(ofn, ".");
			strcat(ofn, ext);
		}
	};
	
	//	wxString xx;xx.Format("FileName:%s",ofn); wxMessageBox(xx);
	return;
}

wxString GenUtils::swapExtension(const wxString& fname, const wxString& ext)
{
	if (ext.IsEmpty()) return fname;
	wxString prefix = fname.BeforeLast('.');
	if (prefix.IsEmpty()) return fname + "." + ext;
	return prefix + "." + ext;
}

char* GenUtils::substr(char* s1, char* s2) 
{
	char * w1= s1, * w2= s2;
	while (*s1 != '\x0') {
		if (*w1 == *w2) {
			do {
				w1++;  w2++;
			}
			while (*w1 == *w2 && *w2 != '\x0');
			if (*w2 == '\x0') return s1;
		};
		w1= ++s1; w2= s2;
	};
	return NULL;
}

bool GenUtils::substr(const wxString& s1, const wxString& s2) 
{
	return (s1.find(s2) != wxString::npos);
}

wxString GenUtils::GetFileDirectory(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Left(pos);
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Left(pos);
	return wxEmptyString;
}

wxString GenUtils::GetFileName(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::GetFileExt(const wxString& path)
{
	int pos = path.Find('.',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::GetTheFileTitle(const wxString& path)
{
	wxString strResult = GetFileName(path);
	int pos = strResult.Find('.',true);
	if (pos >= 0)
		return strResult.Left(pos);
	return strResult;
}

void GenUtils::String2Upper(char * ch)  
{
	while (*ch) {
		*ch = toupper(*ch);
		++ch;
	}
}

/*
 Reverse
 Changes the order of bytes in the presentation of a 4 byte number.
  */
wxInt32 GenUtils::Reverse(const wxInt32 &val)
{
	union {
		wxInt32 v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

long GenUtils::ReverseInt(const int &val)
{
	union {
		int v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

void GenUtils::SkipTillNumber(std::istream &s)
{
	char ch;
	while (s >> ch) {
		if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.')
			break;
	}
	if (s.good())
		s.putback(ch);
}

// This is an implementation of ltoa
void GenUtils::longToString(const long d, char* Id, const int base) 
{
	int i = 0;
	long j = d;
	char rId[ GeoDaConst::ShpObjIdLen ];
	if (d == 0) 
	{
		Id[0] = '0';
		Id[1] = '\0';
		return;
	}
	if (d < 0) j = -d;
	while (j != 0) 
	{
		rId[i] = (j % base) + '0';
		j = j / base;
		i++;
	}
	j = i;
	if (d < 0) 
	{
		Id[0] = '-';
		Id[i + 1] = '\0';
		while (i > 0) {
			Id[i] = rId[j - i];
			i--;
		}
		return;
	}
	
	Id[i] = '\0';
	while (i > 0) 
	{
		Id[i - 1] = rId[j - i];
		i--;
	}
	return;
}

// Calculates Euclidean distance
double GenUtils::distance(const wxRealPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxRealPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

void GenUtils::DrawSmallCirc(wxDC* dc, int x, int y, int radius,
							 const wxColour& color)
{	
	wxPen pen;
    if (color == GeoDaConst::highlight_color) {
	    pen.SetColour(*wxRED);
	    dc->SetPen(pen);
	    dc->SetBrush(GeoDaConst::highlight_color);
	} else {
		pen.SetColour(color);
		dc->SetPen(pen);
		dc->SetBrush(*wxWHITE);
	}
	if (radius < 1) radius = 1;
	dc->DrawCircle(x, y, radius);
}

void GenUtils::strToInt64(const wxString& str, wxInt64 *val)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	strToInt64(buf, val);
}

// Convert an ASCII string into a wxInt64 (or long long)
void GenUtils::strToInt64(const char *str, wxInt64 *val)
{
	wxInt64 total = 0;
	bool minus = 0;
 
	while (isspace(*str)) str++;
	if (*str == '+') {
		str++;
	} else if (*str == '-') {
		minus = true;
		str++;
	}
	while (isdigit(*str)) {
		total *= 10;
		total += (*str++ - '0');
	}
	*val = minus ? -total : total;
}

bool GenUtils::validInt(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return validInt(buf);
}

// Checks that an ASCII string can be parsed to a valid integer.  At least
// one digit must been found.
bool GenUtils::validInt(const char* str)
{
	//LOG_MSG(wxString::Format("GenUtils::validInt(\"%s\"):", str));
	while (isspace(*str)) str++;
	if (*str == '+' || *str == '-') str++;
	const char* t = str;
	while (isdigit(*str)) str++;
	if (t == str) {
		// no digit found so return false
		//LOG_MSG("   no digit found");
		return false;
	}
	while (isspace(*str)) str++;
	// only return true if we are finally pointing at
	// the null terminating character.
	//LOG_MSG(wxString::Format("   final char is null: %d", *str == '\0'));
	return *str == '\0';
}

bool GenUtils::isEmptyOrSpaces(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return isEmptyOrSpaces(buf);
}

// returns true if the string is either empty
// or has only space characters
bool GenUtils::isEmptyOrSpaces(const char *str)
{
	while (isspace(*str)) str++;
	// if the first not-space char is not the end of the string,
	// return false.
	return *str == '\0';
}
