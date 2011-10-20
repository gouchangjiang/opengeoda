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

#include <sstream>
#include <boost/functional/hash.hpp>
#include "ShpFile.h"

bool Shapefile::operator==(Point const& a, Point const& b)
{
	return a.x == b.x && a.y == b.y;
}

std::size_t Shapefile::hash_value(Point const& p)
{
	std::size_t seed=0;
	boost::hash_combine(seed, p.x);
	boost::hash_combine(seed, p.y);
	return seed;
}

bool Shapefile::operator==(Edge const& e1, Edge const& e2)
{
	return e1.a == e2.a && e1.b == e2.b;
}

std::size_t Shapefile::hash_value(Edge const& e)
{
	std::size_t seed=0;
	boost::hash_combine(seed, e.a.x);
	boost::hash_combine(seed, e.a.y);
	boost::hash_combine(seed, e.b.x);
	boost::hash_combine(seed, e.b.y);
	return seed;
}


int Shapefile::calcNumIndexHeaderRecords(const Shapefile::Header& header)
{
  // The header length is 50, 16-byte words, and header.file_length records
  // the number of 16-byte words.  So, (header.file_length - 50) is the number
  // of non-header 16-byte words.  Since each header record is 16-bytes long,
  // we divide by 4 to get the number of records. 
  return (header.file_length - 50)/4;
}

bool Shapefile::populatePointMainRecords(std::vector<MainRecord>& mr,
					 std::ifstream& file)
{
  bool success = true;
  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;
  int start_seek_pos = 100; // beginning of data
  file.seekg(start_seek_pos, std::ios::beg);
  int total_records = mr.size();
  int rec_num;
  for (int i=0; i<total_records && success; i++) {
    file.read((char*) integer32p, 4);
    rec_num = myINT_SWAP_ON_LE(integer32);
    if (rec_num < 1 || rec_num > total_records || 
	mr[rec_num-1].header.record_number != 0) {
      success = false;
    } else { // we have a non-duplicated, valid record number
      mr[rec_num-1].header.record_number = rec_num;
      file.read((char*) integer32p, 4);
      mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
      
      PointContents* pc = dynamic_cast<PointContents*>(mr[rec_num-1].contents_p);
      file.read((char*) integer32p, 4);
      pc->shape_type = myINT_SWAP_ON_BE(integer32);

      file.read((char*) float64p, 8);
      pc->x =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->y =  myDOUBLE_SWAP_ON_BE(float64);
    }
  }
  return success;
}


bool Shapefile::populatePolyLineMainRecords(std::vector<MainRecord>& mr,
					   std::ifstream& file)
{
  bool success = true;
  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;
  int start_seek_pos = 100; // beginning of data
  file.seekg(start_seek_pos, std::ios::beg);
  int total_records = mr.size();
  int rec_num;
  for (int i=0; i<total_records && success; i++) {
    file.read((char*) integer32p, 4);
    rec_num = myINT_SWAP_ON_LE(integer32);
    if (rec_num < 1 || rec_num > total_records || 
	mr[rec_num-1].header.record_number != 0) {
      success = false;
    } else { // we have a non-duplicated, valid record number
      mr[rec_num-1].header.record_number = rec_num;
      file.read((char*) integer32p, 4);
      mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
      
      PolyLineContents* pc =
	dynamic_cast<PolyLineContents*>(mr[rec_num-1].contents_p);
      file.read((char*) integer32p, 4);
      pc->shape_type = myINT_SWAP_ON_BE(integer32);

      file.read((char*) float64p, 8);
      pc->box[0] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[1] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[2] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[3] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) integer32p, 4);
      pc->num_parts = myINT_SWAP_ON_BE(integer32);
      pc->parts.resize(pc->num_parts);

      file.read((char*) integer32p, 4);
      pc->num_points = myINT_SWAP_ON_BE(integer32);
      pc->points.resize(pc->num_points);
      
      for (int j=0; j < pc->num_parts; j++) {
	file.read((char*) integer32p, 4);
	pc->parts[j] = myINT_SWAP_ON_BE(integer32);
      }

      for (int j=0; j < pc->num_points; j++) {
	file.read((char*) float64p, 8);
	pc->points[j].x =  myDOUBLE_SWAP_ON_BE(float64);

	file.read((char*) float64p, 8);
	pc->points[j].y =  myDOUBLE_SWAP_ON_BE(float64);
      }

    }
  }
  return success;
}

