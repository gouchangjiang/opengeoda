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

#include "Generic/macro_cleaner.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces
#include <wx/clipbrd.h>
#include <wx/splitter.h>
#include <wx/overlay.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/dcbuffer.h>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/bimap.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include "Generic/MyShape.h"
#include "ShapeOperations/shp.h"
#include "ShapeOperations/shp2cnt.h"
#include "ShapeOperations/ShpFile.h"
#include "OpenGeoDa.h"
#include "GeoDaConst.h"
#include "GenUtils.h"
#include "logger.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"

extern Selection gSelection;
extern GeoDaEventType gEvent;

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(boost::geometry::cs::cartesian)

IMPLEMENT_CLASS(TemplateCanvas, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateCanvas, wxScrolledWindow)
	EVT_SIZE(TemplateCanvas::OnSize)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

// BEGIN SHAPEMANAGER CODE
TemplateCanvas::TemplateCanvas(wxWindow *parent, const wxPoint& pos,
							   const wxSize& size,
							   bool fixed_aspect_ratio_mode_s,
							   bool fit_to_window_mode_s)
: wxScrolledWindow(parent, -1, pos, size,
		wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE),
	gRegime(RECT_SELECT),
	selection_outline_visible(false), // this variable is for old-style views
	mousemode(select), selectstate(start), brushtype(rectangle),
	scrollbarmode(none),
	fixed_aspect_ratio_mode(fixed_aspect_ratio_mode_s),
	fit_to_window_mode(fit_to_window_mode_s),
	scroll_diff(0,0), remember_shiftdown(false),
	highlight_state(MyFrame::GetProject()->highlight_state),
	num_obs(MyFrame::GetProject()->GetNumRecords()),
	template_frame(0),
	fixed_aspect_ratio_val(1.0),
	current_shps_width(0.0), current_shps_height(0.0),
	virtual_screen_marg_left(GeoDaConst::default_virtual_screen_marg_left),
	virtual_screen_marg_right(GeoDaConst::default_virtual_screen_marg_right),
	virtual_screen_marg_top(GeoDaConst::default_virtual_screen_marg_top),
	virtual_screen_marg_bottom(GeoDaConst::default_virtual_screen_marg_bottom),
	shps_orig_xmin(0), shps_orig_ymin(0),
	shps_orig_xmax(0), shps_orig_ymax(0),
	last_scale_trans(),
	selectable_outline_visible(true),
	selectable_outline_color(GeoDaConst::selectable_outline_color),
	selectable_fill_color(GeoDaConst::selectable_fill_color),
	highlight_color(GeoDaConst::highlight_color),
	canvas_background_color(GeoDaConst::canvas_background_color)
{
	SetMouseMode(mousemode); // will set the correct cursor for current mode
	SetBackgroundStyle(wxBG_STYLE_ERASE);
	LOG_MSG("Entering TemplateCanvas::TemplateCanvas");
	LOG_MSG("Exiting TemplateCanvas::TemplateCanvas");
}

/**
 The destructor.  Note that all destructors in C++ should be declared
 "virtual".  Also note that super-class destructors are called automatically,
 and it is a mistake to call them explicitly (unlike for consturctors or
 other virtual methods).  All of the MyShape objects in
 #selectable_shps are deleted in this destructor.
 */
TemplateCanvas::~TemplateCanvas()
{
	LOG_MSG("Entering TemplateCanvas::~TemplateCanvas()");
	BOOST_FOREACH( MyShape* shp, background_shps ) delete shp;
	BOOST_FOREACH( MyShape* shp, selectable_shps ) delete shp;
	BOOST_FOREACH( MyShape* shp, foreground_shps ) delete shp;
	if (HasCapture()) ReleaseMouse();
	LOG_MSG("Exiting TemplateCanvas::~TemplateCanvas()");
}

bool TemplateCanvas::GetFixedAspectRatioMode()
{
	return fixed_aspect_ratio_mode;
}

void TemplateCanvas::SetFixedAspectRatioMode(bool mode)
{
	wxString msg("TemplateCanvas::SetFixedAspectRatioMode(");
	msg << mode << ")";
	LOG_MSG(msg);
	fixed_aspect_ratio_mode = mode;
	if (fixed_aspect_ratio_mode) {
		if (current_shps_width < current_shps_height) {
			current_shps_height = current_shps_width / fixed_aspect_ratio_val;
		} else {
			current_shps_width = current_shps_height * fixed_aspect_ratio_val;
		}
	}
	ResizeSelectableShps();
	Refresh();
}


bool TemplateCanvas::GetFitToWindowMode()
{
	return fit_to_window_mode;
}

void TemplateCanvas::SetFitToWindowMode(bool mode)
{
	wxString msg("TemplateCanvas::SetFitToWindowMode(");
	msg << mode << ")";
	LOG_MSG(msg);
	fit_to_window_mode = mode;
	scrollbarmode = none;
	if (fit_to_window_mode) {
		int cs_w=0, cs_h=0;
		GetClientSize(&cs_w, &cs_h);
		int vs_w, vs_h;
		GetVirtualSize(&vs_w, &vs_h);
		// SetVirtualSize will automatically generate an OnSize event when
		// the current virtual screen size does not equal the current
		// client screen size.  So, if an OnSize event will not be generated,
		// then we should manually call ResizeSelectableShps
		if (vs_w == cs_w && vs_h == cs_h ) {
			// this call will automatically generate an OnSize event when
			// the current virtual screen size does not equal the current
			// client screen size
			ResizeSelectableShps(cs_w, cs_h);
			Refresh();
		} else {
			SetVirtualSize(cs_w, cs_h);
		}
	}
}

