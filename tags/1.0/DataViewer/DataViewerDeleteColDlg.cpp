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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include "DataViewerDeleteColDlg.h"

BEGIN_EVENT_TABLE( DataViewerDeleteColDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_DELETE_BUTTON"), DataViewerDeleteColDlg::OnDelete )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE"), DataViewerDeleteColDlg::OnChoice )
END_EVENT_TABLE()

DataViewerDeleteColDlg::DataViewerDeleteColDlg( )
{
}

DataViewerDeleteColDlg::DataViewerDeleteColDlg( DbfGridTableBase* grid_base_s,
											   wxWindow* parent)
: grid_base(grid_base_s)
{
	SetParent(parent);
    CreateControls();
    Centre();
}

void DataViewerDeleteColDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_DATA_VIEWER_DELETE_COL_DLG");
	m_del_button = wxDynamicCast(FindWindow(XRCID("ID_DELETE_BUTTON")),
								 wxButton);
	m_del_button->Enable(false);
	m_field = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE")), wxChoice);
	InitFieldChoices();
	m_message = wxDynamicCast(FindWindow(XRCID("ID_TEXT_MSG")),
							  wxStaticText);
	m_message->SetLabelText("");	
}

void DataViewerDeleteColDlg::OnDelete( wxCommandEvent& ev )
{
	if (m_field->GetSelection() == wxNOT_FOUND) {
		m_del_button->Enable(false);
		return;
	}
	int col_del_pos;
	col_del_pos = col_id_map[m_field->GetSelection()];
	wxString del_name = grid_base->col_data[col_del_pos]->name;
	
	grid_base->DeleteCol(col_del_pos);
	InitFieldChoices();
	m_del_button->Enable(false);
	m_message->SetLabelText("Deleted " + del_name.Upper());
}

void DataViewerDeleteColDlg::OnChoice( wxCommandEvent& ev )
{
	m_message->SetLabelText("");
	if (m_field->GetSelection() != wxNOT_FOUND) {
		m_del_button->Enable(true);
	}
}

void DataViewerDeleteColDlg::InitFieldChoices()
{
	col_id_map.clear();
	m_field->Clear();
	grid_base->FillColIdMap(col_id_map);
	for (int i=0, iend=grid_base->GetNumberCols(); i<iend; i++) {
		m_field->Append(grid_base->col_data[col_id_map[i]]->name.Upper());
	}	
}
