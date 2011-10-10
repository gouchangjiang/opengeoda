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

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm> // for vector sorting
#include <set>
#include <boost/math/special_functions/fpclassify.hpp>
#include <wx/colour.h>
#include "../Generic/HighlightState.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "DbfGridTableBase.h"

DbfColContainer::DbfColContainer(DbfGridTableBase* grid_base_s)
: size(0), type(GeoDaConst::unknown_type),
field_len(0), decimals(0),
d_vec(0), l_vec(0), s_vec(0), undefined(0), undefined_initialized(false),
vector_valid(false), raw_data(0), grid_base(grid_base_s)
{
}

DbfColContainer::DbfColContainer(DbfFileReader& dbf, int field,
								 DbfGridTableBase* grid_base_s)
: size(0), type(GeoDaConst::unknown_type),
field_len(0), decimals(0),
d_vec(0), l_vec(0), s_vec(0), undefined(0), undefined_initialized(false),
vector_valid(false), raw_data(0), grid_base(grid_base_s)
{
	int rows = dbf.getNumRecords();
	if (field < 0 || field >= dbf.getNumFields()) return;
	std::vector<DbfFieldDesc> fields = dbf.getFieldDescs();
	
	DbfFieldDesc desc = dbf.getFieldDesc(field);
	DbfFileHeader header = dbf.getFileHeader();
	GeoDaConst::FieldType type;
	if (desc.type == 'N' || desc.type == 'F') {
		if (desc.decimals > 0) {
			type = GeoDaConst::double_type;
			Init(rows, type, desc.name, desc.length, desc.decimals,
				 desc.decimals, true, false, false);
		} else {
			type = GeoDaConst::long64_type;
			Init(rows, type, desc.name, desc.length, desc.decimals,
				 desc.decimals, true, false, false);
		}
	} else if (desc.type == 'D') {
		if (desc.length != 8) {
			LOG_MSG("Error: Date field found with incorrect length!"
					" We recomend fixing this in your DBF before "
					"proceeding.");
		}
		type = GeoDaConst::date_type;
		Init(rows, type, desc.name, desc.length, 0, 0, true, false, false);
	} else {
		// We will assume (desc.type == 'C')
		type = GeoDaConst::string_type;
		Init(rows, type, desc.name, desc.length, desc.decimals, desc.decimals,
			 true, false, false);
	}
	if (!dbf.file.is_open()) {
		dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) return;
	
	// calculate field offset
	int record_offset = 1; // the record deletion flag
	for (int i=0; i<field; i++) {
        record_offset += fields[i].length;
    }
    int field_length = fields[field].length;
		
    dbf.file.seekg(header.header_length + record_offset, std::ios::beg);
	
    for (int i=0; i< (int) header.num_records; i++) {
        dbf.file.read((char*)(raw_data + i*(field_length+1)), field_length);
		raw_data[i*(field_length+1)+field_length] = '\0';
        // seek to next record in file
        dbf.file.seekg(header.length_each_record-field_length, std::ios::cur);
	}
}

DbfColContainer::~DbfColContainer()
{
	if (raw_data) { delete [] raw_data; raw_data = 0; }
}

bool DbfColContainer::Init(int size_s, GeoDaConst::FieldType type_s,
						   const wxString& name_s, int field_len_s,
						   int decimals_s,
						   int displayed_decimals_s,
						   bool alloc_raw_data,
						   bool alloc_vector_data,
						   bool mark_all_defined)
{
	if (type != GeoDaConst::unknown_type || type_s == GeoDaConst::unknown_type
		|| size_s <= 0) {
		return false; // can't change type once set
	}
	
	name = name_s;
	size = size_s;
	type = type_s;
	field_len = field_len_s;
	decimals = decimals_s;
	displayed_decimals = displayed_decimals_s;
	
	// if mark_all_defined is true, then mark all as begin defined.
	undefined.resize(size, !mark_all_defined);
	undefined_initialized = mark_all_defined;
	
	if (alloc_raw_data) {
		raw_data = new char[size * (field_len+1)];
	}
	if (alloc_vector_data) {
		vector_valid = true;
		switch (type) {
			case GeoDaConst::date_type:
				l_vec.resize(size, 0);
				return true;
				break;
			case GeoDaConst::long64_type:
				l_vec.resize(size, 0);
				return true;
				break;
			case GeoDaConst::double_type:
				d_vec.resize(size, 0);
				return true;
				break;
			case GeoDaConst::string_type:
				s_vec.resize(size);
				return true;
				break;
			default:
				break;
		}
		vector_valid = false;
		return false;
	}
	return true;
}