wxString TemplateCanvas::GetCanvasStateString()
{
	wxString str("TemplateCanvas state data:\n");
	str << "  fit_to_window_mode = " << fit_to_window_mode;
	str << "\n  fixed_aspect_ratio_mode = " << fixed_aspect_ratio_mode;
	str << "\n  fixed_aspect_ratio_val = " << fixed_aspect_ratio_val;
	str << "\n  current_shps_width = " << current_shps_width;
	str << "\n  current_shps_height = " << current_shps_height;
	str << "\n  GetClientSize().GetWidth() = "
									<< GetClientSize().GetWidth();
	str << "\n  GetClientSize().GetHeight() = "
									<< GetClientSize().GetHeight();
	str << "\n  GetVirtualSize().GetWidth() = "
									<< GetVirtualSize().GetWidth();
	str << "\n  GetVirtualSize().GetHeight() = "
									<< GetVirtualSize().GetHeight();
	str << "\n";
	//str << "\n   = " << ;
	return str;
}

//bool fixed_aspect_ratio_mode;
//bool fit_to_window_mode;
//int virtual_screen_marg_left;  // virtual screen fixed margins
//int virtual_screen_marg_right;
//int virtual_screen_marg_top;
//int virtual_screen_marg_bottom;
//double current_shps_width;  // these will be set by the zoom in/out code
//double current_shps_height;
// the following four parameters should usually be obtained from
// the shp file bounding box info in the header file.  They are used
// to calculate the affine transformation when the window is resized.
//double shps_orig_xmin;
//double shps_orig_ymin;
//double shps_orig_xmax;
//double shps_orig_ymax;
//
// We assume that the above parameters are all set correctly.
//
// virtual_scrn_w and virtual_scrn_h are optional parameters.  When
// they are > 0, they are used, otherwise we call GetVirtualSize
// to get the current virtual screen size.
//
void TemplateCanvas::ResizeSelectableShps(int virtual_scrn_w,
										  int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
	LOG_MSG("Entering TemplateCanvas::ResizeSelectableShps");
	int vs_w=virtual_scrn_w, vs_h=virtual_scrn_h;
	if (vs_w <= 0 && vs_h <= 0) {
		GetVirtualSize(&vs_w, &vs_h);
	}
	double image_width, image_height;
	bool ftwm = GetFitToWindowMode();
	LOG(ftwm);
	MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
								   shps_orig_xmax, shps_orig_ymax,
								   virtual_screen_marg_top,
								   virtual_screen_marg_bottom,
								   virtual_screen_marg_left,
								   virtual_screen_marg_right,
								   vs_w, vs_h, fixed_aspect_ratio_mode,
								   ftwm,
								   &last_scale_trans.scale_x,
								   &last_scale_trans.scale_y,
								   &last_scale_trans.trans_x,
								   &last_scale_trans.trans_y,
								   ftwm ? 0 : current_shps_width,
								   ftwm ? 0 : current_shps_height,
								   &image_width, &image_height);
	LOG_MSG(last_scale_trans.GetString());
	wxString msg;
	msg << "    image_width = " << image_width;
	msg << ", image_height = " << image_height;
	msg << "\n    current_shps_width = " << current_shps_width;
	msg << ", current_shps_height = " << current_shps_height;
	LOG_MSG(msg);
	BOOST_FOREACH( MyShape* ms, background_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}	
	BOOST_FOREACH( MyShape* ms, selectable_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	BOOST_FOREACH( MyShape* ms, foreground_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}	
	LOG_MSG("Exiting TemplateCanvas::ResizeSelectableShps");
#ifdef __WXGTK__
	Refresh();  // Unfortunately, this is needed for Linux GTK at the moment
#endif
}

void TemplateCanvas::SetMouseMode(MouseMode mode)
{
	mousemode = mode;
	if (mousemode == select) {
		//SetCursor(*wxCROSS_CURSOR);
		SetCursor(*wxSTANDARD_CURSOR);
	} else if (mousemode == pan) {
		SetCursor(wxCursor(wxCURSOR_HAND));
	} else if (mousemode == zoom) {
		SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
	} else { // default 
		SetCursor(*wxSTANDARD_CURSOR);
	}

}

void TemplateCanvas::CreateSelShpsFromProj(
							std::vector<MyShape*>& selectable_shps,
							Project* project)
{
	using namespace Shapefile;
	
	if (selectable_shps.size() > 0) return;
	int num_recs = project->GetNumRecords();
	selectable_shps.resize(num_recs);
	std::vector<MainRecord>& records = project->main_data.records;
	Header& hdr = project->main_data.header;

	wxPen pen(GeoDaConst::selectable_outline_color, 1, wxSOLID);
	wxBrush brush(GeoDaConst::selectable_fill_color, wxSOLID);
	
	if (hdr.shape_type == Shapefile::POINT) {
		PointContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PointContents*) records[i].contents_p;
			selectable_shps[i] = new MyPoint(wxRealPoint(pc->x,pc->y));
			selectable_shps[i]->pen = pen;
			selectable_shps[i]->brush = brush;
			//selectable_shps[i] = new MyPoint(project->main_data.records
		}
	} else if (hdr.shape_type == Shapefile::POLYGON) {
		PolygonContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PolygonContents*) records[i].contents_p;
			selectable_shps[i] = new MyPolygon(pc);
			selectable_shps[i]->pen = pen;
			selectable_shps[i]->brush = brush;
		}
	} else if (hdr.shape_type == Shapefile::POLY_LINE) {
		PolyLineContents* pc = 0;
		wxPen pen(GeoDaConst::selectable_fill_color, 1, wxSOLID);		
		for (int i=0; i<num_recs; i++) {
			pc = (PolyLineContents*) records[i].contents_p;
			selectable_shps[i] = new MyPolyLine(pc);
			selectable_shps[i]->pen = pen;
			selectable_shps[i]->brush = brush;
		}
	}
	
	std::vector<bool>& hl = project->highlight_state->GetHighlight();
	for (int i=0; i<num_recs; i++) selectable_shps[i]->highlight = hl[i];
}

void TemplateCanvas::SetSelectableOutlineVisible(bool visible)
{
	wxString msg("Called TemplateCanvas::SetSelectableOutlineVisible(");
	if (visible) { msg << "true)"; } else { msg << "false)"; }
	LOG_MSG(msg);
	selectable_outline_visible = visible;
	UpdateSelectableOutlineColors();
	Refresh();
}