bool Shapefile::populatePolygonMainRecords(std::vector<MainRecord>& mr,
					   std::ifstream& file)
{
  bool success = true;
  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;
  int start_seek_pos = 100; // beginning of data
  file.seekg(start_seek_pos, std::ios::beg);
  int total_records = mr.size();
  int rec_num;
  for (int i=0; i<total_records && success; i++) {
    file.read((char*) integer32p, 4);
    rec_num = myINT_SWAP_ON_LE(integer32);
    if (rec_num < 1 || rec_num > total_records || 
	mr[rec_num-1].header.record_number != 0) {
      success = false;
    } else { // we have a non-duplicated, valid record number
      mr[rec_num-1].header.record_number = rec_num;
      file.read((char*) integer32p, 4);
      mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
      
      PolygonContents* pc = dynamic_cast<PolygonContents*>(mr[rec_num-1].contents_p);
      file.read((char*) integer32p, 4);
      pc->shape_type = myINT_SWAP_ON_BE(integer32);

      file.read((char*) float64p, 8);
      pc->box[0] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[1] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[2] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) float64p, 8);
      pc->box[3] =  myDOUBLE_SWAP_ON_BE(float64);

      file.read((char*) integer32p, 4);
      pc->num_parts = myINT_SWAP_ON_BE(integer32);
      pc->parts.resize(pc->num_parts);

      file.read((char*) integer32p, 4);
      pc->num_points = myINT_SWAP_ON_BE(integer32);
      pc->points.resize(pc->num_points);
      
      for (int j=0; j < pc->num_parts; j++) {
	file.read((char*) integer32p, 4);
	pc->parts[j] = myINT_SWAP_ON_BE(integer32);
      }

      for (int j=0; j < pc->num_points; j++) {
	file.read((char*) float64p, 8);
	pc->points[j].x =  myDOUBLE_SWAP_ON_BE(float64);

	file.read((char*) float64p, 8);
	pc->points[j].y =  myDOUBLE_SWAP_ON_BE(float64);
      }

    }
  }
  return success;
}


bool Shapefile::populateIndex(const std::string& fname,
							  Shapefile::Index& index_s)
{
  bool success = populateHeader(fname, index_s.header);
  if (!success) return false;

  std::ifstream file;
  file.open(fname.c_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) {
    return false;
  }

  int total_index_records = calcNumIndexHeaderRecords(index_s.header);
  index_s.records.resize(total_index_records);

  int start_seek_pos = 100; // beginning of data

  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;

  file.seekg(start_seek_pos, std::ios::beg);
  for (int i=0; i<total_index_records; i++) {
    file.read((char*) integer32p, 4);
    index_s.records[i].offset = myINT_SWAP_ON_LE(integer32);
    file.read((char*) integer32p, 4);
    index_s.records[i].content_length = myINT_SWAP_ON_LE(integer32);
  }

  file.close();
  return true;
}

bool Shapefile::populateHeader(const std::string& fname,
							   Shapefile::Header& header)
{
  std::ifstream file;
  file.open(fname.c_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) {
    return false;
  }
  
  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;

  file.seekg(0, std::ios::beg);
  file.read((char*) integer32p, 4); // from byte 0
  header.file_code = myINT_SWAP_ON_LE(integer32);

  file.seekg(24, std::ios::beg);
  file.read((char*) integer32p, 4); // from byte 24
  header.file_length = myINT_SWAP_ON_LE(integer32);

  file.read((char*) integer32p, 4); // from byte 28
  header.version = myINT_SWAP_ON_BE(integer32);

  file.read((char*) integer32p, 4); // from byte 32
  header.shape_type = myINT_SWAP_ON_BE(integer32);

  file.read((char*) float64p, 8); // from byte 36
  header.bbox_x_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 44
  header.bbox_y_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 52
  header.bbox_x_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 60
  header.bbox_y_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 68
  header.bbox_z_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 76
  header.bbox_z_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 84
  header.bbox_m_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 92
  header.bbox_m_max = myDOUBLE_SWAP_ON_BE(float64);

  file.close();

  return true;
}