// Change Properties will convert data to vector format if length or
// decimals are changed.
bool DbfColContainer::ChangeProperties(const wxString& new_name, int new_len,
									   int new_dec, int new_disp_dec)
{
	if (!raw_data && !vector_valid) {
		// this violates the assumption that at least either raw_data
		// exists or a valid vector exists.  This should never happen
		return false;
	}
	
	if (!DbfFileUtils::isValidFieldName(new_name)) return false;
	if (type == GeoDaConst::string_type) {
		if (new_len < GeoDaConst::min_dbf_string_len ||
			new_len > GeoDaConst::max_dbf_string_len) {
			return false;
		}
		if (raw_data && !vector_valid) {
			s_vec.resize(size);
			raw_data_to_vec(s_vec);
		}
		// shorten all strings as needed.
		if (new_len < field_len) {
			for (int i=0; i<size; i++) {
				if (new_len < s_vec[i].length()) {
					s_vec[i] = s_vec[i].SubString(0, new_len-1);
				}
			}
		}
		name = new_name;
	} else if (type == GeoDaConst::long64_type) {
		if (new_len < GeoDaConst::min_dbf_long_len ||
			new_len > GeoDaConst::max_dbf_long_len) {
			return false;
		}
		if (raw_data && !vector_valid) {
			l_vec.resize(size);
			raw_data_to_vec(l_vec);
		}
		// limit all vector values to acceptable range
		wxInt64 max_val = DbfFileUtils::GetMaxInt(new_len);
		wxInt64 min_val = DbfFileUtils::GetMinInt(new_len);
		for (int i=0; i<size; i++) {
			if (max_val < l_vec[i]) {
				l_vec[i] = max_val;
			} else if (min_val > l_vec[i]) {
				l_vec[i] = min_val;
			}
		}
		name = new_name;
	} else if (type == GeoDaConst::double_type) {
		if (new_disp_dec < 0 ||
			new_disp_dec > GeoDaConst::max_dbf_double_decimals ||
			new_len < GeoDaConst::min_dbf_double_len ||
			new_len > GeoDaConst::max_dbf_double_len ||
			new_dec < GeoDaConst::min_dbf_double_decimals ||
			new_dec > GeoDaConst::max_dbf_double_decimals) {
			return false;
		}
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(new_len, new_dec,
										  &suggest_len, &suggest_dec);
		if (new_len != suggest_len || new_dec != suggest_dec) {
			return false;
		}
		if (raw_data && !vector_valid) {
			d_vec.resize(size);
			raw_data_to_vec(d_vec);
		}
		// limit all vectors to acceptable range
		double max_val = DbfFileUtils::GetMaxDouble(new_len, new_dec);
		double min_val = DbfFileUtils::GetMinDouble(new_len, new_dec);
		for (int i=0; i<size; i++) {
			if (max_val < d_vec[i]) {
				d_vec[i] = max_val;
			} else if (min_val > d_vec[i]) {
				d_vec[i] = min_val;
			}
		}
		decimals = new_dec;
		displayed_decimals = new_disp_dec;
		name = new_name;
	} else { // GeoDaConst::date_type
		// can only change field name for date_type
		if (new_len != GeoDaConst::max_dbf_date_len) return false;
		name = new_name;
	}
	
	if (raw_data) {
		delete [] raw_data;
		raw_data = 0;
	}	
	vector_valid = true;
	field_len = new_len;
	grid_base->SetChangedSinceLastSave(true);
	return true;
}

bool DbfColContainer::sprintf_period_for_decimal()
{
	char buf[10];
	sprintf(buf, "%#3.1f", 2.5);
	//LOG_MSG(wxString::Format("DbfColContainer::sprintf_period_for_decimal()"
	//						 " = %s", buf[1] == '.' ? "true" : "false"));
	return buf[1] == '.';
}

bool DbfColContainer::IsVecItemDefined(int i)
{
	if (!vector_valid || (undefined_initialized && undefined[i])) {
		return false;
	}
	// can assume vector_valid and !undefined_initialized, so need to
	// determine if vector data is valid depending on the type
	if (type == GeoDaConst::string_type) return true;
	if (type == GeoDaConst::long64_type) return true;
	if (type == GeoDaConst::date_type) return (l_vec[i] != 0);
	// can assume that type is double_type
	return boost::math::isfinite<double>(d_vec[i]);
}

bool DbfColContainer::IsRawItemDefined(int i)
{
	if (!raw_data || (undefined_initialized && undefined[i])) {
		return false;
	}
	// need to determine if data is undefined based on raw string data
	const char* buf = (char*)(raw_data + i*(field_len+1));
	if (type == GeoDaConst::string_type) return true;
	if (type == GeoDaConst::long64_type) return GenUtils::validInt(buf);
	// We need to come up with a better date check than the following
	if (type == GeoDaConst::date_type) return GenUtils::validInt(buf);
	// can assume that type is double_type
	if (GenUtils::isEmptyOrSpaces(buf)) return false;
	wxString temp(buf);
	temp.Trim(true);
	temp.Trim(false);
	double val;
	bool r = temp.ToCDouble(&val);
	return r && boost::math::isfinite<double>(val);
	
}

// Allow for filling of double from long64 field
void DbfColContainer::GetVec(std::vector<double>& vec)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (vec.size() != size) vec.resize(size);
	if (type == GeoDaConst::double_type) {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = d_vec[i];
		} else {
			raw_data_to_vec(vec);
		}
	} else {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = (double) l_vec[i];
		} else {
			std::vector<wxInt64> t(size);
			raw_data_to_vec(t);
			for (int i=0; i<size; i++) vec[i] = (double) t[i];
		}
	}
}

// Allow for filling of long64 from double field
void DbfColContainer::GetVec(std::vector<wxInt64>& vec)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (vec.size() != size) vec.resize(size);
	if (type == GeoDaConst::long64_type) {	
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = l_vec[i];
		} else {
			raw_data_to_vec(vec);
		}
	} else {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = (wxInt64) d_vec[i];
		} else {
			std::vector<double> t(size);
			raw_data_to_vec(t);
			for (int i=0; i<size; i++) vec[i] = (wxInt64) t[i];
		}		
	}
}

void DbfColContainer::GetVec(std::vector<wxString>& vec)
{
	if (vec.size() != size) vec.resize(size);
	if (vector_valid) {
		for (int i=0; i<size; i++) vec[i] = s_vec[i];
	} else {
		raw_data_to_vec(vec);
	}
}

