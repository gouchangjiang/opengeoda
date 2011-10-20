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

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcmemory.h>
#include <wx/dataobj.h>
#include <wx/dcsvg.h>
#include <wx/dcps.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/xrc/xmlres.h>
#include "FramesManager.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"
#include "TemplateLegend.h"
#include "rc/oGeoDaIcon-16x16.xpm"
#include "GeneralWxUtils.h"
#include "OpenGeoDa.h"
#include "Project.h"
#include "logger.h"

IMPLEMENT_CLASS(TemplateFrame, wxFrame)

BEGIN_EVENT_TABLE(TemplateFrame, wxFrame)
END_EVENT_TABLE()

wxList TemplateFrame::my_children;
TemplateFrame* TemplateFrame::activeFrame = 0;
wxString TemplateFrame::activeFrName = wxEmptyString;

TemplateFrame::TemplateFrame(wxFrame *parent, Project* project_s,
							 const wxString& title,
							 const wxPoint& pos,
							 const wxSize& size, const long style)
: wxFrame(parent, -1, title, pos, size, style),
	template_canvas(0), template_legend(0), project(project_s),
	old_style(false), frames_manager(project_s->GetFramesManager())
{
	SetIcon(wxIcon(oGeoDaIcon_16x16_xpm));
	frames_manager->registerObserver(this);
}

TemplateFrame::~TemplateFrame()
{
	LOG_MSG("Called TemplateFrame::~TemplateFrame()");
	if (HasCapture()) ReleaseMouse();
	frames_manager->removeObserver(this);
}

void TemplateFrame::OnSelectWithRect(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithRect");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::rectangle);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithCircle");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::circle);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectWithLine(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithLine");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::line);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectionMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectionMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnFitToWindowMode");
	if (!template_canvas) return;
	template_canvas->SetFitToWindowMode(
				!template_canvas->GetFitToWindowMode());
	UpdateOptionMenuItems();
	LOG_MSG("Exiting TemplateFrame::OnFitToWindowMode");
}

void TemplateFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnFixedAspectRatioMode");
	if (!template_canvas) return;
	template_canvas->SetFixedAspectRatioMode(
				!template_canvas->GetFixedAspectRatioMode());	
	UpdateOptionMenuItems();
	LOG_MSG("Exiting TemplateFrame::OnFixedAspectRatioMode");
}

void TemplateFrame::OnZoomMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnZoomMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::zoom);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnPanMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnPanMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::pan);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnPrintCanvasState");
	if (template_canvas) {
		LOG_MSG(template_canvas->GetCanvasStateString());
	}
}

void TemplateFrame::UpdateOptionMenuItems()
{
	if (template_canvas == 0) return;
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::rectangle);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::circle);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::line);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECTION_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::select);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"),
								  template_canvas->GetFitToWindowMode());	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								  template_canvas->GetFixedAspectRatioMode());	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_ZOOM_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::zoom);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_PAN_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::pan);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
								  template_canvas->
									IsSelectableOutlineVisible());	
}

void TemplateFrame::UpdateContextMenuItems(wxMenu* menu)
{
	if (template_canvas == 0) return;
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_RECT"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::rectangle);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_CIRCLE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::circle);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_LINE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::line);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTION_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::select);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_FIT_TO_WINDOW_MODE"),
								  template_canvas->GetFitToWindowMode());	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								  template_canvas->GetFixedAspectRatioMode());	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_ZOOM_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::zoom);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_PAN_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::pan);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
								  template_canvas->
									IsSelectableOutlineVisible());	
}

void TemplateFrame::RegisterAsActive(const wxString& name,
									 const wxString& title)
{
	if (!MyFrame::theFrame) {
		// ~MyFrame() has been called, program is exiting
		return;
	}
	if (!activeFrame || activeFrame != this) {
		if (activeFrame && activeFrame != this)
			activeFrame->DeregisterAsActive();
		MapMenus();
		// Enable the Close menu item.  This saves including this code in every
		// single MapMenus call by all classes that inherit this class.
		GeneralWxUtils::EnableMenuItem(MyFrame::theFrame->GetMenuBar(),
									   "File",
									   wxID_CLOSE, true);
		activeFrame = this;
		activeFrName = name;
		MyFrame::theFrame->SetTitle(title);
	}
}

void TemplateFrame::DeregisterAsActive()
{
	LOG_MSG("In TemplateFrame::DeregisterAsActive");
	if (!MyFrame::theFrame) {
		// ~MyFrame() has been called, program is exiting
		return;
	}
	if (activeFrame == this) {
		activeFrame = NULL;
		activeFrName = wxEmptyString;
		LOG_MSG("reset toolbar to default.");
		// restore to a default state.
		MyFrame::theFrame->UpdateToolbarAndMenus();
	}
}

wxString TemplateFrame::GetActiveName()
{
	return activeFrName;
}

TemplateFrame* TemplateFrame::GetActiveFrame()
{
	return activeFrame;
}

void TemplateFrame::MapMenus()
{
	LOG_MSG("In TemplateFrame::MapMenus");
}