bool TemplateCanvas::IsSelectableOutlineVisible()
{
	return selectable_outline_visible;
}

void TemplateCanvas::SetSelectableOutlineColor(wxColour color)
{
	LOG_MSG("Called TemplateCanvas::SetSelectableOutlineColor");
	selectable_outline_color = color;
	UpdateSelectableOutlineColors();
	Refresh();
}

void TemplateCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	UpdateSelectableOutlineColors();
	Refresh();
}

void TemplateCanvas::SetHighlightColor(wxColour color)
{
	highlight_color = color;
	Refresh();
}

void TemplateCanvas::SetCanvasBackgroundColor(wxColour color)
{
	canvas_background_color = color;
	Refresh();
}

void TemplateCanvas::UpdateSelectableOutlineColors()
{
	LOG_MSG("Called TemplateCanvas::UpdateSelectableOutlineColors");
	wxPen pen(selectable_outline_visible ? selectable_outline_color :
			  selectable_fill_color, 1, wxSOLID);
	wxBrush brush(GeoDaConst::selectable_fill_color, wxSOLID);
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (MyPolyLine* p = dynamic_cast<MyPolyLine*>(selectable_shps[i])) {
			wxPen pen(GeoDaConst::selectable_fill_color, 1, wxSOLID);
			selectable_shps[i]->pen = pen;
		} else {  // same for all other MyShapes
			selectable_shps[i]->pen = pen;
		}
		selectable_shps[i]->brush = brush;
	}
}

void TemplateCanvas::OnSize(wxSizeEvent& event)
{
	LOG_MSG("Entering TemplateCanvas::OnSize");
	// we know there has been a change in the client size
	int cs_w=0, cs_h=0;
	GetClientSize(&cs_w, &cs_h);
	int vs_w, vs_h;
	GetVirtualSize(&vs_w, &vs_h);
	
	if (GetFitToWindowMode()) {
		double new_w = (cs_w-(virtual_screen_marg_left + 
						   virtual_screen_marg_right));
		double new_h = (cs_h-(virtual_screen_marg_top + 
						   virtual_screen_marg_bottom)); 
		double new_ar = (double) new_w / (double) new_h;
		LOG(new_w);
		LOG(new_h);
		LOG(new_ar);
		LOG(fixed_aspect_ratio_mode);
		LOG(fixed_aspect_ratio_val);
		if (fixed_aspect_ratio_mode) {
			if (fixed_aspect_ratio_val >= new_ar) {
				current_shps_width = new_w;
				current_shps_height = new_w / fixed_aspect_ratio_val;
			} else {
				current_shps_height = new_h;
				current_shps_width = new_h * fixed_aspect_ratio_val;
			}
		} else {
			current_shps_width = new_w;
			current_shps_height = new_h;
		}
		LOG(current_shps_width);
		LOG(current_shps_height);
		SetVirtualSize(cs_w, cs_h);
		ResizeSelectableShps();
	} else {
		int margs_vert = virtual_screen_marg_top + virtual_screen_marg_bottom;
		int margs_horiz = virtual_screen_marg_left + virtual_screen_marg_right;
		int shps_n_margs_w = current_shps_width + margs_horiz;
		int shps_n_margs_h = current_shps_height + margs_vert;
		
		if (shps_n_margs_w <= cs_w && shps_n_margs_h <= cs_h) {
			LOG_MSG("No Scroll Bars");
			ResizeSelectableShps(cs_w, cs_h);
			SetVirtualSize(cs_w, cs_h);
			scrollbarmode = none;
		}
		if (shps_n_margs_w <= cs_w && shps_n_margs_h > cs_h) {
			LOG_MSG("Vertical Scroll Bars Only");
			ResizeSelectableShps(cs_w, shps_n_margs_h);
			SetVirtualSize(cs_w, shps_n_margs_h);
#ifdef __WXMSW__
			Update();  // Only needed in Windows to get Vertical SB to
					   // draw automatically
#endif
			scrollbarmode = vert_only;
		}
		if (shps_n_margs_w > cs_w && shps_n_margs_h <= cs_h) {
			LOG_MSG("Horizontal Scroll Bars Only");
			ResizeSelectableShps(shps_n_margs_w, cs_h);
			SetVirtualSize(shps_n_margs_w, cs_h);
			scrollbarmode = horiz_only;
#ifdef __WXMSW__
			Update(); // Only needed in Windows to get Vertical SB to
			          // draw automatically
#endif
		}
		if (shps_n_margs_w > cs_w && shps_n_margs_h > cs_h) {
			LOG_MSG("Vertical and Horizontal Scroll Bars");
			SetVirtualSize(shps_n_margs_w, shps_n_margs_h);
			if (scrollbarmode != horiz_and_vert) {
				LOG_MSG("One-time shps resize");
				ResizeSelectableShps(shps_n_margs_w, shps_n_margs_h);
			}
			scrollbarmode = horiz_and_vert;
		}
	}

	event.Skip();
	LOG_MSG("Exiting TemplateCanvas::OnSize");
}