// Note: we should check that every value written is within proper bounds
void DbfColContainer::SetFromVec(std::vector<double>& vec)
{
	if (vec.size() != size) return;
	if (type == GeoDaConst::long64_type) {
		if (l_vec.size() != size) l_vec.resize(size);
	} else if (type == GeoDaConst::double_type) {
		if (d_vec.size() != size) d_vec.resize(size);
	} else {
		// only numeric types supported currently
		return;
	}
	if (raw_data) {
		delete [] raw_data;
		raw_data = 0;
	}
	
	for (int i=0; i<size; i++) {
		undefined[i] = !boost::math::isfinite<double>(vec[i]);
	}
	undefined_initialized = true;
	
	if (type == GeoDaConst::long64_type) {
		for (int i=0; i<size; i++) {
			l_vec[i] = undefined[i] ? 0 : (wxInt64) vec[i];
		}
	} else { // must be double_type
		for (int i=0; i<size; i++) {
			d_vec[i] = undefined[i] ? 0 : vec[i];
		}
	}
	vector_valid = true;
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::SetFromVec(std::vector<wxInt64>& vec)
{
	if (vec.size() != size) return;
	if (type == GeoDaConst::long64_type) {
		if (l_vec.size() != size) l_vec.resize(size);
	} else if (type == GeoDaConst::double_type) {
		if (d_vec.size() != size) d_vec.resize(size);
	} else {
		// only numeric types supported currently
		return;
	}
	if (raw_data) {
		delete [] raw_data;
		raw_data = 0;
	}
	
	for (int i=0; i<size; i++) undefined[i] = false;
	undefined_initialized = true;
	
	if (type == GeoDaConst::long64_type) {
		for (int i=0; i<size; i++) l_vec[i] = vec[i];
	} else { // must be double_type
		for (int i=0; i<size; i++) d_vec[i] = (double) vec[i];
	}
	vector_valid = true;
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::SetUndefined(const std::vector<bool>& undef_vec)
{
	if (undefined.size() != size) undefined.resize(size);
	for (int i=0; i<size; i++) undefined[i] = undef_vec[i];
	undefined_initialized = true;
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::GetUndefined(std::vector<bool>& undef_vec)
{
	if (undef_vec.size() != size) undef_vec.resize(size);
	if (!undefined_initialized) {
		CheckUndefined();
	}
	for (int i=0; i<size; i++) undef_vec[i] = undefined[i];
}

void DbfColContainer::CheckUndefined()
{
	if (undefined_initialized) return;
	undefined_initialized = true;
	if (undefined.size() != size) undefined.resize(size);
	for (int i=0; i<size; i++) undefined[i] = false;
	if (type == GeoDaConst::double_type) {
		if (raw_data) {
			std::vector<double> t;
			raw_data_to_vec(t);
		} else {
			for (int i=0; i<size; i++) {
				undefined[i] = !boost::math::isfinite<double>(d_vec[i]);
			}
		}
	} else if (type == GeoDaConst::long64_type) {
		if (raw_data) {
			std::vector<wxInt64> t;
			raw_data_to_vec(t);
		} else {
			// must be defined
		}
	}
}


void DbfColContainer::raw_data_to_vec(std::vector<double>& vec)
{
	if (vec.size() != size) vec.resize(size);
	int inc = field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		// we are not using atof since we it seems to be difficult
		// to choose a US locale on all systems so as to assume the
		// DBF-required use of '.' for the decimal character
		wxString temp(buf);
		temp.Trim(true);
		temp.Trim(false);
		bool r = temp.ToCDouble(&vec[i]);
		undefined[i] = !(r && boost::math::isfinite<double>(vec[i]));
		buf += inc;
	}
	undefined_initialized = true;
}


void DbfColContainer::raw_data_to_vec(std::vector<wxInt64>& vec)
{
	if (vec.size() != size) vec.resize(size);
	int inc = field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		undefined[i] = !GenUtils::validInt(buf);
		GenUtils::strToInt64(buf, &vec[i]);  // will set to 0 if undefined
		buf += inc;
	}
	undefined_initialized = true;
}

void DbfColContainer::raw_data_to_vec(std::vector<wxString>& vec)
{
	if (vec.size() != size) vec.resize(size);
	int inc = field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		vec[i] = wxString(buf);
		undefined[i] = false;
		buf += inc;
	}
	undefined_initialized = true;
}

void DbfColContainer::d_vec_to_raw_data()
{
	char temp[255];
	int inc = field_len + 1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (!boost::math::isfinite<double>(d_vec[i])) {
			// the number is either NaN (not a number) or +/- infinity
			undefined[i] = true;
		}
		if (undefined[i]) {
			for (int j=0; j<field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%#*.*f", field_len, decimals, d_vec[i]);
			for (int j=0; j<field_len; j++) buf[j] = temp[j];
		}
		buf[field_len] = '\0';
		buf += inc;
	}
	if (!sprintf_period_for_decimal()) {
		for (int i=0, iend=size*inc; i<iend; i++) {
			if (raw_data[i] == ',') raw_data[i] = '.';
		}
	}
}

void DbfColContainer::l_vec_to_raw_data()
{
	char temp[255];
	int inc = field_len + 1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (undefined[i]) {
			for (int j=0; j<field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%*lld", field_len, l_vec[i]);
			for (int j=0; j<field_len; j++) buf[j] = temp[j];
		}
		buf[field_len] = '\0';
		buf += inc;
	}
}

void DbfColContainer::s_vec_to_raw_data()
{
	char temp[255];
	int inc = field_len + 1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (undefined[i] || s_vec[i].IsEmpty()) {
			for (int j=0; j<field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%*s", field_len,
					(const_cast<char*>((const char*)s_vec[i].mb_str())));
			for (int j=0; j<field_len; j++) buf[j] = temp[j];
		}
		buf[field_len] = '\0';
		buf += inc;
	}
}

void DbfColContainer::CopyRawDataToVector()
{
	if (!raw_data) return;
	switch (type) {
		case GeoDaConst::date_type:
			raw_data_to_vec(l_vec);
			break;
		case GeoDaConst::long64_type:
			raw_data_to_vec(l_vec);
			break;
		case GeoDaConst::double_type:
			raw_data_to_vec(d_vec);
			break;
		case GeoDaConst::string_type:
			raw_data_to_vec(s_vec);
			break;
		default:
			break;
	}
	vector_valid = true;
}


void DbfColContainer::CopyVectorToRawData()
{
	switch (type) {
		case GeoDaConst::date_type:
		{
			if (l_vec.size() != size) return;
			if (!raw_data) raw_data = new char[size * (field_len+1)];
			
			l_vec_to_raw_data();
		}			
		case GeoDaConst::long64_type:
		{
			if (l_vec.size() != size) return;
			if (!raw_data) raw_data = new char[size * (field_len+1)];
			
			l_vec_to_raw_data();
		}
			break;
		case GeoDaConst::double_type:
		{
			if (d_vec.size() != size) return;
			if (!raw_data) raw_data = new char[size * (field_len+1)];
			
			d_vec_to_raw_data();
		}
			break;
		case GeoDaConst::string_type:
		{
			if (s_vec.size() != size) return;
			if (!raw_data) raw_data = new char[size * (field_len+1)];

			s_vec_to_raw_data();
		}
			break;
		default:
			break;
	}
}


