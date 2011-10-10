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

#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/filedlg.h>
#include "../GeoDaConst.h"
#include "DataViewerApp.h"

extern void DataViewerInitXmlResource();

IMPLEMENT_APP( DataViewerApp )

bool DataViewerApp::OnInit()
{
	GeoDaConst::init();
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
	wxXmlResource::Get()->InitAllHandlers();
	DataViewerInitXmlResource();
	wxFileDialog dlg(0, "Choose DBF file", "", "",
					 "DBF files (*.dbf)|*.dbf");
	if (dlg.ShowModal() == wxID_OK) {
		DataViewerFrame *frame = new DataViewerFrame(0, dlg.GetPath());
		frame->LoadDefaultMenus();
		frame->Show(true);
		return true;
	} else {
		return false;
	}
}

