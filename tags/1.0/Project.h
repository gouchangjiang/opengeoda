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

/*
 *  Notes: All Projects will have a Weights Manager and a DBF file with
 *         associated DbfGridTableBase instance.  Optionally, there can
 *         also be a Shapefile.  So, we should have two possible constructors:
 *         one that passes in a Shapefile name and DbfGridTableBase, and
 *         another that just passes in a DbfGridTableBase.  The Weights
 *         Manager will always be created by the Project instance itself.
 *
 */

#ifndef __GEODA_CENTER_PROJECT_H__
#define __GEODA_CENTER_PROJECT_H__

#undef check // macro undefine needed for Xcode compilation with Boost.Geometry
//#include <boost/geometry/geometry.hpp>
//#include <boost/geometry/geometries/point_xy.hpp>
#include "ShapeOperations/ShpFile.h"
#include "Generic/HighlightState.h"
#include <wx/filename.h>
#include <vector>
#include <set>

//using namespace boost::geometry;

class DbfGridTableBase;
class WeightsManager;
class DbfFileReader;
class FramesManager;

class Project {
public:
	Project(int num_records);
	void Init(DbfGridTableBase* grid_base, wxFileName shp_fname_s);
	void Init(DbfGridTableBase* grid_base);
	virtual ~Project();
	bool IsValid() { return is_project_valid; }
	wxString GetErrorMessage() { return open_error_message; }
	Shapefile::Main main_data;
	Shapefile::Index index_data;
	//std::vector<point_2d> polyCentroids;  // Polygon/PolyLine Centroids
	//std::vector<point_2d> polyMeanCenters; // Polygon/PolyLine Mean Centers
	//std::vector< std::list<polygon_2d> > polyLists; // Polygon parts lists
	wxString GetMainDir(); // returns the path with the trailing separator
	wxString GetMainName(); // name part only, no extension
	wxFileName shp_fname; // including full path
	HighlightState* highlight_state;
	double GetMapAspectRatio() { return map_aspect_ratio; }
    int GetNumRecords() { return num_records; }
	DbfGridTableBase* GetGridBase() { return grid_base; }
	WeightsManager* GetWManager() { return w_manager; }
	FramesManager* GetFramesManager() { return frames_manager; }
	void AddNeighborsToSelection();
	bool IsTableOnlyProject() { return table_only_project; }
	int GetProjectId() { return project_id; }
	// One-off for Regression Dialog only.
	wxWindow* regression_dlg;
	
protected:
	bool OpenShpFile(wxFileName shp_fname_s);
	bool is_project_valid; // true if project Shape File created successfully
	wxString open_error_message; // error message for project open failure.
	bool table_only_project; // true iff only grid_base was passed in
	// without Shapefile.
	static int next_project_id;
	int project_id;
	double map_aspect_ratio;
    int num_records;
	DbfGridTableBase* grid_base;
	WeightsManager* w_manager;
	FramesManager* frames_manager;
};

#endif