DbfGridCellAttrProvider::DbfGridCellAttrProvider(std::vector<int>& row_order_s,
												 std::vector<bool>& selected_s)
: row_order(row_order_s), selected(selected_s)
{
    attrForSelectedRows = new wxGridCellAttr;
    attrForSelectedRows->SetBackgroundColour(*wxLIGHT_GREY);
}

DbfGridCellAttrProvider::~DbfGridCellAttrProvider()
{
    attrForSelectedRows->DecRef();
}

wxGridCellAttr *DbfGridCellAttrProvider::GetAttr(int row, int col,
									wxGridCellAttr::wxAttrKind kind ) const
{
	//LOG_MSG(wxString::Format("Calling DbfGridCellAttrProvider::GetAttr"
	//						 "(%d, %d, %d)", row, col, kind));
    wxGridCellAttr *attr = wxGridCellAttrProvider::GetAttr(row, col, kind);
	
	//if (row >= 0) LOG_MSG(wxString::Format("GetAttr: row=%d, "
	//									   "col=%d selected=%d",
	//									   row, col,
	//									   selected[row_order[row]] ? 1 : 0));
	
    if ( row >= 0 && selected[row_order[row]] ) {
        if ( !attr ) {
            attr = attrForSelectedRows;
            attr->IncRef();
        } else {
            if ( !attr->HasBackgroundColour() ) {
                wxGridCellAttr *attrNew = attr->Clone();
                attr->DecRef();
                attr = attrNew;
                attr->SetBackgroundColour(*wxLIGHT_GREY);
            }
        }
    }
	
    return attr;
}


DbfGridTableBase::DbfGridTableBase(int rows_s, int cols_s,
								   HighlightState* highlight_state_s)
: rows(rows_s), highlight_state(highlight_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1),
sorting_ascending(false),
row_order(rows_s), col_data(cols_s),
changed_since_last_save(false), dbf_file_name_no_ext("")
{
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols = cols_s;
	orig_header.version = 3; // default
	orig_header.year=2011;
	orig_header.month=1;
	orig_header.day=1;
	orig_header.num_records=0;
	orig_header.header_length=0;
	orig_header.length_each_record=0;
	orig_header.num_fields=0;
	
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		col_data[i] = new DbfColContainer(this);
		col_data[i]->Init(rows, GeoDaConst::double_type,
						 wxString::Format("col_%d", i+1),
						 20, 4, 4, false, true, true);
	}
	SortByDefaultDecending();
	for (int j=0, jend=cols; j<jend; j++) {
		std::vector<double>& col_ref = col_data[j]->d_vec;
		for (int i=0, iend=rows; i<iend; i++) {
			col_data[j]->undefined[i] = false;
			col_ref[i] = (j*10000)+i + 0.1;
		}
	}
	//SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

DbfGridTableBase::DbfGridTableBase(DbfFileReader& dbf,
								   HighlightState* highlight_state_s)