std::string Shapefile::shapeTypeToString(int st)
{
  using namespace Shapefile;
  switch (st) {
  case NULL_SHAPE:
    return "NULL_SHAPE";
  case POINT:
    return "POINT";
  case POLY_LINE:
    return "POLY_LINE";
  case POLYGON:
    return "POLYGON";
  case MULTI_POINT:
    return "MULTI_POINT";
  case POINT_Z:
    return "POINT_Z";
  case POLY_LINE_Z:
    return "POLY_LINE_Z";
  case POLYGON_Z:
    return "POLYGON_Z";
  case MULTI_POINT_Z:
    return "MULTI_POINT_Z";
  case POINT_M:
    return "POINT_M";
  case POLY_LINE_M:
    return "POLY_LINE_M";
  case POLYGON_M:
    return "POLYGON_M";
  case MULTI_POINT_M:
    return "MULTI_POINT_M";
  case MULTI_PATCH:
    return "MULTI_PATCH";
  default :
    return "";
  }
}

std::string Shapefile::getIndentString(int indent)
{
  std::string a("");
  std::string b("");
  for (int i=0; i<spaces_per_indent; i++) a += " ";
  for (int i=0; i<indent; i++) b += a;
  return b;
}

void Shapefile::printHeader(const Shapefile::Header& header, std::ostream& s,
			    int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "file_code: " << header.file_code << std::endl;
  s << pre << "file_length: " << header.file_length
    << " (16-bit words)" << std::endl;
  s << pre << "file_length: " << header.file_length/2
    << " (32-bit words)" << std::endl;
  s << pre << "version: " << header.version << std::endl;
  s << pre << "shape_type: "
    << Shapefile::shapeTypeToString(header.shape_type) << std::endl;
  s << pre << "bbox_x_min: " << header.bbox_x_min << std::endl;
  s << pre << "bbox_y_min: " << header.bbox_y_min << std::endl;
  s << pre << "bbox_x_max: " << header.bbox_x_max << std::endl;
  s << pre << "bbox_y_max: " << header.bbox_y_max << std::endl;
  s << pre << "bbox_z_min: " << header.bbox_z_min << std::endl;
  s << pre << "bbox_z_max: " << header.bbox_z_max << std::endl;
  s << pre << "bbox_m_min: " << header.bbox_m_min << std::endl;
  s << pre << "bbox_m_max: " << header.bbox_m_max << std::endl;
}

void Shapefile::printIndexRecord(const Shapefile::IndexRecord& ir,
				 std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "offset: " << ir.offset << std::endl;
  s << pre << "content_length: " << ir.content_length << std::endl;
}

void Shapefile::printMainRecord(const Shapefile::MainRecord& mr,
								std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "header:" << std::endl;
  printMainRecordHeader(mr.header, s, indent+1);
  s << pre << "contents_p:" << std::endl;
  printRecordContents(mr.contents_p, s, indent+1);
}

void Shapefile::printMainRecordHeader(const Shapefile::MainRecordHeader& mrh,
				      std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "record_number: " << mrh.record_number << std::endl;
  s << pre << "content_length: " << mrh.content_length << std::endl;
}

void Shapefile::printRecordContents(const Shapefile::RecordContents* rc,
				    std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  if (const PointContents* p = dynamic_cast<const PointContents*>(rc)) {
    printPointContents(*p, s, indent);
  } else if (const PolyLineContents* p =
			 dynamic_cast<const PolyLineContents*>(rc)) {
    printPolyLineContents(*p, s, indent);
  } else if (const PolygonContents* p =
			 dynamic_cast<const PolygonContents*>(rc)) {
    printPolygonContents(*p, s, indent);
  }
}

void Shapefile::printPolygonContents(const Shapefile::PolygonContents& pc,
				     std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
  s << pre << "box: " << boxToString(pc.box) << std::endl;
  s << pre << "num_parts: " << pc.num_parts << std::endl;
  s << pre << "num_points: " << pc.num_points << std::endl;
  s << pre << "parts(" << pc.num_parts << "): " << std::endl;
  pre = getIndentString(indent+1);
  for (int i=0; i < pc.num_parts; i++) {
    s << pre << "parts[" << i << "]: " << pc.parts.at(i) << std::endl;
  }
  pre = getIndentString(indent);
  s << pre << "points(" << pc.num_points << "): " << std::endl;
  pre = getIndentString(indent+1);
  for (int i=0; i < pc.num_points; i++) {
    s << pre << "points[" << i << "]: "
      << pointToString(pc.points.at(i)) << std::endl;
  }
}

void Shapefile::printPolyLineContents(const Shapefile::PolyLineContents& pc,
				      std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
}

void Shapefile::printPointContents(const Shapefile::PointContents& pc,
				   std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
  s << pre << "(x,y): " << pc.x << ", " << pc.y << ")" << std::endl;
}

