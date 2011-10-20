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

#include <limits>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../OpenGeoDa.h"
#include "../TemplateCanvas.h"
#include "../Thiessen/VorDataType.h"
#include "../GenUtils.h"

#include "AddIdVariable.h"
#include "CreatingWeightDlg.h"

// This function calls ReadData which comes directly from DBF file.
extern double ComputeCutOffPoint(const wxString& infl, long obs,
								 int method, const wxString& v1,
								 const wxString& v2, vector<double>& x,
								 vector<double>& y,
								 bool mean_center);
extern	double ComputeMaxDistance(int, vector<double>&, vector<double>&,
								  int methods);
extern	GalElement	*HOContiguity(const int p, long obs, GalElement *W,
								  bool Lag);
extern  GwtElement* shp2gwt(int Obs, vector<double>& x, vector<double>& y,
							const double threshold, const int degree,
							int method);
extern  GwtElement* inv2gwt(int Obs, vector<double>& x, vector<double>& y,
							const double threshold, const int degree,
							int method, bool standardize);
// This function calls ReadData which comes directly from DBF file.
extern	GwtElement *DynKNN(const wxString& infl, int k, long obs, int method,
						   char *v1, char *v2, bool mean_center);
extern  bool WriteGwt( const GwtElement *g,
					  const wxString& ifname, const wxString& ofname, 
					  const wxString& vname, const long Obs, const int degree,
					  bool gl);

BEGIN_EVENT_TABLE( CreatingWeightDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_BROWSE_ISHP4W"),
			   CreatingWeightDlg::OnCBrowseIshp4wClick )
	EVT_BUTTON( XRCID("ID_CREATE_ID"),
			   CreatingWeightDlg::OnCreateNewIdClick )
    EVT_CHOICE(XRCID("IDC_IDVARIABLE"),
				  CreatingWeightDlg::OnCIdvariableSelected )
    EVT_CHOICE(XRCID("IDC_DISTANCE_METRIC"),
				  CreatingWeightDlg::OnDistanceMetricSelected )
    EVT_CHOICE(XRCID("IDC_XCOORDINATES"), CreatingWeightDlg::OnXSelected )
    EVT_CHOICE(XRCID("IDC_YCOORDINATES"), CreatingWeightDlg::OnYSelected )

	EVT_RADIOBUTTON( XRCID("IDC_RADIO_QUEEN"),
					CreatingWeightDlg::OnCRadioQueenSelected )
    EVT_SPIN( XRCID("IDC_SPIN_ORDEROFCONTIGUITY"),
			 CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_ROOK"),
					CreatingWeightDlg::OnCRadioRookSelected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_DISTANCE"),
					CreatingWeightDlg::OnCRadioDistanceSelected )
	EVT_TEXT( XRCID("IDC_THRESHOLD_EDIT"),
			 CreatingWeightDlg::OnCThresholdTextEdit )
    EVT_SLIDER( XRCID("IDC_THRESHOLD_SLIDER"),
			   CreatingWeightDlg::OnCThresholdSliderUpdated )

	//The following three event handlers are for the temporarily-deleted
	//Inverse Distance option.  See OpenGeoDa_orig.xrc for original dialog.
    EVT_RADIOBUTTON( XRCID("ID_INV_DIST_RADIOBUTTON"),
					CreatingWeightDlg::OnCRadioInvDistanceSelected )
    EVT_CHECKBOX( XRCID("ID_STANDARDIZE"),
				 CreatingWeightDlg::OnStandardizeClick )
    EVT_SPIN( XRCID("IDC_SPIN_POWER"),
			 CreatingWeightDlg::OnCSpinPowerUpdated )

    EVT_RADIOBUTTON( XRCID("IDC_RADIO_KNN"),
					CreatingWeightDlg::OnCRadioKnnSelected )
    EVT_SPIN( XRCID("IDC_SPIN_KNN"), CreatingWeightDlg::OnCSpinKnnUpdated )
    EVT_BUTTON( XRCID("IDOK_CREATE1"), CreatingWeightDlg::OnOkCreate1Click )
    EVT_BUTTON( XRCID("IDOK_RESET1"), CreatingWeightDlg::OnOkReset1Click )
    EVT_BUTTON( XRCID("wxID_CLOSE"), CreatingWeightDlg::OnCancelClick )
END_EVENT_TABLE()


CreatingWeightDlg::CreatingWeightDlg(wxWindow* parent,
									 Project* project_s, // can by NULL
									 wxWindowID id,
									 const wxString& caption,
									 const wxPoint& pos, 
									 const wxSize& size,
									 long style )