/** MMM: ExportImage assuemes the old style template canvas.  We should have
      a second version available.  OnDraw is used by the older
      TemplateCanvas chidren classes.  Perhaps we can just call
      PaintBackground(dc) followed by PaintShapes(dc). */
void TemplateFrame::ExportImage(TemplateCanvas* canvas, const wxString& type)
{
	LOG_MSG("Entering TemplateFrame::ExportImage");
	
    wxFileDialog dialog(canvas, "Save Image to File", wxEmptyString,
						project->GetMainName() + type + ".svg",
						//"BMP|*.bmp|PNG|*.png|PostScript|*.ps|SVG|*.svg",
						"BMP|*.bmp|PNG|*.png|SVG|*.svg",
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	dialog.SetFilterIndex(2);
	
    if (dialog.ShowModal() != wxID_OK) return;
	
	wxSize sz =  canvas->GetVirtualSize();
	
	wxFileName fname = wxFileName(dialog.GetPath());
	wxString str_fname = fname.GetPathWithSep() + fname.GetName();
	
	switch (dialog.GetFilterIndex()) {
		case 0:
		{
			LOG_MSG("BMP selected");
			wxBitmap bitmap( sz.x, sz.y );
			wxMemoryDC dc;
			dc.SelectObject( bitmap );
			dc.SetBrush(*wxWHITE_BRUSH);
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle(0,0,sz.x,sz.y);
			canvas->Draw(&dc);
			dc.SelectObject( wxNullBitmap );
			
			wxImage image = bitmap.ConvertToImage();
			
			if ( !image.SaveFile( str_fname + ".bmp", wxBITMAP_TYPE_BMP )) {
				wxMessageBox("Can't Save file");
			}			
			image.Destroy();
		}
			break;
			
		case 1:
		{
			LOG_MSG("PNG selected");
			wxBitmap bitmap( sz.x, sz.y );
			wxMemoryDC dc;
			dc.SelectObject( bitmap );
			wxColour brushColour(255, 255, 255, wxALPHA_TRANSPARENT); 
			wxBrush brush;
			dc.SetBrush(brushColour);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(0,0,sz.x,sz.y);
			canvas->Draw(&dc);
			dc.SelectObject( wxNullBitmap );
			
			wxImage image = bitmap.ConvertToImage();
			
			if ( !image.SaveFile( str_fname + ".png", wxBITMAP_TYPE_PNG )) {
				wxMessageBox("Can't Save file");
			}
			
			image.Destroy();
		}
			break;
		/* case 2:
		{
			LOG_MSG("PostScript selected");
			wxPrintData printData;
			printData.SetFilename(str_fname + ".ps");
			printData.SetPrintMode(wxPRINT_MODE_FILE);
			wxPostScriptDC dc(printData);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetPen(*wxBLACK_PEN);
			int w, h;
			dc.GetSize(&w, &h);
			LOG_MSG(wxString::Format("wxPostScriptDC GetSize = (%d,%d)", w, h));
			
			if (dc.IsOk()) {
				dc.StartDoc("printing...");
				int paperW, paperH;
				dc.GetSize(&paperW, &paperH);
				paperW += 14; // experimentally obtained tweak
				paperH += -52; // experimentally obtained tweak
				double marginFactor = 0.03;
				int margin = (int) (paperW*marginFactor);
				int workingW = paperW - 2*margin;
				int workingH = paperH - 2*margin;
				int originX = margin+1; // experimentally obtained tweak
				int originY = margin+50; // experimentally obtained tweak
				dc.SetDeviceOrigin(originX, originY); 
				LOG_MSG(wxString::Format(
					"PostScript DC origin set to (%d,%d)",
										 originX, originY));
				//dc.SetPen(*wxRED_PEN);
				//dc.SetBrush(*wxBLUE_BRUSH);
				//wxRect rect(wxPoint(0,0), wxPoint(workingW, workingH));
				//dc.DrawRectangle(rect);
				// Calculate the scaling factor to fit the picture to the page.
				int pictW = sz.GetWidth();
				int pictH = sz.GetHeight();
				double scale = wxMin((double) workingH/pictH,
									 (double) workingW/pictW);
				LOG_MSG(wxString::Format("PostScript DC scale factor = %f",
										 (float) scale));
				canvas->Draw(&dc);
				dc.EndDoc();
			} else {
				wxString msg("There was a problem generating the ");
				msg << "PostScript file.  Failed.";
				wxMessageBox(msg);
			}
		}
			break;
		 */
		case 2:
		{
			LOG_MSG("SVG selected");
			wxSVGFileDC dc(str_fname + ".svg", sz.x, sz.y);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.SetPen(*wxTRANSPARENT_PEN);
			canvas->Draw(&dc);
		}
			break;
			
		default:
		{
			LOG_MSG("Error: A non-recognized type selected.");
		}
			break;
	}
	return;
	
	LOG_MSG("Exiting TemplateFrame::ExportImage");
}

void TemplateFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
	ExportImage(template_canvas, activeFrName);
}