std::string Shapefile::pointToString(const Shapefile::Point& p)
{
  std::ostringstream s;
  s << "(" << p.x << ", " << p.y << ")";
  return s.str();
}

std::string Shapefile::edgeToString(const Shapefile::Edge& e)
{
	std::ostringstream s;
	s << "{" << pointToString(e.a) << ", " << pointToString(e.b) << "}";
	return s.str();
}

std::string Shapefile::boxToString(const std::vector<wxFloat64>& box)
{
  if (box.size() < 4) return "";
  std::ostringstream s;
  s << "(" << box[0] << ", " << box[1] << ", " << box[2]
    << ", " << box[3] << ")";
  return s.str();
}

void Shapefile::printIndex(const Shapefile::Index& index_s, std::ostream& s,
			   int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "Header:" << std::endl;
  Shapefile::printHeader(index_s.header, s, indent+1);
  s << pre << "IndexRecord(" << index_s.records.size() << "):" << std::endl;
  for ( int i=0, iend=index_s.records.size(); i<iend; i++) {
    s << getIndentString(1+indent) << "records[" << i << "]:" << std::endl;
    Shapefile::printIndexRecord(index_s.records[i], s, indent+2);
  }
}

void Shapefile::printMain(const Shapefile::Main& main_s, std::ostream& s,
			  int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "Header:" << std::endl;
  Shapefile::printHeader(main_s.header, s, indent+1);
  s << pre << "MainRecord(" << main_s.records.size() << "):" << std::endl;
  for ( int i=0, iend=main_s.records.size(); i<iend; i++) {
    s << getIndentString(1+indent) << "records[" << i << "]:" << std::endl;
    Shapefile::printMainRecord(main_s.records[i], s, indent+2);
  }
}

bool Shapefile::populateMain(const Index& index_s, const std::string& fname,
			     Main& main_s)
{
  using namespace Shapefile;
  bool success = populateHeader(fname, main_s.header);
  if (!success) return false;
  
  std::ifstream file;
  file.open(fname.c_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) {
    return false;
  }

  if ( main_s.header.shape_type == POINT ||
       main_s.header.shape_type == POLY_LINE ||
       main_s.header.shape_type == POLYGON ) {

    // Allocate memory as needed and put all records in their proper sorted
	// order.
    main_s.records.resize(index_s.records.size());
    if (main_s.header.shape_type == POINT) {
      for (int i=0, iend=main_s.records.size(); i<iend; i++) 
	main_s.records[i].contents_p = new PointContents();
      populatePointMainRecords(main_s.records, file);
    } else if ( main_s.header.shape_type == POLY_LINE ) {
      for (int i=0, iend=main_s.records.size(); i<iend; i++)
	main_s.records[i].contents_p = new PolyLineContents();
      populatePolyLineMainRecords(main_s.records, file);      
    } else if ( main_s.header.shape_type == POLYGON ) {
      for (int i=0, iend=main_s.records.size(); i<iend; i++)
	main_s.records[i].contents_p = new PolygonContents();
      populatePolygonMainRecords(main_s.records, file);
    }
    
  } else {
    success = false;
  }
  
  file.close();
  return success;
}