: all_init(false), m_thres_delta_factor(1.00001),
project(project_s) // can be NULL
{
	// If project != NULL, then we assume that there are no unsaved
	// changes to the associated Table.  The user can open various SHP
	// files, but whenever the SHP file matches the one in the
	// current project, we will synchronize Add ID results with the
	// current Table in memory as well as pass all created Weights files
	// to the current Weights Manager.
	m_method = 1;

	m_thres_val_valid = false;
	m_threshold_val = 0.01;
	m_iwfilesize = 0;
	m_done = false;

	Create(parent, id, caption, pos, size, style);

	all_init = true;
	
	m_neighbors->SetValue("4");
	m_contiguity->SetValue( "1");
	m_default_input_file = wxEmptyString;
	if (project) {
		// check if shp file name even exists.
		wxFileName shp_name(project->GetMainDir() + 
							project->GetMainName() + ".shp");
		wxFileName shx_name(project->GetMainDir() +
							project->GetMainName() + ".shx");
		if (shp_name.FileExists() && shx_name.FileExists()) {
			wxString sel_dbf_name = project->GetMainDir() + 
							project->GetMainName() + ".dbf";
			if (CheckIfDbfSameAsInCurrentProject(sel_dbf_name)) {
				if (!CheckProjectTableSaved()) {
					LOG_MSG("User did not not allow current "
							"Table to be saved.");
				} else {
					m_default_input_file = (project->GetMainDir() + 
											project->GetMainName() + ".shp");
				}
			}
		}
	}
	m_inputfile->SetValue(m_default_input_file);
	m_threshold->ChangeValue( "0.0");
	if (m_power) m_power->SetValue( "1");

	m_spincont->SetRange(1,10);
	m_spincont->SetValue(1);
	m_spinneigh->SetRange(1,10);
	m_spinneigh->SetValue(4);
	if (m_spinpower) m_spinpower->SetRange(1, 10);
	if (m_spinpower) m_spinpower->SetValue(1);
	m_radio = -1;

	wxString m_iShape = m_inputfile->GetValue();
 	if (m_iShape != wxEmptyString) OpenShapeFile();
	
	ResetThresXandYCombo();
    m_distance_metric->Append("<Euclidean Distance>");	
	m_distance_metric->Append("<Arc Distance (miles)>");
	m_distance_metric->SetSelection(0);

	OnReset();
	m_sliderdistance->SetValue(0);
	
	if (!m_default_input_file.IsEmpty()) OpenShapeFile();
}

bool CreatingWeightDlg::Create( wxWindow* parent, wxWindowID id,
								const wxString& caption, const wxPoint& pos,
								const wxSize& size, long style )
{
    m_inputfile = 0;
    m_field = 0;
    m_radio2 = 0;
    m_contiguity = 0;
    m_spincont = 0;
    m_radio1 = 0;
    m_include_lower = 0;
    m_distance_metric = 0;
    m_X = 0;
    m_Y = 0;
    m_radio3 = 0;
    m_threshold = 0;
    m_sliderdistance = 0;
    m_radio_inverse_distance = 0;
    m_standardize = 0;
    m_power = 0;
    m_spinpower = 0;
    m_radio4 = 0;
    m_neighbors = 0;
    m_spinneigh = 0;

    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    return true;
}

void CreatingWeightDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_WEIGHTS_FILE_CREATION");
    m_inputfile = XRCCTRL(*this, "ID_IN_SHP_TXT_CTRL", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_field = XRCCTRL(*this, "IDC_IDVARIABLE", wxChoice);
    m_contiguity = XRCCTRL(*this, "IDC_EDIT_ORDEROFCONTIGUITY", wxTextCtrl);
    m_spincont = XRCCTRL(*this, "IDC_SPIN_ORDEROFCONTIGUITY", wxSpinButton);
    m_include_lower = XRCCTRL(*this, "IDC_CHECK1", wxCheckBox);
    m_distance_metric = XRCCTRL(*this, "IDC_DISTANCE_METRIC", wxChoice);
    m_X = XRCCTRL(*this, "IDC_XCOORDINATES", wxChoice);
    m_Y = XRCCTRL(*this, "IDC_YCOORDINATES", wxChoice);
    m_threshold = XRCCTRL(*this, "IDC_THRESHOLD_EDIT", wxTextCtrl);
    m_sliderdistance = XRCCTRL(*this, "IDC_THRESHOLD_SLIDER", wxSlider);
    // m_standardize = XRCCTRL(*this, "ID_STANDARDIZE", wxCheckBox);
    // m_power = XRCCTRL(*this, "ID_TEXTCTRL4", wxTextCtrl);
    // m_spinpower = XRCCTRL(*this, "IDC_SPIN_POWER", wxSpinButton);
    m_radio2 = XRCCTRL(*this, "IDC_RADIO_QUEEN", wxRadioButton);
    m_radio1 = XRCCTRL(*this, "IDC_RADIO_ROOK", wxRadioButton);
    m_radio3 = XRCCTRL(*this, "IDC_RADIO_DISTANCE", wxRadioButton);
    m_radio4 = XRCCTRL(*this, "IDC_RADIO_KNN", wxRadioButton);
    //m_radio_inverse_distance = XRCCTRL(*this,"ID_INV_DIST_RADIOBUTTON",
	//    wxRadioButton);
    m_neighbors = XRCCTRL(*this, "IDC_EDIT_KNN", wxTextCtrl);
    m_spinneigh = XRCCTRL(*this, "IDC_SPIN_KNN", wxSpinButton);

    if (FindWindow(XRCID("ID_TEXTCTRL4")))
		FindWindow(XRCID("ID_TEXTCTRL4"))->SetValidator(
						wxTextValidator(wxFILTER_NUMERIC, & s_int) );
}

bool CreatingWeightDlg::CheckIfDbfSameAsInCurrentProject(
												const wxString& full_dbf_name)
{
	LOG_MSG("Entering CreatingWeightDlg::CheckIfDbfSameAsInCurrentProject");
	bool ret = false;
	wxString curr_dbf("");
	if (project) {
		curr_dbf = project->GetMainDir() + project->GetMainName() + ".dbf";
		LOG(curr_dbf);
	} else {
		LOG_MSG("no project open");
	}
	if (full_dbf_name.CmpNoCase(curr_dbf) == 0) {
		ret = true;
	} else {
		if (project) LOG_MSG("selected shp file name does not match "
							 "project shp file name");
	}
	return ret;
	LOG_MSG("Exiting CreatingWeightDlg::CheckIfDbfSameAsInCurrentProject");
}