: rows(dbf.getNumRecords()), highlight_state(highlight_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1), sorting_ascending(false),
row_order(dbf.getNumRecords()), col_data(dbf.getNumFields()),
orig_header(dbf.getFileHeader()),
changed_since_last_save(false)
{
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols = dbf.getNumFields();
	SortByDefaultDecending();
	
	dbf_file_name = wxFileName(dbf.fname);
	dbf_file_name_no_ext = wxFileName(dbf.fname).GetName();
	
	std::vector<DbfFieldDesc> desc = dbf.getFieldDescs();
	for (int col=0, col_end=desc.size(); col<col_end; col++) {
		col_data[col] = new DbfColContainer(this);
		GeoDaConst::FieldType type = GeoDaConst::string_type;
		if (desc[col].type == 'N' || desc[col].type == 'F') {
			if (desc[col].decimals > 0) {
				type = GeoDaConst::double_type;
				col_data[col]->Init(rows, type, desc[col].name,
									desc[col].length, desc[col].decimals,
									desc[col].decimals,
									true, false, false);
				//dbf.getFieldValsDouble(col, col_data[col]->d_vec);				
			} else {
				type = GeoDaConst::long64_type;
				col_data[col]->Init(rows, type, desc[col].name,
									desc[col].length, desc[col].decimals,
									desc[col].decimals,
									true, false, false);
				//dbf.getFieldValsLong(col, col_data[col]->l_vec);
			}
		} else if (desc[col].type == 'D') {
			if (desc[col].length != 8) {
				LOG_MSG("Error: Date field found with incorrect length!"
						" We recomend fixing this in your DBF before "
						"proceeding.");
			}
			type = GeoDaConst::date_type;
			col_data[col]->Init(rows, type, desc[col].name,
								desc[col].length, 0, 0, true, false, false);
			// dbf.getFieldValsLong(col, col_data[col]->l_vec);
		} else {
			// We will assume (desc[i].type == 'C')
			type = GeoDaConst::string_type;
			col_data[col]->Init(rows, type, desc[col].name,
								desc[col].length, desc[col].decimals,
								desc[col].decimals,
								true, false, false);
			// note that we consider all strings to be valid, so we chose
			// to say that all string values have been defined.
			//dbf.getFieldValsString(col, col_data[col]->s_vec);
		}
	}
	
	if (!dbf.file.is_open()) {
		dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) return;
	
	// Note: first byte of every DBF row is the record deletion flag, so
	// we always skip this.
	int del_flag_len = 1;  // the record deletion flag
	dbf.file.seekg(dbf.header.header_length, std::ios::beg);
	for (int row=0; row<rows; row++) {
		dbf.file.seekg(del_flag_len, std::ios::cur);
		for (int col=0; col<cols; col++) {
			int field_len = desc[col].length;
			//LOG(dbf.file.tellg());
			dbf.file.read((char*)(col_data[col]->raw_data + row*(field_len+1)),
						  field_len);
			col_data[col]->raw_data[row*(field_len+1)+field_len] = '\0';
			//LOG_MSG(wxString((char*)(col_data[col]->raw_data
			//						 + row*(field_len+1))));
		}
	}
	SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	
	highlight_state->registerObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

DbfGridTableBase::~DbfGridTableBase()
{
	LOG_MSG("Entering DbfGridTableBase::~DbfGridTableBase");
	for (std::vector<DbfColContainer*>::iterator it=col_data.begin();
		 it != col_data.end(); it++) {
		delete (*it);
	}
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::~DbfGridTableBase");
}

/** Impelmentation of HighlightStateObserver interface function.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void DbfGridTableBase::update(HighlightState* o)
{
	LOG_MSG("Entering DbfGridTableBase::update");
	if (GetView()) GetView()->Refresh();
	LOG_MSG("Exiting DbfGridTableBase::update");
}


int DbfGridTableBase::GetNumberRows()
{
	return rows;
}

int DbfGridTableBase::GetNumberCols()
{ 
	return col_data.size();
}

wxString DbfGridTableBase::GetValue(int row, int col)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	int field_len = col_data[col]->field_len;
	
	if (col_data[col]->undefined_initialized &&
		col_data[col]->undefined[row_order[row]]) {
		return wxEmptyString;
	}

	switch (col_data[col]->type) {
		case GeoDaConst::date_type:
		{
			if (col_data[col]->vector_valid) {
				int x = col_data[col]->l_vec[row_order[row]];
				int day = x % 100; x /= 100;
				int month = x % 100; x /= 100;
				int year = x;
				return wxString::Format("%04d %02d %02d", year, month, day);
			}
			if (col_data[col]->raw_data) {
				wxString temp((char*)(col_data[col]->raw_data
									  + row_order[row]*(field_len+1)));
				long val;
				bool success = temp.ToCLong(&val);
				
				if (col_data[col]->undefined_initialized || success) {
					int x = val;
					int day = x % 100; x /= 100;
					int month = x % 100; x /= 100;
					int year = x;
					return wxString::Format("%04d %02d %02d", year, month, day);
				} else {
					return wxEmptyString;
				}
			}
		}
		case GeoDaConst::long64_type:
		{
			if (col_data[col]->vector_valid) {
				return wxString::Format("%lld",
										col_data[col]->l_vec[row_order[row]]);
			}
			if (col_data[col]->raw_data) {
				const char* str = (char*)(col_data[col]->raw_data
										  + row_order[row]*(field_len+1));
				//LOG_MSG(wxString::Format("row: %d, col: %d, raw: %s",
				//						 row, col, str));
				
				if (col_data[col]->undefined_initialized ||
					GenUtils::validInt(str)) {
					wxInt64 val=0;
					GenUtils::strToInt64(str, &val);
					return wxString::Format("%lld", val);
					LOG(val);
				} else {
					return wxEmptyString;
				}
			}
		}
			break;
		case GeoDaConst::double_type:
		{
			// We have to be careful to return a formated string with digits
			// after the decimal place at most min(decimals, displayed_decimals)
			int decimals = col_data[col]->decimals + 1; // one extra decimal
			int disp_dec = GenUtils::min<int>(col_data[col]->decimals,
											col_data[col]->displayed_decimals);
			wxString d_char = DbfColContainer::sprintf_period_for_decimal()
								? "." : ",";
			if (col_data[col]->vector_valid) {
				double val = col_data[col]->d_vec[row_order[row]];
				// limit val to acceptable range
				int d = col_data[col]->decimals;
				int fl = col_data[col]->field_len; 
				double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				double min_val = DbfFileUtils::GetMinDouble(fl, d);
				if (max_val < val) {
					val = max_val;
				} else if (min_val > val) {
					val = min_val;
				}
				wxString s = wxString::Format("%.*f", disp_dec, val);
				return s.SubString(0, s.Find(d_char) + disp_dec);
			}
			if (col_data[col]->raw_data) {
				wxString temp((char*)(col_data[col]->raw_data
									  + row_order[row]*(field_len+1)));
				// trim any leading or trailing spaces.  For some reason
				// a trailing space causes ToCDouble to return false even
				// though it set val to the correct value.
				temp.Trim(true);
				temp.Trim(false);
				double val;
				bool success = temp.ToCDouble(&val);
				if (success) success = boost::math::isfinite<double>(val);
				
				if (col_data[col]->undefined_initialized || success) {
					wxString s = wxString::Format("%.*f", disp_dec, val);
					return s.SubString(0, s.Find(d_char) + disp_dec);
				} else {
					return wxEmptyString;
				}
			}
		}
			break;
		case GeoDaConst::string_type:
		{
			if (col_data[col]->vector_valid) {
				return col_data[col]->s_vec[row_order[row]];
			}
			if (col_data[col]->raw_data) {
				return wxString((char*)(col_data[col]->raw_data
										+ row_order[row]*(field_len+1)));
			}
		}
			break;
		default:
			break;
	}
	return wxEmptyString;
}

// Note: when writing to raw_data, we must be careful not to overwrite
//       the buffer and also to respect the DBF formating requirements,
//       especially for floats.  Aditionally, must check that all numbers
//       are valid and set undefined flag appropriately.  Also, this
//       method should only be called by wxGrid since we automatically
//       compute the correct row.
void DbfGridTableBase::SetValue(int row, int col, const wxString &value)
{
	LOG_MSG(wxString::Format("DbfGridTableBase::SetValue(%d, %d, %s)",
							 row, col,
							 (const_cast<char*>((const char*)value.mb_str()))));
	if (row<0 || row>=GetNumberRows()) return;
	
	int field_len = col_data[col]->field_len;
	int rrow = row_order[row];
	char temp[1024];
	char* buf=0;
	if (col_data[col]->raw_data) {
		buf = col_data[col]->raw_data + rrow*(field_len+1);
		buf[field_len] = '\0';
	}
	
	col_data[col]->undefined[rrow] = false; // assume defined by default
	switch (col_data[col]->type) {
		case GeoDaConst::date_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			bool valid = GenUtils::validInt(
								const_cast<char*>((const char*)value.mb_str()));
			if (valid) {
				GenUtils::strToInt64(
					const_cast<char*>((const char*)value.mb_str()), &l_val);
			} else {
				col_data[col]->undefined[rrow] = true;
			}
			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[rrow]) {
					col_data[col]->l_vec[rrow] = 0;
				} else {
					col_data[col]->l_vec[rrow] = l_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GeoDaConst::long64_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			if (!GenUtils::validInt(value)) {
				col_data[col]->undefined[rrow] = true;
			} else {
				GenUtils::strToInt64(value, &l_val);
				// limit l_val to acceptable range
				int fl = col_data[col]->field_len;
				wxInt64 max_val = DbfFileUtils::GetMaxInt(fl);
				wxInt64 min_val = DbfFileUtils::GetMinInt(fl);
				if (max_val < l_val) {
					l_val = max_val;
				} else if (min_val > l_val) {
					l_val = min_val;
				}
			}

			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[rrow]) {
					col_data[col]->l_vec[rrow] = 0;
				} else {
					col_data[col]->l_vec[rrow] = l_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GeoDaConst::double_type: {
			double d_val;
			if (!value.ToDouble(&d_val)) {
				col_data[col]->undefined[rrow] = true;
			} else if (!boost::math::isfinite<double>(d_val)) {
				col_data[col]->undefined[rrow] = true;
			}
			if (!col_data[col]->undefined[rrow]) {
				// limit d_val to acceptable range
				int d = col_data[col]->decimals;
				int fl = col_data[col]->field_len; 
				double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				double min_val = DbfFileUtils::GetMinDouble(fl, d);
				if (max_val < d_val) {
					d_val = max_val;
				} else if (min_val > d_val) {
					d_val = min_val;
				}
			}
			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[rrow]) {
					col_data[col]->d_vec[rrow] = 0;
				} else {
					col_data[col]->d_vec[rrow] = d_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%#*.*f", field_len, col_data[col]->decimals,
							d_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
					if (!DbfColContainer::sprintf_period_for_decimal()) {
						for (int j=0; j<field_len; j++) {
							if (buf[j] == ',') buf[j] = '.';
						}
					}					
				}
			}
			break;
		}
		case GeoDaConst::string_type: {
			if (col_data[col]->vector_valid) {
				if (value.Length() > field_len) {
					col_data[col]->s_vec[rrow] = value.Mid(0, field_len);	
				} else {
					col_data[col]->s_vec[rrow] = value;
				}
			}
			if (col_data[col]->raw_data) {
				if (value.IsEmpty()) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					strncpy(buf, (const char*)value.mb_str(wxConvUTF8),
							field_len);
					buf[field_len]='\0';
				}
			}
			break;
		}
		default:
			break;
	}
	changed_since_last_save = true;
}

bool DbfGridTableBase::ColNameExists(const wxString& name)
{
	return (FindColId(name) != wxNOT_FOUND);
}

/** Returns the column id in the underlying grid, not the visual grid
 displayed order.  wxNOT_FOUND is returned if not found.  Always
 returns the first result found. */
int DbfGridTableBase::FindColId(const wxString& name)
{
	wxString c_name = name;
	c_name.Trim(false);
	c_name.Trim(true);
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (c_name.CmpNoCase(col_data[i]->name) == 0) return i;
	}
	return wxNOT_FOUND;
}

