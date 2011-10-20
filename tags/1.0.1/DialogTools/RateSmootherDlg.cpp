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

#include <wx/xrc/xmlres.h> // XRC XML resouces
#include <wx/msgdlg.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "RateSmootherDlg.h"

BEGIN_EVENT_TABLE( RateSmootherDlg, wxDialog )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VARIABLE1"),
					   RateSmootherDlg::OnCListVariable1DoubleClicked )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VARIABLE2"),
					   RateSmootherDlg::OnCListVariable2DoubleClicked )
    EVT_CHECKBOX( XRCID("IDC_CHECK1"),
				 RateSmootherDlg::OnCCheck1Click )
    EVT_BUTTON( wxID_CANCEL, RateSmootherDlg::OnCancelClick )
    EVT_BUTTON( wxID_OK, RateSmootherDlg::OnOkClick )
END_EVENT_TABLE()

RateSmootherDlg::RateSmootherDlg(DbfGridTableBase* grid_base_s,
								 wxString v1, wxString v2,
								 double* d1,
								 bool m_VarDef, short smoother,
								 GalElement* gal,
								 wxWindow* pParent)
: grid_base(grid_base_s), obs(grid_base_s->GetNumberRows())
{
	m_CheckDefault = m_VarDef;
	
	m_Var1 = v1;
	m_Var2 = v2;
	m_gal = gal;
	
	Create(pParent);
	
	E = new double[obs];
	P = new double[obs];
	
	m_smoother = smoother; // 9: is for MoranI EB Rate Standardization
	
	m_results = d1;
	PumpingVariables();
	
	pCheck->SetValue(m_CheckDefault);
	
	pCB->Append("Percentile Map");
	pCB->Append("Quantile Map");
	pCB->Append("Box Map (Hinges = 1.5)");
	pCB->Append("Box Map (Hinges = 3.0)");
	pCB->Append("Standard Deviation Map");
	pCB->SetSelection(0);
	
	if (m_smoother == 9 || m_smoother == 5) pCB->Enable(false);
}

RateSmootherDlg::~RateSmootherDlg()
{
	if (E) delete [] E; E = 0;
	if (P) delete [] P; P = 0;
}

bool RateSmootherDlg::Create( wxWindow* parent, wxWindowID id,
							  const wxString& caption, const wxPoint& pos,
							  const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}

void RateSmootherDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_RATE_SMOOTHER");
    if (FindWindow(XRCID("IDC_LIST_VARIABLE1")))
        pLB1 = wxDynamicCast(FindWindow(XRCID("IDC_LIST_VARIABLE1")),
							 wxListBox);
    if (FindWindow(XRCID("IDC_LIST_VARIABLE2")))
        pLB2 = wxDynamicCast(FindWindow(XRCID("IDC_LIST_VARIABLE2")),
							 wxListBox);
    if (FindWindow(XRCID("IDC_CHECK1")))
        pCheck = wxDynamicCast(FindWindow(XRCID("IDC_CHECK1")),
							   wxCheckBox);
    if (FindWindow(XRCID("IDC_COMBO_THEMATIC")))
        pCB = wxDynamicCast(FindWindow(XRCID("IDC_COMBO_THEMATIC")),
							wxComboBox);
}

void RateSmootherDlg::OnCListVariable1DoubleClicked( wxCommandEvent& event )
{
	OnOkClick(event);
}

void RateSmootherDlg::OnCListVariable2DoubleClicked( wxCommandEvent& event )
{
	OnOkClick(event);
}

void RateSmootherDlg::OnCCheck1Click( wxCommandEvent& event )
{
	m_CheckDefault = pCheck->GetValue();
}

void RateSmootherDlg::PumpingVariables()
{
	pLB1->Clear();
	pLB2->Clear();

	grid_base->FillNumericColIdMap(col_id_map);
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		pLB1->Append(grid_base->col_data[col_id_map[i]]->name);
		pLB2->Append(grid_base->col_data[col_id_map[i]]->name);
	}
	
	int sel1 = pLB1->FindString(m_Var1);
	if (sel1 == wxNOT_FOUND) sel1 = 0;
	pLB1->SetSelection(sel1);
	int sel2 = pLB2->FindString(m_Var2);
	if (sel2 == wxNOT_FOUND) sel2 = 0;
	pLB2->SetSelection(sel2);
}

void RateSmootherDlg::PumpingData()
{
	std::vector<double> data;
	grid_base->col_data[col_id_map[pLB1->GetSelection()]]->GetVec(data);
	for (int i=0; i<obs; i++) E[i] = data[i];

	grid_base->col_data[col_id_map[pLB2->GetSelection()]]->GetVec(data);
	for (int i=0; i<obs; i++) P[i] = data[i];
}

void RateSmootherDlg::OnOkClick( wxCommandEvent& event )
{
	m_Var1 = pLB1->GetString(pLB1->GetSelection());
	m_Var2 = pLB2->GetString(pLB2->GetSelection());
	
	m_theme = pCB->GetSelection();
	PumpingData();

	for (int i=0; i<obs; i++) {
		if (P[i] <= 0) {
			wxMessageBox("Error: base values contain non-positive numbers, "
						 "no rate computed.");
			OnCancelClick(event);
			return;
		}
	}

	switch (m_smoother) {
	case 1:
		ComputeSRS();
		break;
	case 2:
		ComputeEBS();
		break;
	case 3:
		ComputeSEBS();
		break;
	case 4:
		ComputeRawRate();
		break;
	case 5:
		ComputeExcessRate();
		break;
	case 9:
		ComputeRateStandardization();		
		break;
	default:
		break;
	};

	event.Skip();
	EndDialog(wxID_OK);
}


void RateSmootherDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
}


void RateSmootherDlg::ComputeSRS()
{
	GeoDaAlgs::RateSmoother_SRS(obs, m_gal, P, E, m_results, m_undef_r);
}

void RateSmootherDlg::ComputeEBS()
{
	GeoDaAlgs::RateSmoother_EBS(obs, P, E, m_results, m_undef_r);
}

void RateSmootherDlg::ComputeSEBS()
{
	GeoDaAlgs::RateSmoother_SEBS(obs, m_gal, P, E, m_results, m_undef_r);
}

void RateSmootherDlg::ComputeRawRate()
{
	GeoDaAlgs::RateSmoother_RawRate(obs, P, E, m_results, m_undef_r);
}

void RateSmootherDlg::ComputeExcessRate()
{
	GeoDaAlgs::RateSmoother_ExcessRisk(obs, P, E, m_results, m_undef_r);
}

bool RateSmootherDlg::ComputeRateStandardization()
{
    return GeoDaAlgs::RateStandardizeEB(obs, P, E, m_results, m_undef_r);
}

