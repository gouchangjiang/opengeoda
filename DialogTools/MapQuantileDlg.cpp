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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>
#include "MapQuantileDlg.h"

BEGIN_EVENT_TABLE( CMapQuantileDlg, wxDialog )
    EVT_BUTTON( wxID_OK, CMapQuantileDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, CMapQuantileDlg::OnCancelClick )
END_EVENT_TABLE()

CMapQuantileDlg::CMapQuantileDlg( int max_classes_s, bool dup_val_warning_s,
								 wxWindow* parent, wxWindowID id,
								 const wxString& caption, const wxPoint& pos,
								 const wxSize& size, long style )
: max_classes(max_classes_s), dup_val_warning(dup_val_warning_s)
{	
	SetParent(parent);
    CreateControls();
    Centre();
}

void CMapQuantileDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_DIALOG_QUANTILE");
	m_classes = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_QUANTILE")),
							  wxSpinCtrl);
	if (max_classes > 9) max_classes = 9;
	m_classes->SetRange(1, max_classes);
	wxString val;
	val << wxMin(4, max_classes);
	m_classes->SetValue(val);
	
}

void CMapQuantileDlg::OnOkClick( wxCommandEvent& event )
{
	int num = m_classes->GetValue();
	if( (num < 1) || (num > 9) ) {
		wxMessageBox("Please enter a number between 1 and 9");
		return;
	} else {
		event.Skip();
		EndDialog(wxID_OK);
	}
}

void CMapQuantileDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}

bool CMapQuantileDlg::ShowToolTips()
{
    return true;
}