void DbfGridTableBase::PrintTable()
{
	for (int row=0, row_end=GetNumberRows(); row<row_end; row++) {
		wxString msg = "   ";
		for (int col=0, col_end=col_data.size(); col<col_end; col++) {
			col_data[col]->CopyRawDataToVector();
			switch (col_data[col]->type) {
				case GeoDaConst::date_type:
					msg << col_data[col]->l_vec[row] << " ";
					break;	
				case GeoDaConst::long64_type:
					msg << col_data[col]->l_vec[row] << " ";
					break;
				case GeoDaConst::double_type:
					msg << col_data[col]->d_vec[row] << " ";
					break;
				case GeoDaConst::string_type:
					msg << col_data[col]->s_vec[row] << " ";
					break;
				default:
					break;
			}
		}
		LOG_MSG(msg);
	}
}

bool DbfGridTableBase::WriteToDbf(const wxString& fname, wxString& err_msg)
{
	std::ofstream out_file;
	out_file.open(fname.mb_str(wxConvUTF8), std::ios::out | std::ios::binary);
	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Error: Problem opening \"" + fname + "\"";
		return false;
	}

	dbf_file_name_no_ext = wxFileName(fname).GetName();
	
	// Ensure that raw_data exists.  If raw_data exists, then each item is
	// assumed to be ready for writing to disk.
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (!col_data[i]->raw_data) col_data[i]->CopyVectorToRawData();
	}

	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	// We must write the DBF in the current displayed column order
	std::vector<int> col_id_map;
	FillColIdMap(col_id_map);
	
	// update orig_header
	orig_header.num_records = GetNumberRows();
	orig_header.num_fields = GetNumberCols();
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header.header_length = 32 + orig_header.num_fields*32 + 1;
	orig_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	for (int i=0; i<orig_header.num_fields; i++) {
		orig_header.length_each_record += col_data[i]->field_len;
	}
	DbfFileHeader header = orig_header;
	
	wxUint32 u_int32;
	wxUint32* u_int32p = &u_int32;
	wxUint16 u_int16;
	wxUint16* u_int16p = &u_int16;
	wxUint8 u_int8;
	wxUint8* u_int8p = &u_int8;
	char membyte;
	
	// byte 0
	membyte = header.version;
	out_file.put(membyte);
	
	// byte 1
	membyte = (char) (header.year - 1900);
	out_file.put(membyte);
	
	// byte 2
	membyte = (char) header.month;
	out_file.put(membyte);
	
	// byte 3
	membyte = (char) header.day;
	out_file.put(membyte);

	// byte 4-7
	u_int32 = header.num_records;
	u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
	out_file.write((char*) u_int32p, 4);
	
	// byte 8-9
	u_int16 = header.header_length;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 10-11
	u_int16 = header.length_each_record;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 12-13 (0x0000)
	u_int16 = 0x0;
	out_file.write((char*) u_int16p, 2);
	
	// bytes 14-31: write 0 
	membyte = 0;
	for (int i=0; i<(31-14)+1; i++) out_file.put(membyte);
	
	// out_file now points to byte 32, which is the beginning of the list
	// of fields.  There must be at least one field.  Each field descriptor
	// is 32 bytes long with byte 0xd following the last field descriptor.
	char* byte32_buff = new char[32];
	for (int i=0; i<header.num_fields; i++) {
		int mi = col_id_map[i];
		for (int j=0; j<32; j++) byte32_buff[j] = 0;
		strncpy(byte32_buff,
				(const char*)col_data[mi]->name.mb_str(wxConvUTF8), 11);
		switch (col_data[mi]->type) {
			case GeoDaConst::date_type:
				byte32_buff[11] = 'D';
				break;
			case GeoDaConst::long64_type:
				byte32_buff[11] = 'N';
				break;
			case GeoDaConst::double_type:
				byte32_buff[11] = 'N';
				break;
			default:
				byte32_buff[11] = 'C';
				break;
		}
		byte32_buff[16] = (wxUint8) col_data[mi]->field_len;
		byte32_buff[17] = (wxUint8) col_data[mi]->decimals;
		out_file.write(byte32_buff, 32);
	}
	delete [] byte32_buff;
	// mark and of field descriptors with 0x0D
	out_file.put((char) 0x0D);
	
	// Write out each record
	for (int row=0; row<header.num_records; row++) {
		out_file.put((char) 0x20); // each record starts with a space character
		for (int col=0; col<header.num_fields; col++) {
			int mcol = col_id_map[col];
			int f_len = col_data[mcol]->field_len;
			out_file.write(col_data[mcol]->raw_data + row*(f_len+1), f_len);
		}
	}
	// 0x1A is the EOF marker
	out_file.put((char) 0x1A);
	out_file.close();
	changed_since_last_save = false;
	
	return true;
}

