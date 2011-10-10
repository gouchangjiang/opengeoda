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

#ifndef __GEODA_CENTER_TEMPLATE_CANVAS_H__
#define __GEODA_CENTER_TEMPLATE_CANVAS_H__

#include <wx/scrolwin.h>
#include <wx/event.h>
#include <wx/string.h>
#include <wx/dc.h>
#include <wx/overlay.h>
#include <vector>
#include <list>
#include <set> // for std::multiset template
#include "Generic/HighlightStateObserver.h"
#include "Generic/MyShape.h"
#include "Project.h"

enum SelectType {         
	// enumeration for the type of selection
    NO_SELECT,
    RECT_SELECT,
    BRUSH_SELECT,
	LINE_SELECT
};

class TemplateFrame;

/** TemplateCanvas is a base class that implements most of the
 functionality associated with selecting polygons.  It is the base
 class of all "views" in GeoDa such as Scatter Plots, Box Plots, etc.
 */

class TemplateCanvas : public wxScrolledWindow, public HighlightStateObserver
{
public:
	TemplateCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size,
				   bool fixed_aspect_ratio_mode = false,
				   bool fit_to_window_mode = true);
	virtual ~TemplateCanvas();
	
	/** to go away eventually */
	int Width;
	/** to go away eventually */
	int Height;
	/** to go away eventually */
	int Left;
	/** to go away eventually */
	int Right;
	/** to go away eventually */
	int Top;
	/** to go away eventually */
	int Bottom;

	/** to go away eventually */
	wxPoint gSelect1;
	/** to go away eventually */
	wxPoint gSelect2;
    /** to go away eventually. State variable to keep track of whether
     * selection tool outline is currently shown. */
    bool selection_outline_visible;
	/** to go away eventually. This keeps track of the last drawn
	rectangle outline.  It is used to erase the selection tool outline.*/
	wxPoint cur_sel_outline[4];
    /** to go away eventually. In conjunction with the use of
     the Overlay Drawing Canvas, this method draws the selection outline
     on the overlay dc according to the values of gSelect1 and gSelect2.  This
     method will eventually replace ClearFrame(). */
    void DrawSelectionOutline();
    /** to go away eventually. This method clears the entire overlay DC. */
    void EraseSelectionOutline();
	/** to go away eventually */
	// CLOCK clock_t ClickClock;
	/** to go away eventually */
	bool btnDown;
	/** to go away eventually */
	SelectType gRegime;
	/** to go away eventually */
	int num_obs;

	/** to go away eventually */
	void ClearFrame();
	/** to go away eventually */
	void EmptyClick();

	/** to go away eventually */
	void ReSizeSelection(wxMouseEvent& event, wxPoint& point);

	/** to go away eventually */
	void OnEvent(wxMouseEvent& event);
	/** to go away eventually */
	void OnLButtonDblClk(wxMouseEvent& event, wxPoint& point);
	/** to go away eventually */
	void OnLButtonDown(wxMouseEvent& event, wxPoint& point);
	/** to go away eventually */
	void OnLButtonUp(wxMouseEvent& event, wxPoint& point);
	/** to go away eventually */
	void OnMouseMove(wxMouseEvent& event, wxPoint& point); 
	/** to go away eventually */
	void OnMouseLeaving(wxMouseEvent& event, wxPoint& point);

	/** to go away eventually */
	virtual void SelectByPoint(wxMouseEvent& event) {}
	/** to go away eventually */
	virtual int SelectByRect(wxMouseEvent& event) {return 0;}
	/** to go away eventually */
	virtual void CheckSize() {}
	/** to go away eventually */
	virtual void Draw(wxDC* pDC) {}
	/** to go away eventually: Overridden from wxScrolledWindow::OnDraw */
	virtual void OnDraw(wxDC& dc) {}
	/** to go away eventually.  This has been added in to solve the
	 issue of the no-longer-working wxDC::SetLogicalFunction on the
	 Mac platform in the old-style canvas code.  This is not needed
	 for Linux and Windows and in fact seems to cause some issues. Therefore
	 the overlay code will mostly be protected with #ifndef __WXMAC__ macros.
	 */
	wxOverlay canvas_overlay; // OVERLAY CODE
	
	// BEGIN SHAPEMANAGER CODE