// returns true if Table already synched with DBF file, or if user
// allowed Table changes to be saved.
bool CreatingWeightDlg::CheckProjectTableSaved()
{
	LOG_MSG("In CreatingWeightDlg::CheckProjectTableSaved");
	if (!project) return true;  // should only be called if project exists
	DbfGridTableBase* grid_base = project->GetGridBase();
	if (grid_base->ChangedSinceLastSave()) {
		LOG_MSG("Unsaved Table changes in open project.");
		wxString msg;
		msg << "Chosen input file matches currently open Table DBF file, ";
		msg << "and Table has unsaved changes. Weights creation for this ";
		msg << "input file can only proceed if Table changes are first ";
		msg << "saved. Press Yes to save current Table changes and create ";
		msg << "weights for current project. Press No to proceed to ";
		msg << "Weights Creation dialog where you can either choose a ";
		msg << "different input file or Close the dialog.";
		wxMessageDialog msgDlg(this, msg,
							   "Save current Table before proceeding?",
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
		if (msgDlg.ShowModal() != wxID_YES) return false;
		
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		grid_base->orig_header.year = timeinfo->tm_year+1900;
		grid_base->orig_header.month = timeinfo->tm_mon+1;
		grid_base->orig_header.day = timeinfo->tm_mday;
		DbfFileHeader t_header = grid_base->orig_header;
		
		wxString curr_dbf = project->GetMainDir() +
			project->GetMainName() + ".dbf";
		
		wxString err_msg;
		bool success = grid_base->WriteToDbf(curr_dbf, err_msg);
		if (!success) {
			grid_base->orig_header = t_header;
			wxMessageBox(err_msg);
			wxMessageDialog dlg (this, err_msg, "Error",
								 wxOK | wxICON_ERROR);
			LOG_MSG(err_msg);
			return false;
		} else {
			GeneralWxUtils::EnableMenuItem(MyFrame::theFrame->GetMenuBar(),
						XRCID("ID_NEW_TABLE_SAVE"),
						project->GetGridBase()->ChangedSinceLastSave());
			wxString msg("Table saved successfully");
			wxMessageDialog dlg (this, msg, "Success",
								 wxOK | wxICON_INFORMATION);
			dlg.ShowModal();
			LOG_MSG(msg);
			return true;
		}
	} else {
		// no changes to be saved
		return true;
	}
}

void CreatingWeightDlg::OnCBrowseIshp4wClick( wxCommandEvent& event )
{
    LOG_MSG("Entering CreatingWeightDlg::OnCBrowseIshp4wClick");
	wxFileName ifn(m_inputfile->GetValue());
	wxString defaultDir(ifn.GetFullPath());
	wxString defaultFile(ifn.GetFullName());
    wxFileDialog dlg(this,
					 "Choose an input Shape file.",
					 defaultDir,
					 defaultFile,
					 "Shape files (*.shp)|*.shp");

    if (dlg.ShowModal() == wxID_OK) {
		// dlg.GetPath returns the selected filename with complete path.
		wxFileName ifn(dlg.GetPath());
		wxString sel_dbf_name = ifn.GetPathWithSep() + ifn.GetName() + ".dbf";
		if (CheckIfDbfSameAsInCurrentProject(sel_dbf_name)) {
			if (!CheckProjectTableSaved()) {
				LOG_MSG("User did not not allow current Table to be saved.");
				return;
			}
		}
		OnReset();
		// dlg.GetFullPath returns the filename with path and extension.
		m_inputfile->SetValue(ifn.GetFullPath());
		OpenShapeFile();
	}
    LOG_MSG("Exiting CreatingWeightDlg::OnCBrowseIshp4wClick");
}

void CreatingWeightDlg::OnCreateNewIdClick( wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnCreateNewIdClick");
	wxString dbf_fname = GenUtils::swapExtension(m_inputfile->GetValue(),
												 "dbf");
	AddIdVariable dlg(dbf_fname, this);
    if (dlg.ShowModal() == wxID_OK) {
		// We know that the new id has been added to the dbf file.
		m_field->Insert(dlg.GetIdVarName(), 0);
		m_field->SetSelection(0);

		if (project && CheckIfDbfSameAsInCurrentProject(dbf_fname)) {
			LOG_MSG("Adding new id field to Table in memory");
			DbfGridTableBase* grid_base = project->GetGridBase();
			grid_base->InsertCol(0, GeoDaConst::long64_type,
								 dlg.GetIdVarName(),
								 GeoDaConst::default_dbf_long_len,
								 0, 0, false, true);
			std::vector<wxInt64> data(grid_base->GetNumberRows());
			for (wxInt64 i=0, iend=data.size(); i<iend; i++) data[i] = i+1;
			grid_base->col_data[0]->SetFromVec(data);
			grid_base->SetChangedSinceLastSave(false);
		}
		
		EnableDistanceRadioButtons(true &&
								   (m_field->GetSelection() != wxNOT_FOUND));
		EnableContiguityRadioButtons(!m_is_point_shp_file 
			&& (m_field->GetSelection() != wxNOT_FOUND));
		UpdateCreateButtonState();
	} else {
		// A new id was not added to the dbf file, so do nothing.
	}	
	LOG_MSG("Exiting CreatingWeightDlg::OnCreateNewIdClick");
}

void CreatingWeightDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}