/**
 Impelmentation of HighlightStateObservable interface.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void TemplateCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering TemplateCanvas::update");
		
	wxClientDC dc(this);
	DoPrepareDC(dc);
	
	// Draw the updated objects.
	LOG_MSG("Painting Updated Shapes");
	
	int total = highlight_state->GetTotalNewlyUnhighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	for (int i=0; i<total; i++) {
		selectable_shps[nuh[i]]->highlight = false;
		DrawMyShape(selectable_shps[nuh[i]], dc);
	}
	
	total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	for (int i=0; i<total; i++) {
		selectable_shps[nh[i]]->highlight = true;
		DrawMyShape(selectable_shps[nh[i]], dc);
	}
	
	// Redraw the foreground
	BOOST_FOREACH(MyShape* shp, foreground_shps) {
		shp->paintSelf(dc);
	}
	
	// Draw the the selection region if needed
	PaintSelectionOutline(dc);
	
	// Draw and scroll/scale-invarant controls such as zoom/pan buttons
	PaintControls(dc);
	LOG_MSG("Exiting TemplateCanvas::update");
}

// Paint events are generated when user interaction
// causes regions to need repainting, or when wxWindow::Refresh
// wxWindow::RefreshRect is called.  wxWindow::Update can be
// called immediately after Refresh or RefreshRect to force the
// paint event to be called immediately.  Use the
// wxFULL_REPAINT_ON_RESIZE window style to have the entire
// window included in the update region.  This is important in the
// case where resizing the window changes the position of all of
// the window graphics.
void TemplateCanvas::OnPaint(wxPaintEvent& event)
{
	LOG_MSG("Entering TemplateCanvas::OnPaint");
	
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // We certainly need to call this for Mac otherwise
		// the image doesn't redraw when scrolling.
	//LOG_MSG("Painting");
	
	PaintBackground(dc);
	
	// Draw the selectable objects.
	PaintShapes(dc);
	
	// Draw the the selection region if needed
	PaintSelectionOutline(dc);
	
	// Draw and scroll/scale-invarant controls such as zoom/pan buttons
	PaintControls(dc);
	
	// redraw scrollbars?
	GetScrollPos(wxHSCROLL);
	
	LOG_MSG("Exiting TemplateCanvas::OnPaint");
}

// We will handle drawing our background in a paint event
// handler.  So, do nothing in this handler.
void TemplateCanvas::OnEraseBackground(wxEraseEvent& event)
{	
	// LOG_MSG("TemplateCanvas::OnEraseBackground called, do nothing.");
}

// wxMouseEvent notes:
// LeftDown(): true when the left button is first pressed down
// LeftIsDown(): true while the left button is down. During a mouse dragging
//  operation, this will continue to return true, while LeftDown() is false.
// RightDown/RightIsDown: similar to Left.
// Moving(): returns true when mouse is moved, but no buttons are pressed.
// Dragging(): returns true when mouse is moved and at least one mouse button is
//   pressed.
// CmdDown(): Checks if MetaDown under Mac and ControlDown on other platforms.
// ButtonDClick(int but = wxMOUSE_BTN_ANY): checks for double click of any
//   button. Can also specify wxMOUSE_BTN_LEFT / RIGHT / MIDDLE.  Or
//   LeftDCLick(), etc.
// LeftUp(): returns true at the moment the button changed to up.

void TemplateCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();
	
	// Scrollwheel data is highly platform specific at this point,
	// so it is unavoidable to have custom code for each different
	// platform.  As of wxWidgets 2.8.x there is no official support
	// for horizontal scrollwheel data, but it appears as though
	// Windows fakes this out by setting the wheel rotation value
	// +/-120 for vertical movement and +/-240 for horizontal movement.
	// We will have to check that this behaviour is consistent
	// across all versions of Windows.  Linux and Windows return +/-120
	// (and +/-240 in the case of Windows) while Mac returns any multiple
	// of 1.  This could possibly be to indicate amount of movement or
	// speed.  In wxWidgets 2.9.x, there is official support for horizontal
	// scrollwheel data on all platforms.
	
	// For non scroll events such as mouse movement, GetWheelDelta
	// sometimes returns 0 (Mac only).  To test if this is actually
	// a scroll event, we make sure event.GetLinesPerAction() returns at
	// least 1.  We might want to ignore scrolling when a mouse button is
	// being held down, or when in brushing mode.
	int wheel_rotation = event.GetWheelRotation();
	int wheel_delta = max(event.GetLinesPerAction(), 1);
	int wheel_lines_per_action = max(event.GetLinesPerAction(), 1);
	if (abs(wheel_rotation) >= wheel_delta) {
		LOG(wheel_rotation);
		LOG(wheel_delta);
		LOG(wheel_lines_per_action);
	}
	
	if (mousemode == select) {
		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}
		} else if (selectstate == leftdown) {
			if (event.Moving() || event.Dragging()) {
				wxPoint act_pos = GetActualPos(event);
				if (fabs((double) (prev.x - act_pos.x)) +
					fabs((double) (prev.y - act_pos.y)) > 2) {
					sel1 = prev;
					sel2 = GetActualPos(event);
					selectstate = dragging;
					remember_shiftdown = event.ShiftDown();
					UpdateSelectRegion();
					UpdateSelection(remember_shiftdown);
					Refresh();
				}
			} else if (event.LeftUp()) {
				UpdateSelectRegion();
				UpdateSelection(event.ShiftDown(), true);
				selectstate = start;
				Refresh();
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				Refresh();
			} else if (event.LeftUp() && !event.CmdDown()) {
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
				Refresh();
			} else if (event.LeftUp() && event.CmdDown()) {
				selectstate = brushing;
				sel2 = GetActualPos(event);
				wxPoint diff = wxPoint(0,0);
				UpdateSelectRegion(false, diff);
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				Refresh();
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		} else if (selectstate == brushing) {
			if (event.LeftIsDown()) {
			} else if (event.LeftUp()) {
				selectstate = start;
				Refresh();
			}
			else if (event.RightDown()) {
				selectstate = start;
				Refresh();
			} else if (event.Moving()) {
				wxPoint diff = GetActualPos(event) - sel2;
				sel1 += diff;
				sel2 = GetActualPos(event);
				UpdateSelectRegion(true, diff);
				UpdateSelection();
				Refresh();
			}
		} else { // unknown state
			LOG_MSG("TemplateCanvas::OnMouseEvent: ERROR, unknown SelectState");
		}
		
	} else if (mousemode == zoom) {
		// we will allow zooming in up to a maximum virtual screen area
		if (event.LeftUp()) {
			SetFitToWindowMode(false);
			int client_screen_w, client_screen_h;
			GetClientSize(&client_screen_w, &client_screen_h);
			int virtual_screen_w, virtual_screen_h;
			GetVirtualSize(&virtual_screen_w, &virtual_screen_h);
			wxSize v_size(GetVirtualSize()); 
			bool zoom_changed = false;
			if (!event.CmdDown()) {  // zoom in
				LOG_MSG("Entering TemplateCanvas::OnMouseEvent zoom in");				
				if ( (int) (current_shps_width * current_shps_height * 4) <=
						GeoDaConst::shps_max_area &&
					(int) (current_shps_width * 2) <= 
						GeoDaConst::shps_max_width &&
					(int) (current_shps_height *2) <=
						GeoDaConst::shps_max_height )
				{
					// If the resulting shps boundary will not fit
					// within the current virtual screen size, then we must
					// increase the virtual screen size to accommodate it.

					current_shps_width *= 2;
					current_shps_height *= 2;

					int new_w = (int) current_shps_width +
						virtual_screen_marg_left + virtual_screen_marg_right;
					int new_h = (int) current_shps_height +
						virtual_screen_marg_top + virtual_screen_marg_bottom;
					if ( new_h > client_screen_w || new_h > client_screen_h ) {
						// this instantly causes an OnSize event which will
						// itself call ResizeSelectableShapes
						SetVirtualSize(max(new_w, client_screen_w),
									   max(new_h, client_screen_h));
					}
					zoom_changed = true;
				}
			} else {                 // zoom out
				LOG_MSG("Entering TemplateCanvas::OnMouseEvent zoom out");
				// MMM we need to do some work here to determine exactly
				// what to do when zooming out.  It will often be the case
				// that scroll bars are no longer needed.  When this happens,
				// we must set the virtual size appropriately.  This isn't
				// happening right now.

				if ( (int) (current_shps_width / 2) >= 
						GeoDaConst::shps_min_width &&
					(int) (current_shps_height / 2) >=
						GeoDaConst::shps_min_height ) {

					current_shps_width /= 2;
					current_shps_height /= 2;
					
					int new_w = (int) current_shps_width +
						virtual_screen_marg_left + virtual_screen_marg_right;
					int new_h = (int) current_shps_height +
						virtual_screen_marg_top + virtual_screen_marg_bottom;
					int new_vs_w = max(new_w, client_screen_w);
					int new_vs_h = max(new_h, client_screen_h);
					LOG(new_vs_w);
					LOG(new_vs_h);
					// this instantly causes an OnSize event which will
					// itself call ResizeSelectableShapes
					SetVirtualSize(new_vs_w, new_vs_h);
					zoom_changed = true;
				}
			}
			if (zoom_changed) {
				LOG_MSG(GetCanvasStateString());
				ResizeSelectableShps();
				//LOG(GetVirtualSize().GetWidth());
				//LOG(GetVirtualSize().GetHeight());
				Refresh();
			}
			LOG_MSG("Exiting TemplateCanvas::OnMouseEvent zoom");
		} else if (event.RightDown()) {
			DisplayRightClickMenu(event.GetPosition());
		}
	} else if (mousemode == pan) {
		if (event.Moving()) {
			// in start state, do nothing
		} else if (event.LeftDown()) {
			prev = event.GetPosition();
			// temporarily set scroll rate to 1
			SetScrollRate(1,1);
		} else if (event.Dragging()
				   && !event.LeftUp() && !event.LeftDown()) {
			int xViewStart, yViewStart;
			GetViewStart(&xViewStart, &yViewStart);
			wxPoint diff = event.GetPosition() - prev;
			prev = event.GetPosition();
			Scroll(xViewStart-diff.x, yViewStart-diff.y);
		} else if (event.LeftUp()) {
			// restore original scroll rate
			SetScrollRate(1,1);
		} else if (event.RightDown()) {
			DisplayRightClickMenu(event.GetPosition());
		}		
	}
}

void TemplateCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void TemplateCanvas::PaintBackground(wxDC& dc)
{
	int xx, yy;
	CalcUnscrolledPosition(0,0,&xx,&yy);
	wxSize sz = GetClientSize();
	sz.SetWidth(sz.GetWidth()/1.0);  // zoom_factor = 1.0
	sz.SetHeight(sz.GetHeight()/1.0);
	xx = xx/1.0;
	yy = yy/1.0;
	wxRect windowRect(wxPoint(xx,yy), sz);
	dc.SetPen(wxPen(canvas_background_color));
	dc.SetBrush(wxBrush(canvas_background_color, wxSOLID));
	dc.DrawRectangle(wxPoint(0,0), GetVirtualSize());
		
	//dc.SetPen(*wxGREEN_PEN);
	//dc.SetBrush(*wxRED_BRUSH);
	//dc.DrawEllipse(wxPoint(180,30),wxSize(100,180));
	
	//int t=0;
	//wxPoint p[30];
	
	// skinny bowtie rectangle
	//p[t].x = -10; p[t++].y = 40;
	//p[t].x = 210; p[t++].y = 60;
	//p[t].x = 210; p[t++].y = 40;
	//p[t].x = -10; p[t++].y = 60;
	//p[t].x = -10; p[t++].y = 40;
	
	// square
	//p[t].x = 20; p[t++].y = 20;
	//p[t].x = 20; p[t++].y = 80;
	//p[t].x = 80; p[t++].y = 80;
	//p[t].x = 80; p[t++].y = 20;
	//p[t].x = 20; p[t++].y = 20;
	
	// clockwise rectangle
	//p[t].x = 0; p[t++].y= 0;
	//p[t].x = 200;  p[t++].y= 0;
	//p[t].x = 200; p[t++].y= 100;
	//p[t].x = 0; p[t++].y= 100;
	//p[t].x = 0; p[t++].y= 0;
	
	// counter-clockwise square
	//p[t].x = 10; p[t++].y = 10;
	//p[t].x = 10; p[t++].y = 90;
	//p[t].x = 90; p[t++].y = 90;
	//p[t].x = 90; p[t++].y = 10;
	//p[t].x = 10; p[t++].y = 10;

	// counter-clockwise square
	//p[t].x = 110; p[t++].y = 10;
	//p[t].x = 110; p[t++].y = 90;
	//p[t].x = 190; p[t++].y = 90;
	//p[t].x = 190; p[t++].y = 10;
	//p[t].x = 110; p[t++].y = 10;

	//p[t].x = 30; p[t++].y = 30;
	//p[t].x = 70; p[t++].y = 30;
	//p[t].x = 70; p[t++].y = 70;
	//p[t].x = 30; p[t++].y = 70;
	//p[t].x = 30; p[t++].y = 30;

	//int count[] = {5, 5, 5, 5, 5, 5};
	//dc.DrawPolyPolygon(6, count, p, 2, 160);
	
	//dc.DrawRectangle(20,20,260,160);
	
}

/** Paint background, selectable and foreground shps */
void TemplateCanvas::PaintShapes(wxDC& dc)
{
	BOOST_FOREACH( MyShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	//LOG(selectable_shps.size());
	std::vector<MyShape*>::iterator it;
	for ( it=selectable_shps.begin(); it != selectable_shps.end(); it++ )
		DrawMyShape(*it, dc);
	
	BOOST_FOREACH( MyShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}	
}

void TemplateCanvas::PaintSelectionOutline(wxDC& dc)
{
	if (mousemode == select &&
		(selectstate == dragging || selectstate == brushing) ) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*wxBLACK_PEN);
		if (brushtype == rectangle) {
			dc.DrawRectangle(wxRect(sel1, sel2));
		} else if (brushtype == line) {
			dc.DrawLine(sel1, sel2);
		} else if (brushtype == circle) {
			dc.DrawPolygon(n_sel_poly_pts, sel_poly_pts);
		}
	}
}

