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

#ifndef __GEODA_TEMPLATE_FRAME_H__
#define __GEODA_TEMPLATE_FRAME_H__

#include <list>
#include <wx/frame.h>
#include "FramesManagerObserver.h"
class FramesManager;
class Project;
class TemplateCanvas;
class TemplateLegend;

/**
 * Common template frame
 */
class TemplateFrame: public wxFrame, public FramesManagerObserver
{
public:
	TemplateFrame(wxFrame *parent, Project* project, const wxString& title,
				  const wxPoint& pos, const wxSize& size, const long style);
	virtual ~TemplateFrame();

	virtual void MapMenus();
	void RegisterAsActive(const wxString& name,
						  const wxString& title = "OpenGeoDa");
	void DeregisterAsActive();
	static wxString GetActiveName();
	static TemplateFrame* GetActiveFrame();
	
protected:
	bool old_style;
public:
	/** Will be used mostly by TemplateFrame to determine which
	 TemplateCanvas children are using the old style drawing and
	 selection methods such as TemplateCanvas::OnDraw(), and the newer style
	 children which use TemplateCanvas::OnPaint() and
	 TemplateCanvas::PaintShapes().   No new classes
	 should use old style (search for "OldStyle" and "old style")
	 and ideally all children will eventually be modified to use
	 the new style TemplateCanvas paradigm. */
	bool IsOldStyle() { return old_style; }
	
	static bool GetColorFromUser(wxWindow* parent,
								 const wxColour& cur_color,
								 wxColour& ret_color,
								 const wxString& title = "Choose A Color");
	
	virtual void ExportImage(TemplateCanvas* canvas, const wxString& type);
	virtual void OnSaveCanvasImageAs(wxCommandEvent& event);
	virtual void OnOldStyleCopyImageToClipboard(wxCommandEvent& event);
	virtual void OnOldStyleCopyLegendToClipboard(wxCommandEvent& event);
	virtual void OnCopyImageToClipboard(wxCommandEvent& event);
	virtual void OnOldStyleCanvasBackgroundColor(wxCommandEvent& event);
	virtual void OnOldStyleLegendBackgroundColor(wxCommandEvent& event);
	virtual void OnCanvasBackgroundColor(wxCommandEvent& event);
	virtual void OnSelectableFillColor(wxCommandEvent& event);
	virtual void OnSelectableOutlineColor(wxCommandEvent& event);
	virtual void OnSelectableOutlineVisible(wxCommandEvent& event);
	virtual void OnHighlightColor(wxCommandEvent& event);
	virtual void OnSelectWithRect(wxCommandEvent& event);
	virtual void OnSelectWithCircle(wxCommandEvent& event);
	virtual void OnSelectWithLine(wxCommandEvent& event);
	virtual void OnSelectionMode(wxCommandEvent& event);
	virtual void OnFitToWindowMode(wxCommandEvent& event);
	virtual void OnFixedAspectRatioMode(wxCommandEvent& event);
	virtual void OnZoomMode(wxCommandEvent& event);
	virtual void OnPanMode(wxCommandEvent& event);
	virtual void OnPrintCanvasState(wxCommandEvent& event);
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	virtual Project* GetProject() { return project; }
	static wxList my_children;
private:
	static TemplateFrame* activeFrame;
	static wxString activeFrName;
protected:
	Project* project;
	TemplateCanvas* template_canvas;
	TemplateLegend* template_legend; // optional
	FramesManager* frames_manager;

	DECLARE_CLASS(TemplateFrame)
	DECLARE_EVENT_TABLE()
};

#endif