void CreatingWeightDlg::OnOkCreate1Click( wxCommandEvent& event )
{
	if(m_radio == -1) {
		wxMessageBox("Error: Please select a weights matrix type.");
		return;
	}
	
	if (m_radio == 3 || m_radio == 7) {
		if (!m_thres_val_valid) {
			wxString msg;
			msg << "The currently entered threshold value is not ";
			msg << "a valid number.  Please move the slider, or enter ";
			msg << "a valid number.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			event.Skip();
			return;
		}
		if (m_threshold_val*m_thres_delta_factor < m_thres_min) {
			wxString msg;
			msg << "The currently entered threshold value of ";
			msg << m_threshold_val << " is less than ";
			msg << m_thres_min << " which is the minimum value for which ";
			msg << "there will be no neighborless observations (isolates). ";
			msg << "Press Yes to proceed anyhow, press No to abort.";
			wxMessageDialog dlg(this, msg, "Warning",
								wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if (dlg.ShowModal() != wxID_YES) {
				event.Skip();
				return;
			}
		}	
	}

	wxFileName ifn(m_inputfile->GetValue());
	wxString defaultDir(ifn.GetPath());
	wxString wildcard;
	if (IsSaveAsGwt()) {
		ifn.SetExt("gwt");
		wildcard = "GWT files (*.gwt)|*.gwt";
	} else {
		ifn.SetExt("gal");
		wildcard = "GAL files (*.gal)|*.gal";
	}
	wxString defaultFile(ifn.GetFullName());
	
    wxFileDialog dlg(this,
                     "Choose an output weights file name.",
                     defaultDir,
					 defaultFile,
					 wildcard,
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	wxString outputfile;
    if (dlg.ShowModal() == wxID_OK) {
		outputfile = dlg.GetPath();
	} else {
		return;
	}
	
	wxString id = wxEmptyString;
	if ( m_field->GetSelection() != wxNOT_FOUND ) {
		id = m_field->GetString(m_field->GetSelection());
	} else {
            return; // we must have key id variable
        }
	
	if( (m_field->GetSelection() != wxNOT_FOUND) && (!CheckID(id)) ) {
            return;
	}

	int m_ooC = m_spincont->GetValue();
	int m_kNN = m_spinneigh->GetValue();
	int m_alpha = 1;
	if (m_spinpower) m_alpha = m_spinpower->GetValue();

	wxString m_iShape = m_inputfile->GetValue();
	wxString m_oShape = outputfile;

	GalElement *gal = 0;
    GalElement *Hgal = 0;
	GwtElement *gwt = 0;
	bool done = false;

	m_method = m_distance_metric->GetSelection() + 1;
	
	//ms_X = m_X->GetValue();
	ms_X = m_X->GetString(m_X->GetSelection());
	//ms_Y = m_Y->GetValue();
	ms_Y = m_Y->GetString(m_Y->GetSelection());

	bool m_check1 = m_include_lower->GetValue();

	switch (m_radio)
	{
		case 7: // inverse distance
		{
			if (m_alpha > 0) {
				bool m_standardize_value = true;
				if (m_standardize) {
					m_standardize_value = m_standardize->GetValue();
				}
				double t_val = m_threshold_val;
				if (t_val <= 0) t_val = std::numeric_limits<float>::min();
				gwt = inv2gwt(m_iwfilesize, m_XCOO, m_YCOO,
							  t_val * m_thres_delta_factor,
							  m_alpha, m_method, m_standardize_value);

				if (gwt == 0) {
					wxMessageBox( "gwt NULL");
					return;
				}

				Shp2GalProgress(0, gwt, m_iShape, m_oShape, id,
								m_iwfilesize);
		
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			}
			else {
				wxString msg = "distance power alpha must be ";
				msg+="greater than 0";
				wxMessageBox(msg);
				return;
			}
		}
			break;
			
		case 3: // threshold distance
		{
			double t_val = m_threshold_val;
			if (t_val <= 0) t_val = std::numeric_limits<float>::min();
			if (t_val > 0) {
				gwt = shp2gwt(m_iwfilesize, m_XCOO, m_YCOO,
							  t_val * m_thres_delta_factor,
							  1, m_method);

				if (gwt == 0) {
					return;
				}
				Shp2GalProgress(0, gwt, m_iShape, m_oShape,
								id, m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			}
		}
			break;
			
		case 4: // k nn
		{
			if (m_kNN > 0 && m_kNN < m_iwfilesize) {
				char v1_b[512];
				strcpy( v1_b, (const char*)ms_X.mb_str(wxConvUTF8) );
				char* v1 = v1_b;
				char v2_b[512];
				strcpy( v2_b, (const char*)ms_Y.mb_str(wxConvUTF8) );
				char* v2 = v2_b;
				bool mean_center = false;
				if (ms_X ==  "<X-Centroids>")
					v1 = 0;
				if (ms_Y ==  "<Y-Centroids>") 
					v2 = 0;
				if (ms_X == "<X-Mean-Centers>") {
					v1 = 0;
					mean_center = true;
				}
				if (ms_Y == "<Y-Mean-Centers>") {
					v2 = 0;
					mean_center = true;
				}
								
				// Notes: 
				// The distance pairs computed in here are not the same with
				// SpaceStat. However, the k-neighbours identified are exactly
				// the same.	
				gwt = DynKNN(m_iShape, m_kNN+1, m_iwfilesize, m_method, v1, v2,
							 mean_center);
				if (gwt==0) {
					return;
				}
				Shp2GalProgress(0, gwt, m_iShape, m_oShape,
								id, m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
				delete [] gwt;
				gwt = 0;
			} else {
				wxString s;
				s.Format("Error: Maximum # of neighbours (%d) is reached!",
						 (int) m_iwfilesize-1);
				wxMessageBox(s);
			}
		}
			break;
			
		case 5:  // rook
		{
			gal = shp2gal(m_iShape, 1, true);
			if (gal==0)
				break;
			if (m_ooC > 1) {
				Hgal = HOContiguity(m_ooC, m_iwfilesize, gal, m_check1);
				Shp2GalProgress(Hgal, 0,m_iShape, m_oShape,id,m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			} else {
 		        Shp2GalProgress(gal, 0, m_iShape, m_oShape,id,m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			}
		}
			break;
			
		case 6: // queen
		{
			gal = shp2gal(m_iShape, 0, true);
			if (gal==0)
				break;
			if (m_ooC > 1) {
				Hgal = HOContiguity(m_ooC, m_iwfilesize, gal, m_check1);
				Shp2GalProgress(Hgal, 0,m_iShape, m_oShape,id,m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			}
			else {
				Shp2GalProgress(gal, 0, m_iShape, m_oShape,id,m_iwfilesize);
				done = true;
				event.Skip(); // wxDialog::OnOK(event);
			}
		}
			break;

		default:
			break;
	};
	
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
}

void CreatingWeightDlg::OnOkReset1Click( wxCommandEvent& event )
{
    OnReset();
}

void CreatingWeightDlg::OnCRadioRookSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(5);
}

void CreatingWeightDlg::OnCRadioQueenSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(6);
}

void CreatingWeightDlg::EnableThresholdControls( bool b )
{
	// This either enable the Threshold distance controls.  This does not
	// affect the state of the Theshold button itself.
	FindWindow(XRCID("IDC_STATIC1"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC2"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC3"))->Enable(b);
	FindWindow(XRCID("IDC_XCOORDINATES"))->Enable(b);
	FindWindow(XRCID("IDC_YCOORDINATES"))->Enable(b);
	FindWindow(XRCID("IDC_DISTANCE_METRIC"))->Enable(b);
	FindWindow(XRCID("IDC_THRESHOLD_SLIDER"))->Enable(b);
	FindWindow(XRCID("IDC_THRESHOLD_EDIT"))->Enable(b);
}

void CreatingWeightDlg::SetRadioBtnAndAssocWidgets(int radio)
{
	// Updates the value of m_radio and disables
	// wigets associated with deslectd radio buttons.
	
	// Disable everything to begin.	
	FindWindow(XRCID("IDC_STATIC_OOC1"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY"))->Enable(false);
	FindWindow(XRCID("IDC_SPIN_ORDEROFCONTIGUITY"))->Enable(false);
	FindWindow(XRCID("IDC_CHECK1"))->Enable(false);
	EnableThresholdControls(false);
	FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_KNN"))->Enable(false);
    FindWindow(XRCID("IDC_SPIN_KNN"))->Enable(false);
	if (FindWindow(XRCID("ID_TEXTCTRL4")))
		FindWindow(XRCID("ID_TEXTCTRL4"))->Enable(false); 
	if (FindWindow(XRCID("IDC_SPIN_POWER")))
		FindWindow(XRCID("IDC_SPIN_POWER"))->Enable(false);
	if (FindWindow(XRCID("ID_STANDARDIZE")))
		FindWindow(XRCID("ID_STANDARDIZE"))->Enable(false);

	if ((radio == 7) || (radio == 3) || (radio == 4) ||
		(radio == 5) || (radio == 6)) {
		m_radio = radio;
	} else {
		m_radio = -1;
	}
	UpdateCreateButtonState();
	
	switch (m_radio) {
		case 6: // queen
		case 5: { // rook
			FindWindow(XRCID("IDC_STATIC_OOC1"))->Enable(true);
			FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY"))->Enable(true);
			FindWindow(XRCID("IDC_SPIN_ORDEROFCONTIGUITY"))->Enable(true);
			FindWindow(XRCID("IDC_CHECK1"))->Enable(true);
		}
			break;
		case 3: { // threshold distance
			EnableThresholdControls(true);
		}
			break;
		case 4: { // k-nn
			FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(true);
			FindWindow(XRCID("IDC_EDIT_KNN"))->Enable(true);
			FindWindow(XRCID("IDC_SPIN_KNN"))->Enable(true);			
		}
			break;
		case 7: { // inverse distance
			if (FindWindow(XRCID("ID_TEXTCTRL4")))
				FindWindow(XRCID("ID_TEXTCTRL4"))->Enable(true); 
			if (FindWindow(XRCID("IDC_SPIN_POWER")))
				FindWindow(XRCID("IDC_SPIN_POWER"))->Enable(true);
			if (FindWindow(XRCID("ID_STANDARDIZE")))
				FindWindow(XRCID("ID_STANDARDIZE"))->Enable(true);			
		}
			break;
		default:
			break;
	}
}

// This function is only called when one of the choices that affects
// the entire threshold scale is changed.  This function will use
// the current position of the slider
void CreatingWeightDlg::UpdateThresholdValues()
{
	LOG_MSG("Entering CreatingWeightDlg::UpdateThresholdValues");
	if (!all_init) return;
	wxString mm_x = m_X->GetString(m_X->GetSelection());
	wxString mm_y = m_Y->GetString(m_Y->GetSelection());
	wxString v1 = mm_x;
	wxString v2 = mm_y;
	
	bool mean_center = false;
	if (mm_x ==  "<X-Centroids>")
		v1 = wxEmptyString;
	if (mm_y ==  "<Y-Centroids>") 
		v2 = wxEmptyString;
	if (mm_x == "<X-Mean-Centers>") {
		v1 = wxEmptyString;
		mean_center = true;
	}
	if (mm_y == "<Y-Mean-Centers>") {
		v2 = wxEmptyString;
		mean_center = true;
	}
	LOG(v1);
	LOG(v2);
	
	m_iwfilesize = GetShpFileSize(m_inputfile->GetValue());
	m_thres_min = ComputeCutOffPoint(m_inputfile->GetValue(),
									 m_iwfilesize, m_method, v1, v2,
									 m_XCOO, m_YCOO, mean_center);
	m_thres_max = ComputeMaxDistance(m_iwfilesize, m_XCOO, m_YCOO, m_method);
	LOG(m_thres_min);
	LOG(m_thres_max);
	m_threshold_val = (m_sliderdistance->GetValue() *
					   (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_thres_val_valid = true;
	m_threshold->ChangeValue( wxString::Format("%f", m_threshold_val));
	LOG_MSG("Exiting CreatingWeightDlg::UpdateThresholdValues");
}

void CreatingWeightDlg::OnCThresholdTextEdit( wxCommandEvent& event )
{
	if (!all_init) return;
	LOG_MSG("In CreatingWeightDlg::OnCThresholdTextEdit");
	wxString val = m_threshold->GetValue();
	val.Trim(false);
	val.Trim(true);
	double t = m_threshold_val;
	m_thres_val_valid = val.ToDouble(&t);
	if (m_thres_val_valid) {
		m_threshold_val = t;
		if (t <= m_thres_min) {
			m_sliderdistance->SetValue(0);
		} else if (t >= m_thres_max) {
			m_sliderdistance->SetValue(100);
		} else {
			double s = (t-m_thres_min)/(m_thres_max-m_thres_min) * 100;
			m_sliderdistance->SetValue((int) s);
		}
	}
}

void CreatingWeightDlg::OnCThresholdSliderUpdated( wxCommandEvent& event )
{
	if (!all_init) return;
	bool m_rad_inv_dis_val = false;
	if (m_radio_inverse_distance) {
		m_rad_inv_dis_val = m_radio_inverse_distance->GetValue();
	}
	
	m_threshold_val = (m_sliderdistance->GetValue() *
					   (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_threshold->ChangeValue( wxString::Format("%f", (double) m_threshold_val));
	if (m_threshold_val > 0)  {
	    FindWindow(XRCID("IDOK_CREATE1"))->Enable(true);
	}
}

void CreatingWeightDlg::OnCRadioDistanceSelected( wxCommandEvent& event )
{
	// Threshold Distance radio button selected
	SetRadioBtnAndAssocWidgets(3);
	UpdateThresholdValues();
}


void CreatingWeightDlg::OnCRadioGeodaLSelected( wxCommandEvent& event )
{
	// do nothing for now, perhaps will force save as GAL in future.
    event.Skip();
}

void CreatingWeightDlg::OnCRadioKnnSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(4);
}

void CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event )
{
	wxString val;
	val << m_spincont->GetValue();
    m_contiguity->SetValue(val);
}

void CreatingWeightDlg::OnCSpinKnnUpdated( wxSpinEvent& event )
{
	wxString val;
	val << m_spinneigh->GetValue();
	m_neighbors->SetValue(val);
}

// updates the enable/disable state of the Create button based
// on the values of various other controls.
void CreatingWeightDlg::UpdateCreateButtonState()
{
	bool enable = true;

	// Check that a Weights File ID variable is selected.
	if (m_field->GetSelection() == wxNOT_FOUND)
		enable = false;
	// Check that a weight type radio button choice is selected.
	if (m_radio == -1)
		enable = false;
	
	FindWindow(XRCID("IDOK_CREATE1"))->Enable(enable);
}

void CreatingWeightDlg::EnableContiguityRadioButtons(bool b)
{
	FindWindow(XRCID("IDC_RADIO_ROOK"))->Enable(b);
	FindWindow(XRCID("IDC_RADIO_QUEEN"))->Enable(b);
}

void CreatingWeightDlg::EnableDistanceRadioButtons(bool b)
{
	FindWindow(XRCID("IDC_RADIO_DISTANCE"))->Enable(b);	
	FindWindow(XRCID("IDC_RADIO_KNN"))->Enable(b);
	if (FindWindow(XRCID("ID_INV_DIST_RADIOBUTTON")))
		FindWindow(XRCID("ID_INV_DIST_RADIOBUTTON"))->Enable(b);
}

void CreatingWeightDlg::ClearRadioButtons()
{
	m_radio1->SetValue(false);
	m_radio2->SetValue(false);
	m_radio3->SetValue(false);
	m_radio4->SetValue(false);
	if (m_radio_inverse_distance) m_radio_inverse_distance->SetValue(false);
	m_radio = -1;
}

void CreatingWeightDlg::ResetThresXandYCombo()
{
	m_X->Clear();
	m_X->Append("<X-Centroids>");
	m_X->Append("<X-Mean-Centers>");
	m_X->SetSelection(0);
	ms_X = m_X->GetString(0);
	m_Y->Clear();
	m_Y->Append("<Y-Centroids>");
	m_Y->Append("<Y-Mean-Centers>");
	m_Y->SetSelection(0);
	ms_Y = m_Y->GetString(0);
}

void CreatingWeightDlg::PumpingVariables()
{
	ResetThresXandYCombo();
	iDBF tb(m_inputfile->GetValue());
	if (tb.IsConnectedToFile()) {
		int numfields;
		numfields = tb.GetNumOfField();

		for (int i=0; i<numfields; i++) {
			if ((tb.GetFieldType(i)=='N' || tb.GetFieldType(i)=='F')) {
			    wxString fnm(tb.GetFieldName(i), wxConvUTF8);
				if (tb.GetFieldPrecision(i) == 0) m_field->Append(fnm);
				m_X->Append(fnm);
				m_Y->Append(fnm);
			}
		}
		m_X->SetSelection(0);
		m_Y->SetSelection(0);
	} else {
		OnReset();
	}
}

bool CreatingWeightDlg::CheckID(const wxString& id)
{
	wxFileName dbf_fn(m_inputfile->GetValue());
	dbf_fn.SetExt("dbf");
	DbfFileReader dbf(dbf_fn.GetFullPath());	
	
	if (!dbf.isDbfReadSuccess()) {
		wxString msg = "Error: a problem was encountered while reading ";
		msg += "the dbf file \"" + dbf_fn.GetFullPath();
		return false;
	}
	if (GetShpFileSize(m_inputfile->GetValue()) != dbf.getNumRecords()) {
		wxString msg = "Error: Number of records in SHP file do not ";
		msg += "match number of records in DBF file.";
		wxMessageBox(msg);
		return false;
	}
	if (dbf.getFieldDesc(id).decimals != 0) {
		wxString msg = id + " has values that are not integers.  Please ";
		msg += "choose a different ID Variable.";
		wxMessageBox(msg);
		return false;
	}
	if (!dbf.isFieldValUnique(id)) {
		wxString msg = id + " has duplicate values.  Please choose ";
		msg += "a different ID Variable.";
		wxMessageBox(msg);
		return false;
	}
	return true;
}

void CreatingWeightDlg::OpenShapeFile()
{
	FindWindow(XRCID("ID_ID_VAR_STAT_TXT"))->Enable(true);
	FindWindow(XRCID("IDC_IDVARIABLE"))->Enable(true);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(true);
	FindWindow(XRCID("IDOK_RESET1"))->Enable(true);
	FindWindow(XRCID("IDOK_CREATE1"))->Enable(false);

	m_is_point_shp_file = IsPointShapeFile(m_inputfile->GetValue());
	m_iwfilesize = GetShpFileSize(m_inputfile->GetValue());

	m_field->Clear();
	PumpingVariables();

	m_spincont->SetRange(1, (int) m_iwfilesize / 2);
	m_spinneigh->SetRange(1, (int) m_iwfilesize - 1);

	if (m_radio1->GetValue()) m_radio = 5;
	else if (m_radio2->GetValue()) m_radio = 6;
	else if (m_radio3->GetValue()) m_radio = 3;
	else if (m_radio4->GetValue()) m_radio = 4;
	else if (m_radio_inverse_distance != 0 &&
			 m_radio_inverse_distance->GetValue()) m_radio = 7;

	m_XCOO.resize(m_iwfilesize);
	m_YCOO.resize(m_iwfilesize);
}

void CreatingWeightDlg::OnReset()
{
	m_inputfile->SetValue( m_default_input_file );
	m_field->Clear();
	m_contiguity->SetValue( "1");
	m_distance_metric->SetSelection(0);
	ResetThresXandYCombo();
	m_sliderdistance->SetRange(0, 100);
	m_sliderdistance->SetValue(0);
	m_threshold->SetValue( "0.0");
	m_spincont->SetRange(1,10);
	m_spincont->SetValue(1);
	m_spinneigh->SetRange(1,10);
	m_spinneigh->SetValue(4);
	m_neighbors->SetValue( "4");
	if (m_spinpower) m_spinpower->SetRange(1, 10);
	if (m_spinpower) m_spinpower->SetValue(1);
	if (m_power) m_power->SetValue("1");
	FindWindow(XRCID("IDOK_RESET1"))->Enable(false);
	FindWindow(XRCID("IDOK_CREATE1"))->Enable(false);
	FindWindow(XRCID("ID_ID_VAR_STAT_TXT"))->Enable(false);
	FindWindow(XRCID("IDC_IDVARIABLE"))->Enable(false);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(false);

	ClearRadioButtons();
	SetRadioBtnAndAssocWidgets(-1);
	EnableContiguityRadioButtons(false);
	EnableDistanceRadioButtons(false);
	
	if(!m_default_input_file.IsEmpty()) OpenShapeFile();
}

bool CreatingWeightDlg::IsSaveAsGwt()
{
	// determine if save type will be GWT or GAL.
	// m_radio values:
	// 7 - inverse distance - GWT
	// 3 - threshold distance - GWT
	// 4 - k-nn - GWT
	// 5 - rook - GAL
	// 6 - queen - GAL
	return 	!((m_radio == 5) || (m_radio == 6));	
}

void CreatingWeightDlg::OnXSelected(wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnXSelected");
	if ( m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" && 
		m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" ) {
		m_Y->SetSelection(0);
	}
	if ( m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" && 
		m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" ) {
		m_Y->SetSelection(1);
	}
	UpdateThresholdValues();
	LOG_MSG("Exiting CreatingWeightDlg::OnXSelected");	
}

void CreatingWeightDlg::OnYSelected(wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnYSelected");
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" && 
		m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" ) {
		m_X->SetSelection(0);
	}
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" && 
		m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" ) {
		m_X->SetSelection(1);
	}
	UpdateThresholdValues();
	LOG_MSG("Exiting CreatingWeightDlg::OnYSelected");
}

void CreatingWeightDlg::OnDistanceMetricSelected(wxCommandEvent& event )
{
	m_method = m_distance_metric->GetSelection() + 1;
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnCIdvariableSelected( wxCommandEvent& event )
{
	EnableDistanceRadioButtons(true && (m_field->GetSelection() != wxNOT_FOUND));
	EnableContiguityRadioButtons(!m_is_point_shp_file
							 && (m_field->GetSelection() != wxNOT_FOUND));
	UpdateCreateButtonState();	
}

bool CreatingWeightDlg::Shp2GalProgress(GalElement *fu, GwtElement *gw,
										const wxString& ifn,
										const wxString& ofn,
										const wxString& idd,
										long Obs)
{
	FindWindow(XRCID("IDOK_RESET1"))->Enable(false);
	FindWindow(XRCID("IDOK_CREATE1"))->Enable(false);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(false);

	bool success = false;
	bool flag = false;
	bool geodaL=true; // always save as "Legacy" format.
	if (fu) // gal
		flag = SaveGal(fu, ifn, ofn, idd, Obs);
	else if (m_radio == 3) // binary distance
		flag = WriteGwt(gw, ifn, ofn, idd, Obs, 1, geodaL );
	else if (m_radio == 4) // kNN
		flag = WriteGwt(gw, ifn, ofn, idd, Obs, -2, geodaL);
	else if (m_radio == 7) // inverse distance weight
		flag = WriteGwt(gw, ifn, ofn, idd, Obs, 2, geodaL);
	else flag = false;

	if (!flag) {
		wxMessageBox("Error: Failed to create the weights file.");
	} else {
		wxFileName t_ofn(ofn);
		wxString fn(t_ofn.GetFullName());
		
		wxString msg = wxEmptyString;
		msg = "Weights file \"" + fn + "\" created successfully.";
		wxMessageBox(msg);
		success = true;
	}

	// try to load just-created GAL/GWT if shp file is associated with
	// currently opened project
	wxFileName t_ifn(m_inputfile->GetValue());
	wxString dbf_fname = t_ifn.GetPathWithSep() + t_ifn.GetName() + ".dbf";
	if (project && success && CheckIfDbfSameAsInCurrentProject(dbf_fname)) {
		wxFileName t_ofn(ofn);
		wxString ext = t_ofn.GetExt().Lower();
		if (ext != "gal" && ext != "gwt") {
			LOG_MSG("File extention not gal or gwt");
		} else {
			DbfGridTableBase* grid_base = project->GetGridBase();
			WeightsManager* w_manager = project->GetWManager();
			
			int obs = w_manager->GetNumObservations();
			if (ext == "gal") {
				GalElement* tempGal=WeightUtils::ReadGal(ofn, grid_base);
				if (tempGal != 0) {
					GalWeight* w = new GalWeight();
					w->num_obs = obs;
					w->wflnm = ofn;
					w->gal = tempGal;
					if (!w_manager->AddWeightFile(w, true)) {
						delete w;
						success = false;
					}
				}
			} else { // ext == "gwt"
				GalElement* tempGal=WeightUtils::ReadGwtAsGal(ofn, grid_base);
				if (tempGal != 0) {
					GalWeight* w = new GalWeight();
					w->num_obs = obs;
					w->wflnm = ofn;
					w->gal = tempGal;
					if (!w_manager->AddWeightFile(w, true)) {
						delete w;
						success = false;
					}
				}
			}
		}
	}
	
	FindWindow(XRCID("IDOK_RESET1"))->Enable(true);
	FindWindow(XRCID("IDOK_CREATE1"))->Enable(true);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
	return success;
}

// inverse distance radio button selected
void CreatingWeightDlg::OnCRadioInvDistanceSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(7);
	
	// wxString mm_x = m_X->GetValue();
	wxString mm_x = m_X->GetString(m_X->GetSelection());
	// wxString mm_y = m_Y->GetValue();
	wxString mm_y = m_Y->GetString(m_Y->GetSelection());
	char b_v1[512];
	strcpy( b_v1, (const char*)mm_x.mb_str(wxConvUTF8) );
	char b_v2[512];
	strcpy( b_v2, (const char*)mm_y.mb_str(wxConvUTF8) );
	char *v1 = b_v1;
	char *v2 = b_v2;

	bool mean_center = false;
	if (mm_x == "<X-Centroids>")
		v1 = 0;
	if (mm_y == "<Y-Centroids>") 
		v2 = 0;
	if (mm_x == "<X-Mean-Centers>") {
		v1 = 0;
		mean_center = true;
	}
	if (mm_y == "<Y-Mean-Centers>") {
		v2 = 0;
		mean_center = true;
	}
	
	m_thres_min = ComputeCutOffPoint(m_inputfile->GetValue() ,m_iwfilesize,
									 m_method, v1, v2, m_XCOO, m_YCOO,
									 mean_center);
	m_thres_max = ComputeMaxDistance(m_iwfilesize, m_XCOO, m_YCOO, m_method);
	m_threshold_val = (m_sliderdistance->GetValue() *
					   (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_threshold->SetValue( wxString::Format("%f", m_threshold_val));
}

void CreatingWeightDlg::OnCSpinPowerUpdated( wxSpinEvent& event )
{
	wxString val;
	int ival = 1;
	if (m_spinpower) ival = m_spinpower->GetValue();
	val << ival;
    if (m_power) m_power->SetValue(val);
}

void CreatingWeightDlg::OnStandardizeClick( wxCommandEvent& event )
{
    event.Skip();
//	m_standardized = !m_standardized;
}


