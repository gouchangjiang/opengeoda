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

#include <cmath>
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../Project.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcBinDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcBinDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_BINARY_RESULT"),
			   FieldNewCalcBinDlg::OnBinaryResultUpdated )
	EVT_CHOICE( XRCID("IDC_BINARY_OPERATOR"),
			   FieldNewCalcBinDlg::OnBinaryOperatorUpdated )
    EVT_TEXT( XRCID("IDC_BINARY_OPERAND1"),
			 FieldNewCalcBinDlg::OnBinaryOperand1Updated )
	EVT_COMBOBOX( XRCID("IDC_BINARY_OPERAND1"),
				 FieldNewCalcBinDlg::OnBinaryOperand1Updated )
    EVT_TEXT( XRCID("IDC_BINARY_OPERAND2"),
			 FieldNewCalcBinDlg::OnBinaryOperand2Updated )
	EVT_COMBOBOX( XRCID("IDC_BINARY_OPERAND2"),
				 FieldNewCalcBinDlg::OnBinaryOperand2Updated )
END_EVENT_TABLE()

FieldNewCalcBinDlg::FieldNewCalcBinDlg(Project* project,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption, 
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), op_string(5), grid_base(project->GetGridBase()),
m_valid_const1(false), m_const1(1), m_var1_sel(wxNOT_FOUND),
m_valid_const2(false), m_const2(1), m_var2_sel(wxNOT_FOUND)
{
	SetParent(parent);
    CreateControls();
    Centre();
    
	op_string[add_op] = "ADD";
	op_string[subtract_op] = "SUBTRACT";
	op_string[multiply_op] = "MULTIPLY";
	op_string[divide_op] = "DIVIDE";
	op_string[power_op] = "POWER";
		
	for (int i=0, iend=op_string.size(); i<iend; i++) {
		m_op->Append(op_string[i]);
	}
	m_op->SetSelection(0);
	
	InitFieldChoices();
	all_init = true;
}

void FieldNewCalcBinDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_BI");
    m_result = XRCCTRL(*this, "IDC_BINARY_RESULT", wxChoice);
    m_var1 = XRCCTRL(*this, "IDC_BINARY_OPERAND1", wxComboBox);
    m_op = XRCCTRL(*this, "IDC_BINARY_OPERATOR", wxChoice);
    m_var2 = XRCCTRL(*this, "IDC_BINARY_OPERAND2", wxComboBox);
    m_text = XRCCTRL(*this, "IDC_EDIT5", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcBinDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please choose a result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	int result_col = col_id_map[m_result->GetSelection()];

	int var1_col = wxNOT_FOUND;
	int var2_col = wxNOT_FOUND;
	if (m_var1_sel != wxNOT_FOUND) var1_col = col_id_map[m_var1_sel];
	if (m_var2_sel != wxNOT_FOUND) var2_col = col_id_map[m_var2_sel];
	if ((var1_col == wxNOT_FOUND && !m_valid_const1)
		|| (var2_col == wxNOT_FOUND && !m_valid_const2)) {
		wxString msg("Operation requires a two valid field name or constants.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	int rows = grid_base->GetNumberRows();
	std::vector<double> data0(rows, 0); // results
	std::vector<bool> undefined0(rows, false); // results
	std::vector<double> data1(rows, 0); // var1
	std::vector<bool> undefined1(rows, false); // var1
	std::vector<double> data2(rows, 0); // var2
	std::vector<bool> undefined2(rows, false); // var2
	
	if (var1_col != wxNOT_FOUND) {
		grid_base->col_data[var1_col]->GetVec(data1);
		grid_base->col_data[var1_col]->GetUndefined(undefined1);
	} else {
		for (int i=0; i<rows; i++) data1[i] = m_const1;
	}
	if (var2_col != wxNOT_FOUND) {
		grid_base->col_data[var2_col]->GetVec(data2);
		grid_base->col_data[var2_col]->GetUndefined(undefined2);
	} else {
		for (int i=0; i<rows; i++) data2[i] = m_const2;
	}
	
	int op_sel = m_op->GetSelection();
	for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
		undefined0[i] = undefined1[i] || undefined2[i];
	}
	
	switch (m_op->GetSelection()) {
		case add_op:
		{
			for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
				if (!undefined0[i]) data0[i] = data1[i] + data2[i];
			}
		}
			break;
		case subtract_op:
		{
			for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
				if (!undefined0[i]) data0[i] = data1[i] - data2[i];
			}
		}
			break;
		case multiply_op:
		{
			for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {				if (!undefined0[i]) data0[i] = data1[i] + data2[i];
				if (!undefined0[i]) data0[i] = data1[i] * data2[i];
			}
		}
			break;
		case divide_op:
		{
			for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
				if (!undefined0[i] && data2[i] != 0) {
					data0[i] = data1[i] / data2[i];
				} else {
					data0[i] = 0;
					undefined0[i] = true;
				}
			}			
		}
			break;
		case power_op:
		{
			// If base is negative and exponent is not an integral value,
			// or if base is zero and exponent is negative, not defined.
			for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
				if (data1[i] == 0 && data2[i] < 0) undefined0[i] = true;
				if (!undefined0[i]) {
					if (data1[i] < 0) {
						// since base is negative, result will have an
						// imaginary part if exponent is not an integer
						int i_exp = (int) data2[i];
						double d_i_exp = (double) i_exp;
						if ((data2[i] - d_i_exp) != 0) {
							undefined0[i] = true;
						} else {
							data0[i] = pow(data1[i], i_exp);
						}
					} else {
						data0[i] = pow(data1[i], data2[i]);
					}
				}
			}			
		}
			break;
		default:
			return;
			break;
	}
	grid_base->col_data[result_col]->SetFromVec(data0);
	grid_base->col_data[result_col]->SetUndefined(undefined0);
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}


void FieldNewCalcBinDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	wxString v1_str_sel = m_var1->GetStringSelection();
	wxString v2_str_sel = m_var2->GetStringSelection();
	m_result->Clear();
	m_var1->Clear();
	m_var2->Clear();
	grid_base->FillNumericColIdMap(col_id_map);
	m_var_str.resize(col_id_map.size());
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = grid_base->col_data[col_id_map[i]]->name;
		m_result->Append(name);
		m_var1->Append(name);
		m_var2->Append(name);
		m_var_str[i] = name;
	}
	m_result->SetSelection(m_result->FindString(r_str_sel));
	m_var1->SetSelection(m_var1->FindString(v1_str_sel));
	m_var2->SetSelection(m_var2->FindString(v2_str_sel));
	m_var1_sel = m_var1->FindString(v1_str_sel);
	m_var2_sel = m_var2->FindString(v2_str_sel);
	
	Display();
}

void FieldNewCalcBinDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	l_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcBinDlg::Display()
{
	wxString s("");
	wxString lhs(m_result->GetStringSelection());
	wxString rhs("");	
	wxString var1("");
	if (m_var1_sel != wxNOT_FOUND) var1 = m_var_str[m_var1_sel];
	if (m_var1_sel == wxNOT_FOUND && m_valid_const1) {
		var1 = m_var1->GetValue();
		var1.Trim(false);
		var1.Trim(true);
	}
	wxString var2("");
	if (m_var2_sel != wxNOT_FOUND) var2 = m_var_str[m_var2_sel];
	if (m_var2_sel == wxNOT_FOUND && m_valid_const2) {
		var2 = m_var2->GetValue();
		var2.Trim(false);
		var2.Trim(true);
	}
	
	int op_sel = m_op->GetSelection();
	if (op_sel == add_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " + " << var2;
	} else if (op_sel == subtract_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " - " << var2;
	} else if (op_sel == multiply_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " * " << var2;
	} else if (op_sel == divide_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " / " << var2;
	} else { // op_sel == power_op
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " ^ " << var2;
	}
	
	if (lhs.IsEmpty() && rhs.IsEmpty()) {
		s = "";
	} else if (!lhs.IsEmpty() && rhs.IsEmpty()) {
		s << lhs << " =";
	} else if (lhs.IsEmpty() && !rhs.IsEmpty()) {
		s << rhs;
	} else {
		// a good time to enable the apply button.
		s << lhs << " = " << rhs;
	}
	
	m_text->SetValue(s);
}

void FieldNewCalcBinDlg::OnBinaryResultUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcBinDlg::OnBinaryOperatorUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand1Updated( wxCommandEvent& event )
{
	if (!all_init) return;
	
	wxString var_val = m_var1->GetValue();
	var_val.Trim(false);
	var_val.Trim(true);
	if (m_var1->GetValue() != m_var1->GetStringSelection()) {
		// User has typed something in manually.
		// if value matches some item on list, then set list to that
		// otherwise, set selection back to wxNOT_FOUND
		m_var1_sel = wxNOT_FOUND;
		for (int i=0, i_end=m_var_str.size(); m_var1_sel==-1 && i<i_end; i++) {
			if (var_val.IsSameAs(m_var_str[i], false)) m_var1_sel = i;
		}
		if (m_var1_sel != wxNOT_FOUND) {
			// don't use SetSelection because otherwise it will
			// be difficult to type in string names that have prefixes that
			// match someing in m_var_str
			//m_var1->SetSelection(m_var1_sel);
		} else {
			m_valid_const1 = var_val.ToDouble(&m_const1);
		}
	} else {
		m_var1_sel = m_var1->GetSelection();
	}
	
	Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand2Updated( wxCommandEvent& event )
{
	wxString var_val = m_var2->GetValue();
	var_val.Trim(false);
	var_val.Trim(true);
	if (m_var2->GetValue() != m_var2->GetStringSelection()) {
		// User has typed something in manually.
		// if value matches some item on list, then set list to that
		// otherwise, set selection back to wxNOT_FOUND
		m_var2_sel = wxNOT_FOUND;
		for (int i=0, i_end=m_var_str.size(); m_var2_sel==-1 && i<i_end; i++) {
			if (var_val.IsSameAs(m_var_str[i], false)) m_var2_sel = i;
		}
		if (m_var2_sel != wxNOT_FOUND) {
			// don't use SetSelection because otherwise it will
			// be difficult to type in string names that have prefixes that
			// match someing in m_var_str
			//m_var->SetSelection(m_var_sel);
		} else {
			m_valid_const2 = var_val.ToDouble(&m_const2);
		}
	} else {
		m_var2_sel = m_var2->GetSelection();
	}
	
	Display();
}

void FieldNewCalcBinDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	m_result->SetSelection(m_result->FindString(dlg.GetColName()));
	UpdateOtherPanels();
}
