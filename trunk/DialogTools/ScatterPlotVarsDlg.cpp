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
#include <wx/wxprec.h>
#include <wx/valtext.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces
#include <wx/grid.h>
#include <wx/generic/gridctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "ScatterPlotVarsDlg.h"

BEGIN_EVENT_TABLE( ScatterPlotVarsDlg, wxDialog )

	EVT_LISTBOX(XRCID("ID_LIST_VAR_X"), ScatterPlotVarsDlg::OnLBsValsChanged )
	EVT_LISTBOX(XRCID("ID_LIST_VAR_Y"), ScatterPlotVarsDlg::OnLBsValsChanged )
	EVT_LISTBOX(XRCID("ID_LIST_VAR_Z"), ScatterPlotVarsDlg::OnLBsValsChanged )
	EVT_CHECKBOX( XRCID("ID_SET_VARS_AS_DEFAULT"),
				 ScatterPlotVarsDlg::OnSetVarsAsDefaultClick )
    EVT_CHECKBOX( XRCID("ID_INCLUDE_BUBBLE_SIZE"),
				 ScatterPlotVarsDlg::OnIncludeBubbleSizeClick )
    EVT_BUTTON( XRCID("wxID_OK"), ScatterPlotVarsDlg::OnOkClick )
    EVT_BUTTON( XRCID("wxID_CANCEL"), ScatterPlotVarsDlg::OnCancelClick )

END_EVENT_TABLE()

ScatterPlotVarsDlg::ScatterPlotVarsDlg(wxString& g_def_var_X,
										 wxString& g_def_var_Y,
										 wxString& g_def_var_Z,
										 DbfGridTableBase* grid_base,
										 wxWindow* parent,
										 wxWindowID id,
										 const wxString& title, 
										 const wxPoint& pos,
										 const wxSize& size,
										 long style,
										 const wxString& name)
:	global_default_X(g_def_var_X), global_default_Y(g_def_var_Y),
	global_default_Z(g_def_var_Z),
	p_LB_X(0), p_LB_Y(0), p_LB_Z(0), p_set_vars_as_default(0),
	p_include_bubble_size(0), is_set_vars_as_new_defaults(false),
	var_X(wxEmptyString), var_Y(wxEmptyString), var_Z(wxEmptyString),
	is_include_bubble_size(false)
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_SCATTER_PLOT_VARS");
    p_LB_X = XRCCTRL(*this, "ID_LIST_VAR_X", wxListBox);
    p_LB_Y = XRCCTRL(*this, "ID_LIST_VAR_Y", wxListBox);
	p_LB_Z = XRCCTRL(*this, "ID_LIST_VAR_Z", wxListBox);
    p_set_vars_as_default = XRCCTRL(*this,
									"ID_SET_VARS_AS_DEFAULT", wxCheckBox);
	is_set_vars_as_new_defaults = p_set_vars_as_default->GetValue();
    p_include_bubble_size = XRCCTRL(*this,
									"ID_INCLUDE_BUBBLE_SIZE", wxCheckBox);
	is_include_bubble_size = p_include_bubble_size->GetValue();

	std::vector<int> col_id_map;
	grid_base->FillNumericColIdMap(col_id_map);
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString label = grid_base->col_data[col_id_map[i]]->name;
		p_LB_X->Append(label);
		p_LB_Y->Append(label);
		p_LB_Z->Append(label);
	}
	
	int sel = wxNOT_FOUND;
	sel = p_LB_X->FindString(global_default_X);
	if (sel != wxNOT_FOUND) {
		p_LB_X->SetSelection(sel);
		var_X = global_default_X;
	}
	sel = p_LB_Y->FindString(global_default_Y);
	if (sel != wxNOT_FOUND) {
		p_LB_Y->SetSelection(sel);
		var_Y = global_default_Y;
	}
	sel = p_LB_Z->FindString(global_default_Z);
	if (sel != wxNOT_FOUND) {
		p_LB_Z->SetSelection(sel);
		var_Z = global_default_Z;
	}
	
	wxCommandEvent dummy_event;
	OnIncludeBubbleSizeClick( dummy_event );
}

void ScatterPlotVarsDlg::OnLBsValsChanged( wxCommandEvent& event )
{
	if (p_LB_X->GetSelection() == wxNOT_FOUND) {
		var_X = wxEmptyString;
	} else {
		var_X = p_LB_X->GetString(p_LB_X->GetSelection());
	}
	if (p_LB_Y->GetSelection() == wxNOT_FOUND) {
		var_Y = wxEmptyString;
	} else {
		var_Y = p_LB_Y->GetString(p_LB_Y->GetSelection());
	}
	if (p_LB_Z->GetSelection() == wxNOT_FOUND) {
		var_Z = wxEmptyString;
	} else {
		var_Z = p_LB_Z->GetString(p_LB_Z->GetSelection());
	}
	UpdateButtons();
}

void ScatterPlotVarsDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip(); 
	EndDialog(wxID_CANCEL);// wxDialog::OnCancel(event);
}

void ScatterPlotVarsDlg::OnOkClick( wxCommandEvent& event )
{
	if (is_set_vars_as_new_defaults) {
		if (!var_X.IsEmpty()) global_default_X = var_X;
		if (!var_Y.IsEmpty()) global_default_Y = var_Y;
		if (!var_Z.IsEmpty() && is_include_bubble_size) {
			global_default_Z = var_Z;
		}
	}
	event.Skip();
	EndDialog(wxID_OK); // wxDialog::OnOK(event);
}

void ScatterPlotVarsDlg::OnSetVarsAsDefaultClick( wxCommandEvent& event )
{
	is_set_vars_as_new_defaults = p_set_vars_as_default->GetValue();
}

void ScatterPlotVarsDlg::OnIncludeBubbleSizeClick( wxCommandEvent& event )
{
	is_include_bubble_size = p_include_bubble_size->GetValue();
	p_LB_Z->Enable(is_include_bubble_size);
	FindWindow(XRCID("ID_STATICTEXT_Z"))->Enable(is_include_bubble_size);

	UpdateButtons();
}

void ScatterPlotVarsDlg::UpdateButtons()
{
	bool disable_ok = (var_X.IsEmpty() || var_Y.IsEmpty() ||
					   (is_include_bubble_size && var_Z.IsEmpty()));
	FindWindow(XRCID("wxID_OK"))->Enable(!disable_ok);
}
