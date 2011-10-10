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

#ifndef __GEODA_CENTER_CREATING_WEIGHT_DLG_H__
#define __GEODA_CENTER_CREATING_WEIGHT_DLG_H__

#include <vector>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

class wxSpinButton;
class GalElement;
class GwtElement;
class Project;

class CreatingWeightDlg: public wxDialog
{
public:
    CreatingWeightDlg(wxWindow* parent,
					  Project* project = 0, // can be NULL
					  wxWindowID id = -1,
					  const wxString& caption = "Weights File Creation",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxCAPTION|wxSYSTEM_MENU );
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Weights File Creation",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );
    void CreateControls();
    void OnCBrowseIshp4wClick( wxCommandEvent& event );
	void OnCreateNewIdClick( wxCommandEvent& event );
    void OnCIdvariableSelected( wxCommandEvent& event );
    void OnDistanceMetricSelected(wxCommandEvent& event );
    void OnXSelected(wxCommandEvent& event );
    void OnYSelected(wxCommandEvent& event );
    void OnCRadioQueenSelected( wxCommandEvent& event );
    void OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event );
    void OnCRadioRookSelected( wxCommandEvent& event );
    void OnCRadioDistanceSelected( wxCommandEvent& event );
    void OnStandardizeClick( wxCommandEvent& event );
	void OnCThresholdTextEdit( wxCommandEvent& event );
    void OnCThresholdSliderUpdated( wxCommandEvent& event );
    void OnCRadioInvDistanceSelected( wxCommandEvent& event );
    void OnCSpinPowerUpdated( wxSpinEvent& event );
    void OnCRadioKnnSelected( wxCommandEvent& event );
    void OnCRadioGeodaLSelected( wxCommandEvent& event );
    void OnCSpinKnnUpdated( wxSpinEvent& event );
    void OnOkCreate1Click( wxCommandEvent& event );
    void OnOkReset1Click( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
	bool CheckIfDbfSameAsInCurrentProject(const wxString& full_dbf_name);
	bool CheckProjectTableSaved();

	bool all_init;
	wxString m_default_input_file;
    wxTextCtrl* m_inputfile;
    wxChoice* m_field;
    wxRadioButton* m_radio2;
    wxTextCtrl* m_contiguity;
    wxSpinButton* m_spincont;
    wxRadioButton* m_radio1;
    wxCheckBox* m_include_lower;
    wxChoice* m_distance_metric;
    wxCheckBox* m_standardize;  // for inverse distance
    wxChoice* m_X;
    wxChoice* m_Y;
    wxRadioButton* m_radio3;
    wxTextCtrl* m_threshold;
    wxSlider* m_sliderdistance;
    wxRadioButton* m_radio_inverse_distance; // for inverse distance
    wxTextCtrl* m_power; // for inverse distance
    wxSpinButton* m_spinpower; // for inverse distance
    wxRadioButton* m_radio4;
    wxCheckBox* m_radio9;
    wxTextCtrl* m_neighbors;
    wxSpinButton* m_spinneigh;

	Project* project; // can be NULL
	
	bool				m_is_point_shp_file;
	int					m_radio;
	int					m_iwfilesize;
	bool				m_done;
	double				m_thres_min; // minimum to avoid isolates
	double				m_thres_max; // maxiumum to include everything
	double				m_threshold_val;
	double				m_thres_val_valid;
	const double		m_thres_delta_factor;
	
	wxString			m_id;
	wxString			ms_X;
	wxString			ms_Y;
	int					m_method;  // 1 == Euclidean Dist, 2 = Arc Dist
	int					m_pos;
	std::vector<double>	m_XCOO;
	std::vector<double>	m_YCOO;
	wxString			fn;

	// updates the enable/disable state of the Create button based
	// on the values of various other controls.
	void UpdateCreateButtonState();
	void SetRadioBtnAndAssocWidgets(int radio);
	void UpdateThresholdValues();
	void ResetThresXandYCombo();
	void EnableThresholdControls(bool b);
	void EnableContiguityRadioButtons(bool b);
	void EnableDistanceRadioButtons(bool b);
	void ClearRadioButtons();
	void PumpingVariables();
	bool CheckID(const wxString& id);
	void OpenShapeFile();
    void OnReset();
	bool IsSaveAsGwt(); // determine if save type will be GWT or GAL.
    bool Shp2GalProgress(GalElement *fu, GwtElement *gw,
						 const wxString& ifn, const wxString& ofn,
						 const wxString& idd, long Obs);
   
	wxString s_int;
	
	DECLARE_EVENT_TABLE()
};

#endif