public:
	/** The mouse can be in one of three operational modes: select,
	 pan and zoom. */
	enum MouseMode { select, pan, zoom };
	
	/** When in mouse is in the 'select' operational mode, the SelectState
	 enum describes the types of states it can be in.  Initially it is in the
	 start state. */
	enum SelectState { start, leftdown, dragging, brushing };
	
	/** The selection/brushing tool can be either a rectangle, line or a
	 circle. */
	enum BrushType { rectangle, line, circle };

	/** The selection/brushing tool can be either a rectangle, line or a
	 circle. */
	enum ScrollBarMode { none, horiz_only, vert_only, horiz_and_vert };

public:
	/** Colors */
	bool selectable_outline_visible;
	wxColour selectable_outline_color;
	wxColour selectable_fill_color;
	wxColour highlight_color;
	wxColour canvas_background_color;
	
	virtual void SetSelectableOutlineVisible(bool visible);
	virtual bool IsSelectableOutlineVisible();
	virtual void SetSelectableOutlineColor(wxColour color);
	virtual void SetSelectableFillColor(wxColour color);
	virtual void SetHighlightColor(wxColour color);
	virtual void SetCanvasBackgroundColor(wxColour color);
	
protected:
	virtual void UpdateSelectableOutlineColors();
	
protected:
	MouseMode mousemode;
	SelectState selectstate;
	BrushType brushtype;
	ScrollBarMode scrollbarmode;
	
	/** The following parameters are used by the window resizing system.
	 We need to very carefully determine how these can be used together
	 in a flexible resizing system.
	 
	 Ideally the subclassed window will only need to draw objects to
	 an initial fixed working area, perhaps specified within initial
	 bbox dimensions.  These should be double precision floating points
	 to avoid resize errors.  From then on, all operations will be managed
	 by the TemplateCanvas class.
	 
	 We want to mimic the OS X Preview program zoom/pan behaviour as much
	 as possible.
	 */
	
	bool fixed_aspect_ratio_mode;
	bool fit_to_window_mode;
	int virtual_screen_marg_left;
	int virtual_screen_marg_right;
	int virtual_screen_marg_top;
	int virtual_screen_marg_bottom;
	double fixed_aspect_ratio_val;
	double current_shps_width;
	double current_shps_height;
	// the following four parameters should usually be obtained from
	// the shp file bounding box info in the header file.  They are used
	// to calculate the affine transformation when the window is resized.
	double shps_orig_xmin;
	double shps_orig_ymin;
	double shps_orig_xmax;
	double shps_orig_ymax;

public:
	/** This is the implementation of the Observer interface update function. 
	 It is called whenever the Observable's state has changed.  In this case,
	 the Observable is an instance of the HighlightState, which keeps track
	 of the hightlighted/selected state for every SHP file observation.
	 */
	virtual void update(HighlightState* o);
