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

#include <vector>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "3DDlg.h"

BEGIN_EVENT_TABLE( C3DDlg, wxDialog )
    EVT_BUTTON( wxID_OK, C3DDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, C3DDlg::OnCancelClick )
END_EVENT_TABLE()

C3DDlg::C3DDlg(DbfGridTableBase* grid_base, wxWindow* parent, wxWindowID id,
			   const wxString& caption,
			   const wxPoint& pos, const wxSize& size, long style )
{
	Create(parent, id, caption, pos, size, style);

	pLBx->Clear();
	pLBy->Clear();
	pLBz->Clear();

	grid_base->FillNumericColIdMap(col_id_map);
	
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = grid_base->col_data[col_id_map[i]]->name;
		pLBx->Append(name);
		pLBy->Append(name);
		pLBz->Append(name);
	}
}

bool C3DDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
					const wxPoint& pos, const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    return true;
}

void C3DDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_3DPLOT");
    if (FindWindow(XRCID("IDC_LIST_VARIN_X")))
        pLBx = wxDynamicCast(FindWindow(XRCID("IDC_LIST_VARIN_X")), wxListBox);
    if (FindWindow(XRCID("IDC_LIST_VARIN_Y")))
        pLBy = wxDynamicCast(FindWindow(XRCID("IDC_LIST_VARIN_Y")), wxListBox);
    if (FindWindow(XRCID("IDC_LIST_VARIN_Z")))
        pLBz = wxDynamicCast(FindWindow(XRCID("IDC_LIST_VARIN_Z")), wxListBox);
}

void C3DDlg::OnOkClick( wxCommandEvent& event )
{
	if (pLBx->GetSelection() == wxNOT_FOUND ||
		pLBy->GetSelection() == wxNOT_FOUND ||
		pLBz->GetSelection() == wxNOT_FOUND) {
		return;
	}
	
	x_col_id = col_id_map[pLBx->GetSelection()];
	y_col_id = col_id_map[pLBy->GetSelection()];
	z_col_id = col_id_map[pLBz->GetSelection()];

	event.Skip();
	EndDialog(wxID_OK);
}

void C3DDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
}