void TemplateCanvas::PaintControls(wxDC& dc)
{
	/*

	wxSize sz = GetClientSize();
	int w = 40;
	int h = 40;
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	int xx, yy;
	CalcUnscrolledPosition((sz.x-50),(sz.y-50),&xx,&yy);
	dc.DrawRectangle(xx,yy,w,h);
	dc.DrawCircle(xx,yy,w);
	
	wxPoint a[10];
	a[0] = wxPoint(0,0);
	a[1] = wxPoint(0,160);
	a[2] = wxPoint(160,160);
	a[3] = wxPoint(160,00);
	a[4] = wxPoint(0,0);
	a[5] = wxPoint(40,40);
	a[6] = wxPoint(120,40);
	a[7] = wxPoint(120,120);
	a[8] = wxPoint(40,120);
	a[9] = wxPoint(40,40);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxRED_BRUSH);
	dc.DrawPolygon(5, a, 200, 200);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawPolygon(5, (a+5), 200, 200);

	 */
}

wxPoint TemplateCanvas::GetActualPos(const wxMouseEvent& event) {
	int xx;
	int yy;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &xx, &yy);
	return wxPoint(xx/1.0,yy/1.0); // zoom_factor = 1.0
}

void TemplateCanvas::DrawMyShape(MyShape* shape, wxDC& dc)
{
	dc.SetPen(shape->pen);
	dc.SetBrush(shape->brush);
	wxBrush h_brush(highlight_color, wxCROSSDIAG_HATCH);
	wxPen h_pen(highlight_color);
	if (MyPoint* p = dynamic_cast<MyPoint*> (shape)) {
		dc.DrawCircle(p->point, GeoDaConst::my_point_click_radius);
		if (p->highlight) {
			dc.SetPen(h_pen);
			dc.DrawCircle(p->point, GeoDaConst::my_point_click_radius);
		}
	} else if (MyCircle* p = dynamic_cast<MyCircle*> (shape)) {
		dc.DrawCircle(p->center, p->radius);
		if (p->highlight) {
			dc.SetBrush(h_brush);
			dc.DrawCircle(p->center, p->radius);
		}
	} else if (MyPolygon* p = dynamic_cast<MyPolygon*> (shape)) {
		dc.DrawPolygon(p->n, p->points);
		if (p->highlight) {
			dc.SetBrush(h_brush);
			dc.DrawPolygon(p->n, p->points);
		}
	} else if (MyPolyLine* p = dynamic_cast<MyPolyLine*> (shape)) {
		int chunk_index = 0;  // will have the initial index of each part
		for (int h=0; h < p->n_count; h++) {
			if (p->count[h] > 1) {  // ensure this is a valid part
				dc.DrawLines(p->count[h],p->points+chunk_index);
				if (p->highlight) {
					dc.SetPen(h_pen);
					dc.DrawLines(p->count[h],p->points+chunk_index);
				}
			}
			chunk_index += p->count[h]; // increment to next part
		}
	}
}

void TemplateCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("TemplateCanvas::DisplayRightClickMenu() called");
}

void TemplateCanvas::UpdateSelectRegion(bool translate, wxPoint diff)
{
	wxPoint* p = sel_poly_pts;
	if (brushtype == rectangle) {
		sel_region = wxRegion(wxRect(sel1, sel2));
	} else if (brushtype == line) {
		sel_region = MyShape::createLineRegion(sel1, sel2);
	} else if (brushtype == circle) {
		if (!translate) {
			double radius = GenUtils::distance(sel1, sel2);			
			sel_region = MyShape::createCircleRegion(sel1, radius, 0,
													 sel_poly_pts,
													 &n_sel_poly_pts);
		} else { // we are just translating a previously drawn circle
			for (int i=0; i<n_sel_poly_pts; i++) {
				p[i] += diff;
			}
			sel_region = wxRegion(n_sel_poly_pts, sel_poly_pts);
		}
		
	}
}

void TemplateCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	LOG_MSG("Entering TemplateCanvas::UpdateSelection");
	int cnt = 0;  // number of deltas
	int total_sel_shps = selectable_shps.size();
	if (pointsel) { // a point selection
		for (int i=0; i<total_sel_shps; i++) {
			if (selectable_shps[i]->pointWithin(sel1)) {
				selectable_shps[i]->highlight = !selectable_shps[i]->highlight;
			} else {
				if (!shiftdown && selectable_shps[i]->highlight) {
					selectable_shps[i]->highlight = false;
				}
			}			
		}
	} else { // determine which centroids intersect the selection region.
		
		for (int i=0; i<total_sel_shps; i++) {
			if (!shiftdown) {
				if (!selectable_shps[i]->highlight &&
					selectable_shps[i]->regionIntersect(sel_region))
				{
					selectable_shps[i]->highlight = true;
				} else if (selectable_shps[i]->highlight &&
						   !selectable_shps[i]->regionIntersect(sel_region))
				{
					selectable_shps[i]->highlight = false;
				}
			} else { // do not unhighlight if not in intersection region
				if (!selectable_shps[i]->highlight &&
					selectable_shps[i]->regionIntersect(sel_region))
				{
					selectable_shps[i]->highlight = true;
				}
			}
		}		
	}
	NotifyObservables();
	LOG_MSG("Exiting TemplateCanvas::UpdateSelection");
}

