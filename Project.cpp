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

#include <string>
#include <sstream>
#include <assert.h>
#include "logger.h"
#include "FramesManager.h"
#include "DataViewer/DbfGridTableBase.h"
#include "ShapeOperations/WeightsManager.h"
#include "ShapeOperations/GalWeight.h"
#include "ShapeOperations/DbfFile.h"
//#include "ShapeOperations/ShapeUtils.h"
#include "Project.h"

int Project::next_project_id = 0;

Project::Project(int num_records_s)
: project_id(next_project_id++),
grid_base(0), map_aspect_ratio(0),
table_only_project(false), is_project_valid(false),
num_records(num_records_s),
regression_dlg(0)
{
	frames_manager = new FramesManager();
	highlight_state = new HighlightState();
	highlight_state->SetSize(num_records);
	w_manager = new WeightsManager(num_records);	
}

void Project::Init(DbfGridTableBase* grid_base_s)
{
	grid_base = grid_base_s;
	table_only_project = true;
	is_project_valid = true;
}

void Project::Init(DbfGridTableBase* grid_base_s, wxFileName shp_fname)
{
	grid_base = grid_base_s;
	table_only_project = false;
	is_project_valid = true;
	OpenShpFile(shp_fname);
}

Project::~Project()
{
	LOG_MSG("Entering Project::~Project");
	if (w_manager) delete w_manager; w_manager = 0;
	//NOTE: the wxGrid instance in NewTableViewerFrame has
	// ownership and is therefore responsible for deleting the
	// grid_base when it closes.
	//if (grid_base) delete grid_base; grid_base = 0;
	LOG_MSG("Exiting Project::~Project");
}

bool Project::OpenShpFile(wxFileName shp_fname_s)
{
	LOG_MSG("Entering Project::OpenShpFile");
	using namespace std;
	using namespace Shapefile;
	
	shp_fname = shp_fname_s;
	wxFileName m_shx_fname(shp_fname);
	m_shx_fname.SetExt("shx");
	std::string m_shx_str(m_shx_fname.GetFullPath().mb_str());
	wxFileName m_shp_fname(shp_fname);
	m_shp_fname.SetExt("shp");
	std::string m_shp_str(m_shp_fname.GetFullPath().mb_str());
	
	Shapefile::populateIndex(m_shx_str, index_data);
	//std::ostringstream index_strm;
	//Shapefile::printIndex(index_data, index_strm);
	//wxString msg1(index_strm.str().c_str(), wxConvUTF8);
	//LOG_MSG(msg1);
	
	//Shapefile::populateMain(index_data, m_shp_str, main_data);

    //std::ostringstream main_strm;
	//Shapefile::printMain(main_data, main_strm);
	//wxString msg2(main_strm.str().c_str(), wxConvUTF8);
	//LOG_MSG(msg2);
	
    int shp_num_recs = Shapefile::calcNumIndexHeaderRecords(index_data.header);
    LOG(shp_num_recs);
	//LOG(main_data.records.size());

	map_aspect_ratio = 1;
	//map_aspect_ratio = ShapeUtils::CalcAspectRatio(index_data.header);
	//if (map_aspect_ratio == 0) return false;

	//if (main_data.header.shape_type == POLY_LINE ||
	//	main_data.header.shape_type == POLYGON ) {
		//polyCentroids.resize(main_data.records.size());
		//polyMeanCenters.resize(main_data.records.size());
	//}

//	if (main_data.header.shape_type == POINT ) {
//		for (int i=0, iend=main_data.records.size(); i<iend; i++) {
//			Shapefile::MainRecord& rec = main_data.records[i];
//			Shapefile::PointContents* pc_p =
//			dynamic_cast<Shapefile::PointContents*>(rec.contents_p);
//			assert(pc_p);
//		}
//	} else if (main_data.header.shape_type == POLYGON ) {
		//polyLists.resize(main_data.records.size());
//		for (int i=0, iend=main_data.records.size(); i<iend; i++) {
//			Shapefile::MainRecord& rec = main_data.records[i];
//			Shapefile::PolygonContents* pc_p =
//			dynamic_cast<Shapefile::PolygonContents*>(rec.contents_p);
//			assert(pc_p);
//			std::list<polygon_2d>* l_p = new std::list<polygon_2d>;
			//ShapeUtils::polygonContentsToPolyList(*pc_p, polyLists[i]);
			//polyCentroids[i] = ShapeUtils::centroid(polyLists[i]);
			//polyMeanCenters[i] = ShapeUtils::meanCenter(polyLists[i]);
//		}
//	}

	//LOG_MSG("polyList created:");
	//for (int i=0; i<polyLists.size(); i++) {
	//	std::ostringstream s;
	//	ShapeUtils::printPolyList(polyLists[i], s, 1);
	//	wxString msg(s.str().c_str(), wxConvUTF8);
	//	LOG_MSG(msg);
	//}
	
	//for (int i=0; i<polyLists.size(); i++) {
	//	std::ostringstream s1;
	//	s1 << dsv(polyCentroids[i]);
	//	wxString pt1(s1.str().c_str(), wxConvUTF8);
	//	std::ostringstream s2;
	//	s2 << dsv(polyMeanCenters[i]);
	//	wxString pt2(s2.str().c_str(), wxConvUTF8);		
	//	wxString msg("polyCentroids[");
	//	msg << i << "] = " << pt1 << ",  polyMeanCenters[";
	//	msg << i << "] = " << pt2;
	//	LOG_MSG(msg);
	//}
	
	// NOTE: clean this up
		
	LOG_MSG("Exiting Project::OpenShpFile");
	return true;
}


