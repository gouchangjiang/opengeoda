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

#include "GeoDaConst.h"
#include "GeneralWxUtils.h"

wxFont* GeoDaConst::small_font = 0;
wxFont* GeoDaConst::medium_font = 0;
wxFont* GeoDaConst::large_font = 0;

// The following are defined in shp2cnt and should be moved from there.
//background color -- this is light gray
const wxColour GeoDaConst::backColor(192, 192, 192);
// background color -- this is light gray
const wxColour GeoDaConst::darkColor(20, 20, 20);
// color of text, frames, points -- this is dark cherry
const wxColour GeoDaConst::textColor(128, 0, 64);
// outliers color (also used for regression, etc.) -- blue
const wxColour GeoDaConst::outliers_colour(0, 0, 255);
// envelope color (also used for regression, etc.) -- blue
const wxColour GeoDaConst::envelope_colour(0, 0, 255);

const wxColour GeoDaConst::selectable_outline_color(0, 0, 0); // black
const wxColour GeoDaConst::selectable_fill_color(49, 163, 84); // forest green
const wxColour GeoDaConst::highlight_color(255, 255, 0); // yellow
const wxColour GeoDaConst::canvas_background_color(255, 255, 255); // white
const wxColour GeoDaConst::legend_background_color(255, 255, 255); // white

// Map
const wxSize GeoDaConst::map_default_size(500, 300);
/// this is a light forest green
const wxColour GeoDaConst::map_default_fill_colour(49, 163, 84);
const wxColour GeoDaConst::map_default_outline_colour(0, 0, 0);
const wxColour GeoDaConst::map_default_highlight_colour(255, 255, 0); // yellow

// Map Movie
const wxColour GeoDaConst::map_movie_default_fill_colour(49, 163, 84);
const wxColour GeoDaConst::map_movie_default_highlight_colour(224, 113, 182);

// Histogram
const wxSize GeoDaConst::hist_default_size(500, 300);

// Table
const wxString GeoDaConst::table_frame_title("Table");
const wxSize GeoDaConst::table_default_size(750, 500);
const wxColour GeoDaConst::table_no_edit_color(80, 80, 80); // grey

// Scatterplot
const wxSize GeoDaConst::scatterplot_default_size(400, 400);
const wxColour GeoDaConst::scatterplot_scale_color(0, 0, 0);
const wxColour GeoDaConst::scatterplot_regression_color(0, 79, 241); 
const wxColour GeoDaConst::scatterplot_regression_selected_color(0, 146, 31); 
const wxColour GeoDaConst::scatterplot_regression_excluded_color(204, 41, 44);
const wxColour GeoDaConst::scatterplot_origin_axes_color(120, 120, 120);
wxPen* GeoDaConst::scatterplot_reg_pen = 0;
wxPen* GeoDaConst::scatterplot_reg_selected_pen = 0;
wxPen* GeoDaConst::scatterplot_reg_excluded_pen = 0;
wxPen* GeoDaConst::scatterplot_scale_pen = 0;
wxPen* GeoDaConst::scatterplot_origin_axes_pen = 0;

// Boxplot
const wxSize GeoDaConst::boxplot_default_size(300, 500);

// PCP (Parallel Coordinate Plot)
const wxSize GeoDaConst::pcp_default_size(500, 300);

// 3D Plot
const wxSize GeoDaConst::three_d_default_size(700, 500);

// Conditional View
const wxSize GeoDaConst::cond_view_default_size(500, 300);


/**
 Certain objects such as wxFont objects need to be created after
 wxWidgets is sufficiently initialized.  This function will be
 called just once by MyApp::OnInit() when the program begins.
 */
void GeoDaConst::init()
{
	// standard GeoDa font creation.  Through experimentation, as of the
	// wxWidgets 2.9.1 release, it appears that neither wxFont::SetPixelSize()
	// nor wxFont::SetPointSize() results in fonts of nearly similar vertical
	// height on all of our three supported platforms, Mac, Linux and Windows.
	// Therefore, at present we will specify sizes differently on each
	// platform according to experimentation.  When we specify the font
	// using point size, it seems that Linux and Windows are very similar, but
	// Mac is considerably smaller.
	const int ref_small_pt_sz = 8;
	const int ref_medium_pt_sz = 12;
	const int ref_large_pt_sz = 16;
	if (GeneralWxUtils::isMac()) {
		small_font = wxFont::New(ref_small_pt_sz+4, wxFONTFAMILY_SWISS,
								 wxFONTSTYLE_NORMAL, wxNORMAL, false,
								 wxEmptyString, wxFONTENCODING_DEFAULT);
		medium_font = wxFont::New(ref_medium_pt_sz+5, wxFONTFAMILY_SWISS,
								  wxFONTSTYLE_NORMAL, wxNORMAL, false,
								  wxEmptyString, wxFONTENCODING_DEFAULT);
		large_font = wxFont::New(ref_large_pt_sz+5, wxFONTFAMILY_SWISS,
								 wxFONTSTYLE_NORMAL, wxNORMAL, false,
								 wxEmptyString, wxFONTENCODING_DEFAULT);
	} else { // assumed to be Windows or GTK
		small_font = wxFont::New(ref_small_pt_sz, wxFONTFAMILY_SWISS,
								 wxFONTSTYLE_NORMAL, wxNORMAL, false,
								 wxEmptyString, wxFONTENCODING_DEFAULT);
		medium_font = wxFont::New(ref_medium_pt_sz, wxFONTFAMILY_SWISS,
								  wxFONTSTYLE_NORMAL, wxNORMAL, false,
								  wxEmptyString, wxFONTENCODING_DEFAULT);
		large_font = wxFont::New(ref_large_pt_sz, wxFONTFAMILY_SWISS,
								 wxFONTSTYLE_NORMAL, wxNORMAL, false,
								 wxEmptyString, wxFONTENCODING_DEFAULT);		
	}
	
	//ScatterPlot and ScatterNewPlot resources
	scatterplot_reg_pen = new wxPen(scatterplot_regression_color, 2);
	scatterplot_reg_selected_pen =
	new wxPen(scatterplot_regression_selected_color, 2);
	scatterplot_reg_excluded_pen =
	new wxPen(scatterplot_regression_excluded_color, 2);
	scatterplot_scale_pen =	new wxPen(scatterplot_scale_color);
	scatterplot_origin_axes_pen =
	new wxPen(scatterplot_origin_axes_color, 1, wxSHORT_DASH);
}