public:
	/** Returns a human-readable string of the values of many of the
	 internal state variables for the TemplateCanvas class instance.  This
	 is for debugging purposes. */
	virtual wxString GetCanvasStateString();

	void OnSize(wxSizeEvent& event);
	
	/** Where all the drawing action happens.  Should do something similar
	 to the update() method. */
	void OnPaint(wxPaintEvent& event);
	
	/** This function is needed to handle the Erase Background WX event.  It
	 does nothing, since we handle the Paint event ourselves and draw the
	 background and foreground ourselves. */
	void OnEraseBackground(wxEraseEvent& event);
	
	/** The function handles all mouse events. */
	void OnMouseEvent(wxMouseEvent& event);
	
	/** This function handles possible WX Mouse Capture Lost events. */
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
	
	/** This might go away since we have background_shps.  For now, this
	 should only be used to paint a solid-colour backgound.  In the future
	 this might display a bitmap image as well. */
	virtual void PaintBackground(wxDC& dc);
	
	/** Display all shapes in background_shps, selectable_shps and
	 foreground_shps on the drawing canvas. */
	virtual void PaintShapes(wxDC& dc);
	
	/** Draw the outline of the current selection tool. */
	virtual void PaintSelectionOutline(wxDC& dc);
	
	/** This might go away since we have foreground_shps. */
	virtual void PaintControls(wxDC& dc);
	
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	
	virtual void DrawMyShape(MyShape* shape, wxDC& dc);
	
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	
	virtual void UpdateSelectRegion(bool translate = false,
									wxPoint diff = wxPoint(0,0) );
	
	virtual void NotifyObservables();
	
	/** Resize the selectable_shps MyShape objects based on the
	 current screen size, the virtual screen size, fixed_aspect_ratio_mode,
	 fit_to_window_mode and virtual_screen_marg_left,
	 virtual_screen_marg_right, virtual_screen_marg_top,
	 and virtual_screen_marg_bottom.  When not in fit_to_window_mode, the
	 values of current_shps_width and current_shps_height are used to
	 resize the shps.  It is expected that these have been set to the
	 desired size before calling this method.  This will only be done by
	 the zoom method generally.
	 virtual_scrn_w and virtual_scrn_h are optional parameters.  When
	 they are > 0, they are used, otherwise we call GetVirtualSize
	 to get the current virtual screen size. */
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
	virtual void ApplyLastResizeToShp(MyShape* s) {
		s->applyScaleTrans(last_scale_trans); }
	
	virtual void SetBrushType(BrushType brush) { brushtype = brush; }
	virtual BrushType GetBrushType() { return brushtype; }
	
	virtual MouseMode GetMouseMode() { return mousemode; }
	/** Will set the mouse mode and change the mouse cursor */
	virtual void SetMouseMode(MouseMode mode);
	
	virtual bool GetFitToWindowMode();
	virtual void SetFitToWindowMode(bool mode);

	virtual bool GetFixedAspectRatioMode();
	virtual void SetFixedAspectRatioMode(bool mode);
		
	virtual void SetSelShpsMargs( int left, int right, int top, int bottom ) {
		virtual_screen_marg_left = left; virtual_screen_marg_right = right;
		virtual_screen_marg_top = top; virtual_screen_marg_bottom = bottom;
	}
	/** generic function to create and initialized the selectable_shps vector
		based on a passed-in Project pointer and given an initial canvas
	    screen size. */
	static void CreateSelShpsFromProj(std::vector<MyShape*>& selectable_shps,
									  Project* project);
	
protected:
	/** highlight_state is a pointer to the Observable HighlightState instance.
	 A HightlightState instance is a vector of booleans that keep track
	 of the highlight state for every observation in the currently opened SHP
	 file. This shared state object is the means by which the different
	 views in GeoDa are linked. */
	HighlightState* highlight_state;

	std::list<MyShape*> background_shps;
	/** This is an array of selectable objects.  In a map, these would
	 be the various observation regions or points, while in a histogram
	 these would be the bars of the histogram. This array of shapes is drawn
	 after the background_shps multi-set. */
	std::vector<MyShape*> selectable_shps;
	std::list<MyShape*> foreground_shps;
	
	wxPoint GetActualPos(const wxMouseEvent& event);
	wxPoint sel_poly_pts[100];  // for UpdateSelectRegion and UpdateSelection
	int n_sel_poly_pts;         // for UpdateSelectRegion and UpdateSelection
	wxRegion sel_region;	    // for UpdateSelectRegion and UpdateSelection
	bool remember_shiftdown;    // used by OnMouseEvent
	wxPoint diff;               // used by OnMouseEvent
	wxPoint prev;	            // used by OnMouseEvent
	wxPoint sel1;
	wxPoint sel2;
	wxPoint scroll_diff;
	MyScaleTrans last_scale_trans;
	
public:
	TemplateFrame* template_frame;
	
private:
	DECLARE_CLASS(TemplateCanvas)
	DECLARE_EVENT_TABLE()
	// END SHAPEMANAGER CODE
};


#endif

// BEGIN SHAPEMANAGER CODE

// END SHAPEMANAGER CODE