/** In this default implementation of NotifyObservables, we assume
 that the selectable_shps are in one-to-one correspondence
 with the shps in the highlight_state observable vector.  If this is
 not true, then NotifyObservables() needs to be redefined in the
 child class.  This method compares the currently selected objects
 in this view with the currently selected objects in the shared
 HighlightState Observable instance and notifies the Observable instance
 of any changes.
 */
void TemplateCanvas::NotifyObservables()
{
	LOG_MSG("Entering TemplateCanvas::NotifyObservables");
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	for (int i=0; i<hl_size; i++) {
		//LOG(hs[i]);
		//LOG(selectable_shps[i]->highlight);
		if (hs[i] && !selectable_shps[i]->highlight) {
			hs[i] = false;
			nuh[total_newly_unselected++] = i;
		} else if (!hs[i] && selectable_shps[i]->highlight) {
			hs[i] = true;
			nh[total_newly_selected++] = i;
		}
	}
	LOG(total_newly_selected);
	LOG(total_newly_unselected);
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		highlight_state->notifyObservers();
	}
	LOG_MSG("Exiting TemplateCanvas::NotifyObservables");
}

// END SHAPEMANAGER CODE

void TemplateCanvas::DrawSelectionOutline()
{
	//LOG_MSG("In TemplateCanvas::DrawSelectionOutline");

	wxClientDC dc(this);
	PrepareDC(dc);
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
		// OVERLAY CODE
		wxDCOverlay overlaydc( canvas_overlay, &dc );
#endif

	if (selection_outline_visible) {
		//LOG_MSG("   selection_outline_visible==true, first erasing selection \
		//		outline");
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
		// OVERLAY CODE
		//LOG_MSG("Calling overlaydc.Clear()");
		overlaydc.Clear();
		//LOG_MSG("Called overlaydc.Clear()");
#else
		if (!selection_outline_visible) return;
		dc.SetLogicalFunction(wxINVERT);
		dc.SetPen(*wxBLACK_PEN);
		dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
		dc.DrawLine(cur_sel_outline[1], cur_sel_outline[2]);
		dc.DrawLine(cur_sel_outline[2], cur_sel_outline[3]);
		dc.DrawLine(cur_sel_outline[3], cur_sel_outline[0]);
#endif
		selection_outline_visible = false;
	}

	wxPoint p1, p2;
	if (gSelect1.x <= gSelect2.x) {
		p1.x= gSelect1.x;  p2.x= gSelect2.x;
	}  else  {
		p2.x= gSelect1.x;  p1.x= gSelect2.x;
	}
	if (gSelect1.y <= gSelect2.y)  {
		p1.y= gSelect1.y;  p2.y= gSelect2.y;
	}  else  {
		p2.y= gSelect1.y;  p1.y= gSelect2.y;
	}

	cur_sel_outline[0]   = p1;
	cur_sel_outline[1].x = p1.x;
	cur_sel_outline[1].y = p2.y;
	cur_sel_outline[2]   = p2;
	cur_sel_outline[3].x = p2.x;
	cur_sel_outline[3].y = p1.y;

#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	dc.SetPen(*wxBLUE_PEN);
#else
	dc.SetLogicalFunction(wxINVERT);
	dc.SetPen(*wxBLACK_PEN);
#endif
	dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
    dc.DrawLine(cur_sel_outline[1], cur_sel_outline[2]);
    dc.DrawLine(cur_sel_outline[2], cur_sel_outline[3]);
    dc.DrawLine(cur_sel_outline[3], cur_sel_outline[0]);
	selection_outline_visible = true;
}

void TemplateCanvas::EraseSelectionOutline()
{
	//LOG_MSG("In TemplateCanvas::EraseSelectionOutline");
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	wxClientDC dc(this);
	PrepareDC(dc);
    wxDCOverlay overlaydc( canvas_overlay, &dc );
    //LOG_MSG("Calling overlaydc.Clear()");
    overlaydc.Clear();
    //LOG_MSG("Called overlaydc.Clear()");
#else
	if (!selection_outline_visible) return;
	wxClientDC dc(this);
	PrepareDC(dc);
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(*wxBLACK_PEN);
	dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
    dc.DrawLine(cur_sel_outline[1], cur_sel_outline[2]);
    dc.DrawLine(cur_sel_outline[2], cur_sel_outline[3]);
    dc.DrawLine(cur_sel_outline[3], cur_sel_outline[0]);
#endif
	selection_outline_visible = false;
}

void TemplateCanvas::ClearFrame()  
{
	//LOG_MSG("In TemplateCanvas::ClearFrame");
	
	wxPoint p1, p2;
	if (gSelect1.x <= gSelect2.x) {
		p1.x= gSelect1.x;  p2.x= gSelect2.x;
	}  else  {
		p2.x= gSelect1.x;  p1.x= gSelect2.x;
	}
	if (gSelect1.y <= gSelect2.y)  {
		p1.y= gSelect1.y;  p2.y= gSelect2.y;
	}  else  {
		p2.y= gSelect1.y;  p1.y= gSelect2.y;
	}
	
	wxPoint pnt[4];
	pnt[0]   = p1;
	pnt[1].x = p1.x;
	pnt[1].y = p2.y;
	pnt[2]   = p2;
	pnt[3].x = p2.x;
	pnt[3].y = p1.y;
	
	wxClientDC dc(this);
	PrepareDC(dc);

	// OVERLAY CODE
    // __WXMAC__ __WXGTK__ __WXMSW__
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
    wxDCOverlay overlaydc( canvas_overlay, &dc );
    //LOG_MSG("Calling overlaydc.Clear()");
    overlaydc.Clear();
    //LOG_MSG("Called overlaydc.Clear()");
    dc.SetPen(*wxBLUE_PEN);
#else
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(*wxBLACK_PEN);
#endif
    dc.DrawLine(pnt[0], pnt[1]);
    dc.DrawLine(pnt[1], pnt[2]);
    dc.DrawLine(pnt[2], pnt[3]);
    dc.DrawLine(pnt[3], pnt[0]);
}

