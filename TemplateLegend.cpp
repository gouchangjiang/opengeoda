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
#include "TemplateFrame.h"
#include "TemplateLegend.h"

IMPLEMENT_ABSTRACT_CLASS(TemplateLegend, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateLegend, wxScrolledWindow)
END_EVENT_TABLE()

TemplateLegend::TemplateLegend(wxWindow *parent, const wxPoint& pos,
							   const wxSize& size)
: wxScrolledWindow(parent, wxID_ANY, pos, size,
				   wxBORDER_SUNKEN | wxVSCROLL | wxHSCROLL),
legend_background_color(GeoDaConst::legend_background_color),
template_frame(0)
{
	SetBackgroundColour(GeoDaConst::legend_background_color);
}

TemplateLegend::~TemplateLegend()
{
}
