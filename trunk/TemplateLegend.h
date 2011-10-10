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

#ifndef __GEODA_CENTER_TEMPLATE_LEGEND_H__
#define __GEODA_CENTER_TEMPLATE_LEGEND_H__

#include <wx/scrolwin.h>
#include <wx/dc.h>

class TemplateFrame;

class TemplateLegend: public wxScrolledWindow
{
public:
	TemplateLegend(wxWindow *parent, const wxPoint& pos, const wxSize& size);
	virtual ~TemplateLegend();
	
	virtual void OnDraw(wxDC& dc) = 0;
	wxColour legend_background_color;

	TemplateFrame* template_frame;
	
	DECLARE_ABSTRACT_CLASS(TemplateLegend)
	DECLARE_EVENT_TABLE()
};

#endif