bool DbfGridTableBase::IsSelected(int row)
{
	return hs[row];
}

void DbfGridTableBase::Select(int row)
{
	//LOG_MSG(wxString::Format("selecting %d", (int) row));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyHighlighted(0, row);
	highlight_state->SetTotalNewlyHighlighted(1);
	highlight_state->SetTotalNewlyUnhighlighted(0);
	highlight_state->notifyObservers();	
}

void DbfGridTableBase::Deselect(int row)
{
	//LOG_MSG(wxString::Format("deselecting %d", (int) row));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyUnhighlighted(0, row);
	highlight_state->SetTotalNewlyHighlighted(0);
	highlight_state->SetTotalNewlyUnhighlighted(1);
	highlight_state->notifyObservers();	
}

/** Only wxGrid should call this, others should use Selected(int row) */
bool DbfGridTableBase::FromGridIsSelected(int row)
{
	return hs[row_order[row]];
}

/** Only wxGrid should call this, others should use Select(int row) */
void DbfGridTableBase::FromGridSelect(int row)
{
	//LOG_MSG(wxString::Format("selecting %d", (int) row_order[row]));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyHighlighted(0, row_order[row]);
	highlight_state->SetTotalNewlyHighlighted(1);
	highlight_state->SetTotalNewlyUnhighlighted(0);
	highlight_state->notifyObservers();
}

/** Only wxGrid should call this, others should use Deselect(int row) */
void DbfGridTableBase::FromGridDeselect(int row)
{
	//LOG_MSG(wxString::Format("deselecting %d", (int) row_order[row]));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyUnhighlighted(0, row_order[row]);
	highlight_state->SetTotalNewlyHighlighted(0);
	highlight_state->SetTotalNewlyUnhighlighted(1);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::SelectAll()
{
	highlight_state->SetEventType(HighlightState::highlight_all);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::DeselectAll()
{
	highlight_state->SetEventType(HighlightState::unhighlight_all);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::InvertSelection()
{
	highlight_state->SetEventType(HighlightState::invert);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::SortByDefaultDecending()
{
	LOG_MSG("Calling DbfGridTableBase::SortByDefaultDecending");
	for (int i=0; i<rows; i++) {
		row_order[i] = i;
	}
	sorting_ascending = false;
	sorting_col = -1;
}

void DbfGridTableBase::SortByDefaultAscending()
{
	LOG_MSG("Calling DbfGridTableBase::SortByDefaultAscending");
	int last_ind = rows-1;
	for (int i=0; i<rows; i++) {
		row_order[i] = last_ind - i;
	}
	sorting_ascending = true;
	sorting_col = -1;
}


template <class T>
class index_pair
{
public:
	int index;
	T val;
	static bool less_than(const index_pair& i,
						   const index_pair& j) {
		return (i.val<j.val);
	}
	static bool greater_than (const index_pair& i,
							  const index_pair& j) {
		return (i.val>j.val);
	}
};

void DbfGridTableBase::SortByCol(int col, bool ascending)
{
	if (col == -1) {
		if (ascending) {
			SortByDefaultAscending();
		} else {
			SortByDefaultDecending();
		}
		return;
	}
	sorting_ascending = ascending;
	sorting_col = col;
	int rows = GetNumberRows();
	
	switch (col_data[col]->type) {
		case GeoDaConst::date_type:
		case GeoDaConst::long64_type:
		{
			std::vector<wxInt64> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<wxInt64> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxInt64>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxInt64>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		case GeoDaConst::double_type:
		{
			std::vector<double> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<double> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<double>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<double>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		case GeoDaConst::string_type:
		{
			std::vector<wxString> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<wxString> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxString>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxString>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		default:
			break;
	}
}

void DbfGridTableBase::MoveSelectedToTop()
{
	LOG_MSG("Entering DbfGridTableBase::MoveSelectedToTop");
	std::set<int> sel_set;
	for (int i=0, iend=rows; i<iend; i++) {
		if (hs[row_order[i]]) sel_set.insert(row_order[i]);
	}
	int sel_row_offset = 0;
	int unsel_row_offset = sel_set.size();
	for (int i=0; i<rows; i++) {
		if (sel_set.find(i) != sel_set.end()) {
			row_order[sel_row_offset++] = i;
		} else {
			row_order[unsel_row_offset++] = i;
		}
	}
	sorting_col = -1;
	LOG_MSG("Exiting DbfGridTableBase::MoveSelectedToTop");	
}

void DbfGridTableBase::ReorderCols(const std::vector<int>& col_order)
{
	LOG_MSG("Entering DbfGridTableBase::ReorderCols");
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG(col_order[i]);
	}
	if (col_order.size() != col_data.size()) return;
	if (sorting_col != -1) sorting_col = col_order[sorting_col];
	std::vector<DbfColContainer*> orig(col_data);
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG_MSG(wxString::Format("col_data[%d] = orig[%d]",
								 col_order[i], i));
		col_data[col_order[i]] = orig[i];
	}
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG(col_data[i]->name);
	}
	changed_since_last_save = true;
	LOG_MSG("Entering DbfGridTableBase::ReorderCols");
}

/** If there is an associated wxGrid, then return the column ids in the order
 they are displayed in the table.  Otherwise, just return 0, 1, 2, ... The
 vector is automatically resized to col_data.size() 
 A mapping from displayed col order to actual col ids in table
 Eg, in underlying table, we might have A, B, C, D, E, F,
 but because of user wxGrid col reorder operaions might see these
 as C, B, A, F, D, E.  In this case, the col_map would be
 0->2, 1->1, 2->0, 3->5, 4->3, 5->4  */
void DbfGridTableBase::FillColIdMap(std::vector<int>& col_map)
{
	col_map.resize(col_data.size());
	wxGrid* grid = GetView();
	if (grid) {
		for (int i=0, e=col_map.size(); i<e; i++) col_map[grid->GetColPos(i)]=i;
	} else {
		for (int i=0, e=col_map.size(); i<e; i++) col_map[i] = i;
	}
}

/** Similar to FillColIdMap except this is a map of numeric type columns
 only.  The size of the resulting corresponds to the number of numeric
 columns */
void DbfGridTableBase::FillNumericColIdMap(std::vector<int>& col_map)
{
	std::vector<int> t;
	FillColIdMap(t);
	int numeric_cnt = 0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type ||
			col_data[t[i]]->type == GeoDaConst::double_type) {
			numeric_cnt++;
		}
	}
	col_map.resize(numeric_cnt);
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type ||
			col_data[t[i]]->type == GeoDaConst::double_type) {
			col_map[cnt++] = t[i];
		}
	}
}	