void TemplateCanvas::EmptyClick()
{
	gEvent= NEW_SELECTION;
}

void TemplateCanvas::OnEvent(wxMouseEvent& event)
{
	if (event.RightIsDown()) {
		// Right mouse drags are not supported, so just return
		// if this is true during the event.
		return;
	}
	
    wxClientDC dc(this);
    PrepareDC(dc);
    wxPoint point(event.GetLogicalPosition(dc));

	if(event.LeftDClick()) {
		OnLButtonDblClk(event, point);
		return;
	}
	if(event.LeftDown()) {
		OnLButtonDown(event, point);
		return;
	}
	if(event.LeftUp()) {
		OnLButtonUp(event, point);
		return;
	}
	if(event.Dragging()) {
		OnMouseMove(event, point);
		return;
	}
	if(event.Moving()) {
		if(gRegime == BRUSH_SELECT) {
			OnMouseMove(event, point);
		}
		return;
	}
}

void TemplateCanvas::OnLButtonDblClk(wxMouseEvent& event, wxPoint& point) 
{
	gSelection.Reset(true);
	gSelection.Invert();
	for (int cnt= 0; cnt<num_obs; ++cnt) {
		if (gSelection.selected(cnt)) gSelection.Push(cnt);
	}
	gEvent = NEW_SELECTION;
	gSelection.Update();
	MyFrame::theFrame->UpdateWholeView(NULL); 
	gSelection.Reset(true);
	// BTN_DOWN btnDown= false;
}

void TemplateCanvas::OnLButtonDown(wxMouseEvent& event, wxPoint& point) 
{
	//LOG_MSG("Entering TemplateCanvas::OnLButtonDown");
	if (gRegime != BRUSH_SELECT) { 
		gSelect1 = point;
		gSelect2 = point;
	}
	gRegime = RECT_SELECT;        // working on creating a rectangle
	LOG_MSG("TemplateCanvas::OnLButtonDown calling EraseSelectionOutline");
	EraseSelectionOutline();
	//LOG_MSG("Exiting TemplateCanvas::OnLButtonDown");
}

void TemplateCanvas::OnLButtonUp(wxMouseEvent& event, wxPoint& point) 
{
	LOG_MSG("Entering TemplateCanvas::OnLButtonUp");

	gSelect2 = point;
	// If the total movement of the mouse is less than three pixels
	// from the initial gSelect1 position recorded when the
	// left mouse button was first pressed, then we treat the
	// selection as a simple selection rather than a mouse drag.
	if (abs(gSelect1.x-gSelect2.x) + abs(gSelect1.y-gSelect2.y) <= 3) {
		EraseSelectionOutline();
		SelectByPoint(event);
		gSelection.Reset( true );
		return;
	}
  
	if (event.CmdDown()) gRegime= BRUSH_SELECT;

	if (gRegime != BRUSH_SELECT) {
		LOG_MSG("TemplateCanvas::OnLButtonUp, gRegime != BRUSH_SELECT, \
			calling EraseSelectionOutline");
		EraseSelectionOutline();
	}
	SelectByRect(event);
	gSelection.Reset( true );

	canvas_overlay.Reset(); // OVERLAY CODE

	LOG_MSG("Exiting TemplateCanvas::OnLButtonUp");
}

void TemplateCanvas::OnMouseLeaving(wxMouseEvent& event, wxPoint& point)
{
}


void TemplateCanvas::OnMouseMove(wxMouseEvent& event, wxPoint& point) 
{
	//LOG_MSG("Entering TemplateCanvas::OnMouseMove");
	gSelection.Reset(true);
	ReSizeSelection(event, point);
	//LOG_MSG("Exiting TemplateCanvas::OnMouseMove");
}

void TemplateCanvas::ReSizeSelection(wxMouseEvent& event, wxPoint& point)
{
	//LOG_MSG("Entering TemplateCanvas::ReSizeSelection");

	if (gRegime == NO_SELECT) {
		//LOG_MSG("ReSizeSelection, gRegime == NO_SELECT, exiting...");
		return;
	}

	// check if gSelect1 is valid.  If not, set it to current mouse position
	if (gSelect1.x == -1) gSelect1 = point;
	if (gRegime == RECT_SELECT) {			// resizing a rectangle
		gSelect2 = point;					// update the corner
		//LOG_MSG("gRegime == RECT_SELECT, so calling DrawSelectionOutline");
		DrawSelectionOutline();
	} else {								// gRegime == BRUSH_SELECT
		EraseSelectionOutline();
		gSelect1.x += (point.x - gSelect2.x);
		gSelect1.y += (point.y - gSelect2.y);
		gSelect2 = point;					// update the corner
		//wxString msg;
		//msg << "*\n*\ngSelect1 = (" << gSelect1.x << "," << gSelect1.y << ")  ";
		//msg << "gSelect2 = (" << gSelect2.x << "," << gSelect2.y << ")\n*\n*";
		//LOG_MSG(msg);

		// mCnt returns the number of selected objects.
		int mCnt = SelectByRect(event);
		//LOG(mCnt);

		if (gEvent == NO_EVENTS && !(event.ShiftDown())) {
			//LOG_MSG("gEvent == NO_EVENTS && !(event.ShiftDown())");
			gEvent = NEW_SELECTION;
		}

		//LOG_MSG("Calling DrawSelectionOutline near end of ReSizeSelection");
		DrawSelectionOutline();
	}
	//LOG_MSG("Exiting TemplateCanvas::ReSizeSelection");
}
