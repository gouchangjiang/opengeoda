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

#ifndef __GEODA_CENTER_DATA_VIEWER_RESIZE_COL_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_RESIZE_COL_DLG_H__

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>
#include "DbfGridTableBase.h"

class DataViewerResizeColDlg: public wxDialog
{
public:
    DataViewerResizeColDlg(wxGrid* grid,
						   DbfGridTableBase* grid_base,
						   wxWindow* parent );
    void CreateControls();
    void OnOkClick( wxCommandEvent& event );
	wxTextCtrl* m_col_id;
	int id;
	wxTextCtrl* m_col_width;
	int width;
	wxGrid* grid;
	DbfGridTableBase* grid_base;
	
	
private:
	DECLARE_EVENT_TABLE()
};

#endif