bool DbfGridTableBase::InsertCol(int pos, GeoDaConst::FieldType type,
								 const wxString& name,
								 int field_len, int decimals,
								 int displayed_decimals,
								 bool alloc_raw_data,
								 bool mark_all_defined)
{
	LOG_MSG(wxString::Format("Inserting column into table at postion %d", pos));
	if (pos < 0 || pos > col_data.size()) return false;
	std::vector<DbfColContainer*> orig(col_data);
	col_data.resize(orig.size()+1);
	for (int i=0; i<pos; i++) col_data[i] = orig[i];
	col_data[pos] = new DbfColContainer(this);
	if (type == GeoDaConst::date_type) {
		// will leave unitialized
		col_data[pos]->Init(rows, type, name, field_len, decimals, decimals,
							alloc_raw_data, !alloc_raw_data, false);
		col_data[pos]->undefined_initialized = true;
	} else {
		col_data[pos]->Init(rows, type, name, field_len, decimals, decimals,
							alloc_raw_data, !alloc_raw_data, mark_all_defined);
	}
	for (int i=pos; i<orig.size(); i++) col_data[i+1] = orig[i];
	if (pos <= sorting_col) sorting_col++;
	LOG(col_data.size());
	
	if (GetView()) {
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_INSERTED, pos, 1 );
		GetView()->ProcessTableMessage( msg );
		if (type == GeoDaConst::long64_type) {
			GetView()->SetColFormatNumber(pos);
		} else if (type == GeoDaConst::double_type) {
			GetView()->SetColFormatFloat(pos, -1,
						GenUtils::min<int>(decimals, displayed_decimals));
		} else {
			// leave as a string
		}
	}
	
	changed_since_last_save = true;
	return true;
}

bool DbfGridTableBase::DeleteCol(int pos)
{
	LOG_MSG(wxString::Format("Deleting column from table at postion %d", pos));
	if (pos < 0 || pos >= col_data.size() || col_data.size() == 0) return false;
	std::vector<DbfColContainer*> orig(col_data);
	col_data.resize(orig.size()-1);
	for (int i=0; i<pos; i++) col_data[i] = orig[i];
	delete col_data[pos];
	for (int i=pos+1; i<orig.size(); i++) col_data[i-1] = orig[i];
	if (pos == sorting_col) { sorting_col = -1; sorting_ascending = true; }
	if (pos < sorting_col) sorting_col--;
	
	if (GetView()) {
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED, pos, 1 );
		GetView()->ProcessTableMessage( msg );
	}
	changed_since_last_save = true;	
	return true;
}

wxString DbfGridTableBase::GetRowLabelValue(int row)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	return wxString::Format("%d", row_order[row]+1);
}

wxString DbfGridTableBase::GetColLabelValue(int col)
{
	// \uXXXX are unicode characters for up and down arrows
	// \u0668 Arabic-Indic digit eight (up pointing arrow)
	// \u0667 Arabic-Indic digit seven (down pointing arrow)
	// \u25B5 white small up-pointing triangle (too big on OSX)
	// \u25BF white small down-pointing triangle
	// \u25B4 black small up-pointing triangle  (too big on OSX)
	// \u25BE black small down-pointing triangle
	// \u02C4, \u02C5 are up and down-pointing arrows
	// \u27F0 upward quadruple arrow
	// \u27F1 downward quadruble arrow

	if (col<0 || col>col_data.size()) return wxEmptyString;
	
	wxString label(col_data[col]->name);
	if (col == sorting_col) {
		if (GeneralWxUtils::isMac()) {
			label << (sorting_ascending ? " \u02C4" : " \u02C5");
		} else if (GeneralWxUtils::isUnix()) {
			label << (sorting_ascending ? " \u25B5" : " \u25BF");
		} else {
			label << (sorting_ascending ? " >" : " <");
		}
	}
	return label;
}