/** the main_f file stream must be open already. */
bool Shapefile::populatePolygonContents(PolygonContents& pc, int rec_id,
										Index& index, std::ifstream& main_f)
{
	using namespace std;
	if ( 0 > rec_id || rec_id >= calcNumIndexHeaderRecords(index.header) ) {
		cout << "Error: rec_id " << rec_id << " is out of range (0,";
		cout << calcNumIndexHeaderRecords(index.header) << ")." << endl;
		return false;
	}
	if (!(main_f.is_open() && main_f.good())) {
		cout << "Error: main_f file stream not open or on bad state." << endl;
		return false;
	}
	
	// offset in bytes (rather than 16-bit words)
	int rec_offset = index.records[rec_id].offset * 2;
	//cout << "rec_offset: " << rec_offset << endl;
	// content length in 16-bit words
	int rec_content_len = index.records[rec_id].content_length;
	//cout << "rec_content_len: " << rec_content_len << endl;
	
	int start_seek_pos = rec_offset; // beginning of record data
	
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	
	/*
	char char8;
	char* char8p = &char8;
	cout.unsetf(ios::dec | ios::oct);
	cout.setf(ios::hex | ios::uppercase | ios::showbase);
	main_f.seekg(start_seek_pos, std::ios::beg);
	main_f.read(char8p, 1); integer32 = (unsigned char) char8;
	cout << "first byte val: " << integer32 << endl; 
	main_f.read(char8p, 1); integer32 = (unsigned char) char8;
	cout << "second byte val: " << integer32 << endl;
	main_f.read(char8p, 1); integer32 = (unsigned char) char8;
	cout << "third byte val: " << integer32 << endl;
	main_f.read(char8p, 1); integer32 = (unsigned char) char8;
	cout << "forth byte val: " << integer32 << endl;
	cout.unsetf(ios::hex | ios::uppercase | ios::showbase);
	cout.setf(ios::dec);
	*/
	 
	main_f.seekg(start_seek_pos, std::ios::beg);
	wxInt32 m_rec_num;
	main_f.read((char*) integer32p, 4);
	m_rec_num = myINT_SWAP_ON_LE(integer32);
	if (m_rec_num-1 != rec_id) {
		cout << "Error: read record id " << m_rec_num-1 << " does not match ";
		cout << "requested id " << rec_id << "." << endl;
		return false;
	}
	main_f.read((char*) integer32p, 4);
	wxInt32 m_content_length = myINT_SWAP_ON_LE(integer32);
	if (m_content_length != rec_content_len) {
		cout << "Error: read content length " << m_content_length;
		cout << " does not match ";
		cout << "requested content length " << rec_content_len << "." << endl;
		return false;
	}
	
	main_f.read((char*) integer32p, 4);
	pc.shape_type = myINT_SWAP_ON_BE(integer32);
			
	main_f.read((char*) float64p, 8);
	pc.box[0] =  myDOUBLE_SWAP_ON_BE(float64);
			
	main_f.read((char*) float64p, 8);
	pc.box[1] =  myDOUBLE_SWAP_ON_BE(float64);
		
	main_f.read((char*) float64p, 8);
	pc.box[2] =  myDOUBLE_SWAP_ON_BE(float64);
			
	main_f.read((char*) float64p, 8);
	pc.box[3] =  myDOUBLE_SWAP_ON_BE(float64);
			
	main_f.read((char*) integer32p, 4);
	pc.num_parts = myINT_SWAP_ON_BE(integer32);
	pc.parts.resize(pc.num_parts);
			
	main_f.read((char*) integer32p, 4);
	pc.num_points = myINT_SWAP_ON_BE(integer32);
	pc.points.resize(pc.num_points);
			
	for (int j=0; j < pc.num_parts; j++) {
		main_f.read((char*) integer32p, 4);
		pc.parts[j] = myINT_SWAP_ON_BE(integer32);
	}
			
	for (int j=0; j < pc.num_points; j++) {
		main_f.read((char*) float64p, 8);
		pc.points[j].x =  myDOUBLE_SWAP_ON_BE(float64);
				
		main_f.read((char*) float64p, 8);
		pc.points[j].y =  myDOUBLE_SWAP_ON_BE(float64);
	}
			
	return true;
}


/** The following could define a run-time and relatively robust endianess test */
//bool isBigEndian() {
//  const int i = 1;
//  return (*(char*)&i) == 0;
//}

/** Alternately, to avoid a function call, could use the following macro */
const int i = 1;
#define is_bigendian() ( (*(char*)&i) == 0 )

int Shapefile::myINT_SWAP_ON_BE( int x )
{
	if is_bigendian() {
		char* c = (char*) &x;
		union {
			wxInt32 y;
			char c[4];
		} data;
		data.c[0] = c[3];
		data.c[1] = c[2];
		data.c[2] = c[1];
		data.c[3] = c[0];
		return data.y;
	}
	return x;
}

int Shapefile::myINT_SWAP_ON_LE( int x )
{
	if is_bigendian() {
		return x;
	}
	char* c = (char*) &x;
	union {
		wxInt32 y;
		char c[4];
	} data;
	data.c[0] = c[3];
	data.c[1] = c[2];
	data.c[2] = c[1];
	data.c[3] = c[0];
	return data.y;
}

wxFloat64 Shapefile::myDOUBLE_SWAP_ON_BE( wxFloat64 x )
{
  	if is_bigendian() {
		char* c = (char*) &x;
		union {
			wxFloat64 y;
			char c[8];
		} data;
		data.c[0] = c[7];
		data.c[1] = c[6];
		data.c[2] = c[5];
		data.c[3] = c[4];
		data.c[4] = c[3];
		data.c[5] = c[2];
		data.c[6] = c[1];
		data.c[7] = c[0];
		return data.y;
	}
	return x;
}
