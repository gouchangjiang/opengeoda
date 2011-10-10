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

#ifndef __GEODA_CENTER_RATE_SMOOTHER_DLG_H__
#define __GEODA_CENTER_RATE_SMOOTHER_DLG_H__

#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/listbox.h>

class GalElement;
class DbfGridTableBase;

class RateSmootherDlg: public wxDialog
{    
public:
	RateSmootherDlg(DbfGridTableBase* grid_base,
					wxString v1, wxString v2, 
					double* d1, bool m_VarDef, short smoother,
					GalElement* gal=0, wxWindow* pParent=0);   

	virtual ~RateSmootherDlg();	
	
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Rate Smoothing",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnCListVariable1DoubleClicked( wxCommandEvent& event );
    void OnCListVariable2DoubleClicked( wxCommandEvent& event );
    void OnCCheck1Click( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );

    wxListBox* pLB1;
    wxListBox* pLB2;
    wxCheckBox* pCheck;
    wxComboBox* pCB;
	bool	 F;
	wxString m_Var1;
	wxString m_Var2;
	bool m_CheckDefault;
	long obs;

	double*	E;
	double* P;
	double* m_results;

	GalElement* m_gal;
	short	m_smoother;
	std::vector<bool> m_undef_r;	
	int m_theme;

	void PumpingVariables();
	void PumpingData();	

	void ComputeSRS();
	void ComputeEBS();
	void ComputeSEBS();
	void ComputeRawRate();
	void ComputeExcessRate();

	bool ComputeRateStandardization();
	
	DbfGridTableBase* grid_base;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed in wxGrid
	std::vector<int> col_id_map;
	
	DECLARE_EVENT_TABLE()
};

#endif