wxString Project::GetMainDir()
{
	if (table_only_project) {
		return grid_base->GetDbfFileName().GetPathWithSep();
	} else {
		return shp_fname.GetPathWithSep();
	}
}

wxString Project::GetMainName()
{
	if (table_only_project) {
		return grid_base->GetDbfFileName().GetName();
	} else {
		return shp_fname.GetName();
	}
}

void Project::AddNeighborsToSelection()
{
	if (!GetWManager() || (GetWManager() && !GetWManager()->GetCurrWeight())) {
		return;
	}
	LOG_MSG("Entering Project::AddNeighborsToSelection");
	GalWeight* gal_weights = 0;

	GeodaWeight* w = GetWManager()->GetCurrWeight();
	if (!w) {
		LOG_MSG("Warning: no current weight matrix found");
		return;
	}
	if (w->weight_type != GeodaWeight::gal_type) {
		LOG_MSG("Error: Only GAL type weights are currently supported. "
				"Other weight types are internally converted to GAL.");
		return;
	} else {
		gal_weights = (GalWeight*) w;
	}
	
	// go through the list of all objects in current selection
	// for each selected object and add each of its neighbor to
	// the list so long as it isn't already selected.	
	
	HighlightState& hs = *highlight_state;
	std::vector<bool>& h = hs.GetHighlight();
	std::vector<int>& nh = hs.GetNewlyHighlighted();
	std::vector<int>& nuh = hs.GetNewlyUnhighlighted();
	int nh_cnt = 0;
	std::vector<bool> add_elem(gal_weights->num_obs, false);
	
	for (int i=0; i<gal_weights->num_obs; i++) {
		if (h[i]) {
			GalElement& e = gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				int obs = e.elt(j);
				if (!h[obs] && !add_elem[obs]) {
					add_elem[obs] = true;
					nh[nh_cnt++] = obs;
				}
			}
		}
	}
	
	if (nh_cnt > 0) {
		hs.SetEventType(HighlightState::delta);
		hs.SetTotalNewlyHighlighted(nh_cnt);
		hs.SetTotalNewlyUnhighlighted(0);
		hs.notifyObservers();
	} else {
		LOG_MSG("No elements to add to current selection");
	}
	LOG_MSG("Exiting Project::AddNeighborsToSelection");
}