// MMM: This is for the old style TemplateCanvas children.  We should
// improve this function to use the PaintBackground function.
void TemplateFrame::OnOldStyleCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnOldStyleCopyImageToClipboard");
    wxSize sz = template_canvas->GetVirtualSize();
	
    wxBitmap bitmap(sz.x, sz.y);
	
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
	
    wxBrush brush;
    brush.SetColour(template_canvas->GetBackgroundColour());
    dc.SetBrush(brush);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
    template_canvas->Draw(&dc);
    dc.SelectObject(wxNullBitmap);
	
    if (!wxTheClipboard->Open()) {
        wxMessageBox("Can't open clipboard.");
    } else {
        wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
    }
	LOG_MSG("Exiting TemplateFrame::OnOldStyleCopyImageToClipboard");
}

void TemplateFrame::OnOldStyleCopyLegendToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnOldStyleCopyLegendToClipboard");
	if (!template_legend) return;
    wxSize sz = template_legend->GetVirtualSize();
	
    wxBitmap bitmap(sz.x, sz.y);
	
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
	
    wxBrush brush;
    brush.SetColour(template_legend->GetBackgroundColour());
    dc.SetBrush(brush);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
    template_legend->OnDraw(dc);
    dc.SelectObject(wxNullBitmap);
	
    if (!wxTheClipboard->Open()) {
        wxMessageBox("Can't open clipboard.");
    } else {
        wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
    }
	LOG_MSG("Exiting TemplateFrame::OnOldStyleCopyLegendToClipboard");
}


// MMM: This is for new style TemplateCanvas children.  We should
// improve this function to use the PaintBackground function.
void TemplateFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnCopyImageToClipboard");
	wxSize sz = template_canvas->GetVirtualSize();
		
	wxBitmap bitmap( sz.x, sz.y );
		
	wxMemoryDC dc;
	dc.SelectObject( bitmap );
		
	wxBrush brush;
	brush.SetColour(wxColour("WHITE"));
	dc.SetBrush(brush);
	dc.DrawRectangle(0,0,sz.x,sz.y);
	template_canvas->PaintShapes(dc);
	dc.SelectObject( wxNullBitmap );

	if ( !wxTheClipboard->Open() ) {
		wxMessageBox("Can't open clipboard.");
	} else {
		wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
		wxTheClipboard->Close();
	}
	LOG_MSG("Exiting TemplateFrame::OnCopyImageToClipboard");
}

void TemplateFrame::OnOldStyleCanvasBackgroundColor(wxCommandEvent& event)
{
	if (!template_canvas) return;
    wxColour col = template_canvas->GetBackgroundColour();
	
    wxColourData data;
    data.SetColour(col);
    data.SetChooseFull(true);
    for (int i = 0; i < 16; i++) {
        wxColour colour(i*16, i*16, i*16);
        data.SetCustomColour(i, colour);
    }
	
    wxColourDialog dialog(this, &data);
    dialog.SetTitle("Background Color");
    if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        col = retData.GetColour();
        template_canvas->SetBackgroundColour(col);
        template_canvas->Refresh();
    }
}

void TemplateFrame::OnOldStyleLegendBackgroundColor(wxCommandEvent& event)
{
	if (!template_legend) return;
    wxColour col = template_legend->GetBackgroundColour();
	
    wxColourData data;
    data.SetColour(col);
    data.SetChooseFull(true);
    for (int i = 0; i < 16; i++) {
        wxColour colour(i*16, i*16, i*16);
        data.SetCustomColour(i, colour);
    }
	
    wxColourDialog dialog(this, &data);
    dialog.SetTitle("Legend Background Color");
    if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        col = retData.GetColour();
        template_legend->SetBackgroundColour(col);
        template_legend->Refresh();
    }
}


bool TemplateFrame::GetColorFromUser(wxWindow* parent,
									 const wxColour& cur_color,
									 wxColour& ret_color,
									 const wxString& title)
{
	wxColourData data;
	data.SetColour(cur_color);
	data.SetChooseFull(true);
	for (int i=0; i<16; i++) {
		wxColour color(i*16, i*16, i*16);
		data.SetCustomColour(i, color);
	}
	
	wxColourDialog dialog(parent, &data);
	dialog.SetTitle(title);
	if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        ret_color = retData.GetColour();
		return true;
    }
	return false;
}

void TemplateFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnCanvasBackgroundColor");
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->canvas_background_color,
						  new_color,
						  "Background Color") ) {
		template_canvas->SetCanvasBackgroundColor(new_color);
	}
}

void TemplateFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineColor");
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->selectable_fill_color,
						  new_color,
						  "Fill Color") ) {
		template_canvas->SetSelectableFillColor(new_color);
	}	
}

void TemplateFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineColor");
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->selectable_outline_color,
						  new_color,
						  "Outline Color") ) {
		template_canvas->SetSelectableOutlineColor(new_color);
	}	
}

void TemplateFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineVisible");
	template_canvas->SetSelectableOutlineVisible(
						!template_canvas->selectable_outline_visible);
}

void TemplateFrame::OnHighlightColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnHighlightColor");
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->highlight_color,
						  new_color,
						  "Highlight Color") ) {
		template_canvas->SetHighlightColor(new_color);
	}
}

void TemplateFrame::update(FramesManager* o)
{
}


