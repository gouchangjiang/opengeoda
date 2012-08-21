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

#include <time.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces

#include "../FramesManager.h"
#include "../OpenGeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/DbfFile.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "SaveToTableDlg.h"
#include "../TemplateCanvas.h"
#include "SelectWeightDlg.h"
#include "ProgressDlg.h"
#include "../Regression/mix.h"
#include "../Regression/Lite2.h"
#include "../Regression/PowerLag.h"
#include "../Regression/ML_im.h"
#include "../Regression/smile.h"
#include "../Regression/DiagnosticReport.h"
#include "../GenUtils.h"
#include "../logger.h"

#include "RegressionDlg.h"

void Compute_MoranI(const GalElement* g, double *resid, int dim, double* rst);
void Compute_RSLmError(const GalElement* g, double *resid, int dim,
					   double* rst);
void Compute_RSLmErrorRobust(const GalElement* g, double** cov, DenseVector y,
							 DenseVector *x, DenseVector ols, double *resid,
							 int dim, int expl,	double* rst);
void Compute_RSLmLag(const GalElement* g, double** cov, DenseVector y,
					 DenseVector *x, DenseVector ols, double *resid,
					 int dim, int expl, double* rst);
void Compute_RSLmLagRobust(const GalElement* g,	double** cov, DenseVector y,
						   DenseVector *x, DenseVector ols, double *resid,
						   int dim, int expl, double* rst);
void Compute_RSLmSarma(const GalElement* g, double** cov, DenseVector y,
					   DenseVector *x, DenseVector ols,	double *resid,
					   int dim, int expl, double* rst);

//=====
extern inline double ComputeMean(double *dt, int dim);
extern inline double ComputeSdev(double *dt, int dim);

void ReportRegression( const char *ofl, wxString *vname, int obs, int deps,
					  int nResult, double *result, double *resid, double *yHat,
					  double *y, double **x, double **cov, GalElement *w);

bool classicalRegression( const GalElement *g, int num_obs, double * Y,
						 int dim, double ** X, 
						 int expl, DiagnosticReport *dr, bool InclConstant,
						 bool m_moranz, wxGauge* gauge = 0);

bool spatialLagRegression( const GalElement *g, int num_obs, double * Y,
						  int dim, double ** X, int deps, DiagnosticReport *dr,
						  bool InclConstant, wxGauge* p_bar = 0) ;

bool spatialErrorRegression( const GalElement *g, int num_obs, double * Y,
							int dim, double ** XX, int deps,
							DiagnosticReport *rr, 
							bool InclConstant, wxGauge* p_bar = 0);

BEGIN_EVENT_TABLE( RegressionDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_RUN"), RegressionDlg::OnRunClick )
    EVT_BUTTON( XRCID("ID_VIEW_RESULTS"), RegressionDlg::OnViewResultsClick )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VARIN"),
					   RegressionDlg::OnCListVarinDoubleClicked )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VAROUT"),
					   RegressionDlg::OnCListVaroutDoubleClicked )
    EVT_CHECKBOX( XRCID("ID_STANDARDIZE"), RegressionDlg::OnStandardizeClick )
    EVT_BUTTON( XRCID("IDC_BUTTON1"), RegressionDlg::OnCButton1Click )
    EVT_BUTTON( XRCID("IDC_BUTTON2"), RegressionDlg::OnCButton2Click )
    EVT_BUTTON( XRCID("IDC_RESET"), RegressionDlg::OnCResetClick )
    EVT_BUTTON( XRCID("IDC_BUTTON3"), RegressionDlg::OnCButton3Click )
    EVT_BUTTON( XRCID("IDC_BUTTON4"), RegressionDlg::OnCButton4Click )
    EVT_BUTTON( XRCID("IDC_BUTTON5"), RegressionDlg::OnCButton5Click )
    EVT_CHECKBOX( XRCID("IDC_WEIGHT_CHECK"),
				 RegressionDlg::OnCWeightCheckClick )
    EVT_BUTTON( XRCID("IDC_SAVE_REGRESSION"),
			   RegressionDlg::OnCSaveRegressionClick )
    EVT_BUTTON( XRCID("wxID_CLOSE"), RegressionDlg::OnCloseClick )
	EVT_MENU( XRCID("wxID_CLOSE"), RegressionDlg::OnCloseClick )
	EVT_CLOSE( RegressionDlg::OnClose )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO1"), RegressionDlg::OnCRadio1Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO2"), RegressionDlg::OnCRadio2Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO3"), RegressionDlg::OnCRadio3Selected )
    EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"), RegressionDlg::OnCOpenWeightClick )
END_EVENT_TABLE()


RegressionDlg::RegressionDlg(Project* project_s,
							 wxWindow* parent, int model, 
							 wxString title, wxString outputfilename,
							 bool check1, bool check2, bool check3,
							 wxWindowID id, const wxString& caption,
							 const wxPoint& pos, const wxSize& size,
							 long style )
: project(project_s), frames_manager(project_s->GetFramesManager()),
w_manager(project_s->GetWManager()),
grid_base(project_s->GetGridBase())
{
	Create(parent, id, caption, pos, size, style);

	m_choice->Clear();
	for (int i=0; i<w_manager->GetNumWeights(); i++) {
		m_choice->Append(w_manager->GetWFilename(i));
	}
	if (w_manager->GetCurrWeightInd() != -1) {
		m_choice->SetSelection(w_manager->GetCurrWeightInd());
	}
	
	RegressModel = model;

	lastSelection = 1;
	nVarName = 0;
	m_Run = false;
	m_OpenDump = false;
	m_moranz= m_output1 = m_output2 = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	m_output1 = check1;
	m_output2 = check2;
	m_moranz  = check3;

	m_title = title;
	m_syslogfilename = outputfilename;

	logFile = new Syslog();
	logFile->open(m_syslogfilename.mb_str(), ofstream::app);
	printHeader();
	
	InitVariableList();
	frames_manager->registerObserver(this);
}

RegressionDlg::~RegressionDlg()
{
	LOG_MSG("Entering RegressionDlg::~RegressionDlg");
	frames_manager->removeObserver(this);
	LOG_MSG("Exiting RegressionDlg::~RegressionDlg");
}

bool RegressionDlg::Create(wxWindow* parent, wxWindowID id,
							const wxString& caption, const wxPoint& pos,
							const wxSize& size, long style )
{
    m_varlist = NULL;
    m_dependent = NULL;
    m_independentlist = NULL;
    m_CheckConstant = NULL;
    m_CheckWeight = NULL;
    m_standardize = NULL;
    m_choice = NULL;
    m_radio1 = NULL;
    m_radio2 = NULL;
    m_radio3 = NULL;
    m_gauge = NULL;
	m_gauge_text = NULL;

    SetParent(parent);
    CreateControls();
    Centre();

    return true;
}

void RegressionDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_REGRESSION");
    m_varlist = XRCCTRL(*this, "IDC_LIST_VARIN", wxListBox);
    m_dependent = XRCCTRL(*this, "IDC_DEPENDENT_VAR", wxTextCtrl);
	m_dependent->SetMaxLength(0);
    m_independentlist = XRCCTRL(*this, "IDC_LIST_VAROUT", wxListBox);
	//Option to not use include constant removed.
    m_CheckConstant = NULL; // XRCCTRL(*this, "IDC_CHECK_CONSTANT", wxCheckBox);
    m_CheckWeight = XRCCTRL(*this, "IDC_WEIGHT_CHECK", wxCheckBox);
	//Option to not click standardize removed.
    m_standardize = NULL; // XRCCTRL(*this, "ID_STANDARDIZE", wxCheckBox);
    m_choice = XRCCTRL(*this, "IDC_CURRENTUSED_W", wxChoice);
    m_radio1 = XRCCTRL(*this, "IDC_RADIO1", wxRadioButton);
    m_radio2 = XRCCTRL(*this, "IDC_RADIO2", wxRadioButton);
    m_radio3 = XRCCTRL(*this, "IDC_RADIO3", wxRadioButton);
	m_gauge = XRCCTRL(*this, "IDC_GAUGE", wxGauge);
	m_gauge->SetRange(200);
	m_gauge->SetValue(0);
    m_gauge_text = XRCCTRL(*this, "IDC_GAUGE_TEXT", wxStaticText);
}

void RegressionDlg::OnRunClick( wxCommandEvent& event )
{
	LOG_MSG("Entering RegressionDlg::OnRunClick");

	UpdateMessageBox("calculating...");
	
	// Getting X's name
	int nX = m_independentlist->GetCount();
	LOG(nX);
	
	m_Xnames = new wxString [nX + 3];

	int ix = 0, ixName = 0;
	unsigned int i = 0;
	//Option to not use "include constant" removed.
	bool m_constant_term = true; // m_CheckConstant->GetValue();
	bool m_WeightCheck = m_CheckWeight->GetValue();
	bool m_standardization = true; // m_standardize->GetValue();

	wxString m_Yname = m_dependent->GetValue();

	if (m_constant_term) {
		nX = nX + 1;
		ix = 1; ixName = 1;
		if (RegressModel == 2) { 
			ixName = 2;

			wxString W_name = "W_" + m_Yname;
			W_name = W_name.Left(12);
			m_Xnames[0] = W_name;
			m_Xnames[1] = "CONSTANT";
			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i + 2] = m_independentlist->GetString(i);
			}
		} else if (RegressModel == 3) {
			ixName = 2;

			m_Xnames[nX] = "LAMBDA";
			m_Xnames[0]  = "CONSTANT";
			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i + 1] = m_independentlist->GetString(i);
			}
		} else {
			m_Xnames[0]  = "CONSTANT";
			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i + 1] = m_independentlist->GetString(i);
			}
		}
	} else {
		if (RegressModel == 2) {
			ixName = 1;

			wxString W_name = "W_" + m_Yname;
			W_name = W_name.Left(12);
			m_Xnames[0] = W_name;
			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i + 1] = m_independentlist->GetString(i);
			}
		} else if (RegressModel == 3) {
			ixName = 1;

			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i] = m_independentlist->GetString(i);
			}
			m_Xnames[nX] = "LAMBDA";
		} else {
			for (i = 0; i < m_independentlist->GetCount(); i++) {
				m_Xnames[i] = m_independentlist->GetString(i);
			}
		}
	}

	if (m_constant_term) {
		x = new double* [nX + 1]; // the last one is for Y
		alloc(x[0], m_obs, 1.0);
	} else {
		x = new double* [nX];
	}
	
	nVarName = nX + ixName - 1;

	int col = 0, row = 0;
 
	wxString s_list = wxEmptyString;
	
	// Y and X's data
	wxString st;
	m_independentlist->SetSelection(0);
	
	const unsigned int sz = m_independentlist->GetCount();
	LOG(sz);
	for(i = 0; i < sz; i++) {
		st = m_independentlist->GetString(i);
		s_list += st + ",";
	}
	m_Yname.Trim(false);
	m_Yname.Trim(true);
	s_list += m_Yname + ","; // add the dependent variable to the end of the
	// list
	// s_list includes everything in m_independentlist (sz) + m_Yname
	// m_Yname is dependent val
	
	double** dt = new double* [sz + 1];
	for (i = 0; i < sz + 1; i++) dt[i] = new double[m_obs];

	// WS1447
	// fill in each field from m_independentlist and tack on
	// m_Yname to the end
	// NOTE: We need to close this gapping memory leak!  It looks like
	// dt and x is allocated, but never freed!
	std::vector<double> vec(m_obs);
	for ( i=0; i < m_independentlist->GetCount(); i++) {
		int col = name_to_id[m_independentlist->GetString(i)]; 
		int tm = name_to_tm_id[m_independentlist->GetString(i)];
		grid_base->col_data[col]->GetVec(vec, tm);
		for (int j=0; j<m_obs; j++) dt[i][j] = vec[j];
	}
	grid_base->col_data[name_to_id[m_Yname]]->GetVec(vec,
													 name_to_tm_id[m_Yname]);
	for (int j=0; j<m_obs; j++) dt[sz][j] = vec[j];
		
	for (i = 0; i < sz + 1; i++) {
		x[i + ix] = dt[i];
	}
	
	if (m_constant_term) {
		y = x[sz + 1];
	} else {
		y = x[sz];  
	}
	
	double* result = NULL;
	
	const int n = m_obs;

	char buf_dtsetname[512];
	strcpy( buf_dtsetname,
		   (const char*)grid_base->GetDbfNameNoExt().mb_str(wxConvUTF8) );
	char* dtsetname = buf_dtsetname;

	if (m_WeightCheck) {
		const int op = m_choice->GetSelection();
		GeoDaWeight* w = w_manager->GetWeight(op);
		const GalElement* gal_weight = NULL;
		if (w_manager->IsGalWeight(op)) {
			gal_weight = w_manager->GetGalWeight(op)->gal;
		} else {
			wxMessageBox("Error: Only weights files in GAL format, or weights "
						 "files internernally converted to GAL format "
						 "supported.");
		}

		wxString fname = w_manager->GetWFilename(op);
		fname = GenUtils::GetFileName(fname);
		char buf_weightName[512];
		strcpy( buf_weightName, (const char*)fname.mb_str(wxConvUTF8) );
		char* weightName = buf_weightName;

		if (RegressModel == 1) {
			DiagnosticReport m_DR(n, nX, m_constant_term, true, RegressModel);
			m_DR.SetWeightName(weightName);
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight &&
				!classicalRegression(gal_weight, m_obs, y, n, x, nX, &m_DR,
									 m_constant_term, m_moranz, m_gauge)) {
				wxMessageBox("Error: the inverse matrix is ill-conditioned");
				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
				m_resid1= m_DR.GetResidual();
				printAndShowClassicalResults(dtsetname, weightName, &m_DR,
											 n, nX);
				m_yhat1 = m_DR.GetYHAT();
				m_OpenDump = true;
				m_Run = true;
				b_done1 = false;
			}

			m_DR.release_Var();
			gal_weight = NULL;

		} else if (RegressModel == 2) {
			// Check for Symmetry first
			if (!w->symmetry_checked) {
				ProgressDlg* p_dlg = new ProgressDlg(this, wxID_ANY,
													 "Weights Symmetry Check");
				p_dlg->Show();
				p_dlg->StatusUpdate(0, "Checking Symmetry...");
				WeightsManager::CheckWeightSymmetry(w, p_dlg);
				p_dlg->StatusUpdate(1, "Finished");
				p_dlg->Destroy();
			}
			if (!w->is_symmetric) {
				wxMessageBox("Only symmetric weights are supported for "
							 "this operation, please choose a symmetric "
							 "weights file. You can still choose Classic "
							 "regression for non-symmetric weights.");
				UpdateMessageBox("");
				return;
			}
			
			DiagnosticReport m_DR(n, nX + 1, m_constant_term, true,
								  RegressModel);
			
			m_DR.SetWeightName(weightName);
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight && !spatialLagRegression(gal_weight, m_obs,
													y, n, x, nX, &m_DR, true,
													m_gauge)) {
				wxMessageBox("Error: the inverse matrix is ill-conditioned.");
				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
				printAndShowLagResults(dtsetname, weightName, &m_DR, n, nX);
				m_yhat2 = m_DR.GetYHAT();
				m_resid2= m_DR.GetResidual();
				m_prederr2 = m_DR.GetPredError();
				m_Run = true;
				m_OpenDump = true;
				b_done2 = false;
			}
			
			m_DR.release_Var();
			gal_weight = NULL;
		} else if (RegressModel == 3) {
			// Check for Symmetry first
			if (!w->symmetry_checked) {
				ProgressDlg* p_dlg = new ProgressDlg(this, wxID_ANY,
													 "Weights Symmetry Check");
				p_dlg->Show();
				p_dlg->StatusUpdate(0, "Checking Symmetry...");
				WeightsManager::CheckWeightSymmetry(w, p_dlg);
				p_dlg->StatusUpdate(1, "Finished");
				p_dlg->Destroy();
			}
			if (!w->is_symmetric) {
				wxMessageBox("Only symmetric weights are supported for "
							 "this operation, please choose a symmetric "
							 "weights file. You can still choose Classic "
							 "regression for non-symmetric weights.");
				UpdateMessageBox("");
				return;
			}			
			
			// Error Model
			DiagnosticReport m_DR(n, nX + 1, m_constant_term, true,
								  RegressModel);
			m_DR.SetWeightName(weightName);
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight && !spatialErrorRegression(gal_weight, m_obs,
													  y, n, x, nX,
													  &m_DR, true, m_gauge)) {
				wxMessageBox("Error: the inverse matrix is ill-conditioned.");
				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
	  			printAndShowErrorResults(dtsetname, weightName, &m_DR, n, nX);
				m_yhat3 = m_DR.GetYHAT();
				m_resid3= m_DR.GetResidual();
				m_prederr3 = m_DR.GetPredError();
				m_Run = true;
				m_OpenDump = true;
				b_done3 = false;
			}

			m_DR.release_Var();
			gal_weight = NULL;

		} else {
			wxMessageBox("wrong model number");
			UpdateMessageBox("");
			return;
		}
	} else {
		DiagnosticReport m_DR(n, nX, m_constant_term, false, RegressModel);
		SetXVariableNames(&m_DR);
		m_DR.SetMeanY(ComputeMean(y, n));
		m_DR.SetSDevY(ComputeSdev(y, n));

		if (!classicalRegression((GalElement*)NULL, m_obs, y, n, x, nX, &m_DR, 
								 m_constant_term, false, m_gauge)) {
			wxMessageBox("Error: the inverse matrix is ill-conditioned.");
			m_OpenDump = false;
			OnCResetClick(event);
			UpdateMessageBox("");
			return;
		} else {
			char empty_str[] = "";
			printAndShowClassicalResults(dtsetname, empty_str, &m_DR, n, nX);
			m_yhat1 = m_DR.GetYHAT();
			m_resid1= m_DR.GetResidual();
			m_OpenDump = true;
			m_Run = true;
			b_done1 = false;
		}
		m_DR.release_Var();
	}

	EnablingItems();
	FindWindow(XRCID("ID_RUN"))->Enable(false);	
	UpdateMessageBox("done");
	
	LOG_MSG("Exiting RegressionDlg::OnRunClick");
}

void RegressionDlg::OnViewResultsClick( wxCommandEvent& event )
{
 	if (m_OpenDump) {
 		printTail();
 		(*logFile).close();
		MyFrame::theFrame->DisplayRegression(logReport);
	}
}

void RegressionDlg::OnCListVarinDoubleClicked( wxCommandEvent& event )
{
	if (lastSelection == 1) {
		OnCButton1Click(event);
	} else {
		OnCButton2Click(event);
	}
}

void RegressionDlg::OnCListVaroutDoubleClicked( wxCommandEvent& event )
{
	OnCButton3Click(event);
}

void RegressionDlg::OnCButton1Click( wxCommandEvent& event )
{
	if (m_varlist->GetCount() > 0) {
		if (m_varlist->GetSelection() >= 0) {
			wxString temp = m_dependent->GetValue();
			m_dependent->SetValue(
							m_varlist->GetString(m_varlist->GetSelection()));
			m_varlist->Delete(m_varlist->GetSelection());
			if (temp != wxEmptyString) m_varlist->Append(temp);
			b_done1 = b_done2 = b_done3 = false;
			m_nCount = 0;
			m_varlist->SetSelection(0);
		}
	}
	lastSelection = 2;
	EnablingItems();
}

void RegressionDlg::OnCButton2Click( wxCommandEvent& event )
{
	if (m_varlist->GetCount() > 0) {
		if (m_varlist->GetSelection() >= 0) {
			int cur_sel = m_varlist->GetSelection();
			m_independentlist->Append(m_varlist->GetString(cur_sel));
			m_varlist->Delete(cur_sel);
			if (cur_sel == m_varlist->GetCount()) {
				cur_sel = m_varlist->GetCount()-1;
			}
			m_varlist->SetSelection(cur_sel);
			b_done1 = b_done2 = b_done3 = false;
			m_nCount = 0;
		}
	}
	EnablingItems();
	lastSelection = 2;
}

void RegressionDlg::OnCResetClick( wxCommandEvent& event )
{
	logReport = wxEmptyString;
	lastSelection = 1;
	nVarName = 0;
	m_Run = false;
	m_OpenDump = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	RegressModel = 1;

	m_dependent->SetValue(wxEmptyString);
	InitVariableList();
}

void RegressionDlg::OnCButton3Click( wxCommandEvent& event )
{
	if (m_independentlist->GetCount() > 0) {
		if(m_independentlist->GetSelection() >= 0) {
			int cur_sel = m_independentlist->GetSelection();
			m_varlist->Append(m_independentlist->GetString(cur_sel));
			m_independentlist->Delete(cur_sel);
			if (cur_sel == m_independentlist->GetCount()) {
				cur_sel = m_independentlist->GetCount()-1;
			}
			m_independentlist->SetSelection(cur_sel);
			m_nCount = 0;
			b_done1 = b_done2 = b_done3 = false;
		}
	}
	if(m_independentlist->GetCount() <= 0) m_Run = false;
	EnablingItems();	
}

void RegressionDlg::OnCButton4Click( wxCommandEvent& event )
{
	for (unsigned int i=0; i<m_varlist->GetCount(); i++) {
		m_independentlist->Append(m_varlist->GetString(i));
	}
	m_varlist->Clear();	
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	EnablingItems();		
	lastSelection = 2;
}

void RegressionDlg::OnCButton5Click( wxCommandEvent& event )
{
	for (unsigned int i=0; i<m_independentlist->GetCount(); i++) {
		m_varlist->Append(m_independentlist->GetString(i));
	}
	m_independentlist->Clear();	
	m_Run = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	EnablingItems();	
	lastSelection = 2;
}

void RegressionDlg::OnCSaveRegressionClick( wxCommandEvent& event )
{
	LOG_MSG("Entering RegressionDlg::OnCSaveRegressionClick");
	if (!grid_base) return;
	int n_obs = grid_base->GetNumberRows();
	
	std::vector<double> yhat(grid_base->GetNumberRows());
	std::vector<double> resid(grid_base->GetNumberRows());
	std::vector<double> prederr(RegressModel > 1 ? n_obs : 0);
	std::vector<SaveToTableEntry> data(RegressModel > 1 ? 3 : 2);
		
	wxString pre = "";
	if (RegressModel==1) {
		pre = "OLS_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat1[i];
			resid[i] = m_resid1[i];
		}
	} else if (RegressModel==2) {
		pre = "LAG_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat2[i];
			resid[i] = m_resid2[i];
			prederr[i] = m_prederr2[i];
		}
	} else { // RegressModel==3
		pre = "ERR_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat3[i];
			resid[i] = m_resid3[i];
			prederr[i] = m_prederr3[i];
		}
	}	
	
	data[0].d_val = &yhat;
	data[0].label = "Predicted Value";
	data[0].field_default = pre + "PREDIC";
	data[0].type = GeoDaConst::double_type;
	
	data[1].d_val = &resid;
	data[1].label = "Residual";
	data[1].field_default = pre + "RESIDU";
	data[1].type = GeoDaConst::double_type;
		
	if (RegressModel > 1) {
		data[2].d_val = &prederr;
		data[2].label = "Prediction Error";
		data[2].field_default = pre + "PRDERR";
		data[2].type = GeoDaConst::double_type;
	}
	
	SaveToTableDlg dlg(grid_base, this, data,
					   "Save Regression Results",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();	
	
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	LOG_MSG("Exiting RegressionDlg::OnCSaveRegressionClick");
}

void RegressionDlg::OnCloseClick( wxCommandEvent& event )
{
	LOG_MSG("Entering RegressionDlg::OnCloseClick");
	(*logFile).close();
	event.Skip();
	EndDialog(wxID_CLOSE);
	Destroy();
	project->regression_dlg = 0;
	LOG_MSG("Entering RegressionDlg::OnCloseClick");
}

void RegressionDlg::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering RegressionDlg::OnClose");
	(*logFile).close();
	Destroy();
	project->regression_dlg = 0;
	LOG_MSG("Exiting RegressionDlg::OnClose");
}

void RegressionDlg::OnCOpenWeightClick( wxCommandEvent& event )
{
	SelectWeightDlg dlg(project, this);
	if (dlg.ShowModal()!= wxID_OK) return;
	m_choice->Clear();
	for (int i=0; i<w_manager->GetNumWeights(); i++) {
		m_choice->Append(w_manager->GetWFilename(i));
	}
	if (w_manager->GetCurrWeightInd() >=0 ) {
		m_choice->SetSelection(w_manager->GetCurrWeightInd());
	}
	bool m_Run1 = m_independentlist->GetCount() > 0;
	bool enable_run = (m_Run1 &&
					   (!m_CheckWeight->GetValue() ||
						(m_CheckWeight->GetValue() &&
						 (m_choice->GetSelection() !=wxNOT_FOUND))));
	FindWindow(XRCID("ID_RUN"))->Enable(enable_run);	
}

void RegressionDlg::InitVariableList()
{
	UpdateMessageBox(wxEmptyString);

	m_varlist->Clear();
	m_independentlist->Clear();
	m_Xnames = NULL;
	m_radio1->SetValue(true);
	//Option to not use include constant removed.
	//m_CheckConstant->SetValue(true);
	//m_standardize->SetValue(true);

	m_resid1 = NULL;
	m_yhat1 = NULL;
	m_resid2 = NULL;
	m_yhat2 = NULL;
	m_prederr2 = NULL;
	m_resid3 = NULL;
	m_yhat3 = NULL;
	m_prederr3 = NULL;
 
	m_obs = project->GetNumRecords();
	if (m_obs <= 0) {
		wxMessageBox("Error: no records found in DBF file");
		return;
	}
	
	lists = NULL;
	listb = NULL;

	grid_base->FillNumericColIdMap(col_id_map);
	name_to_id.clear(); // map to grid_base col id
	name_to_tm_id.clear(); // map to corresponding time id
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int id = col_id_map[i];
		wxString name = grid_base->col_data[id]->name.Upper();
		if (grid_base->IsColTimeVariant(id)) {
			for (int t=0; t<grid_base->col_data[id]->time_steps; t++) {
				wxString nm = name;
				nm << " (" << grid_base->time_ids[t] << ")";
				name_to_id[nm] = id;
				name_to_tm_id[nm] = t;
				m_varlist->Append(nm);
			}
		} else {
			name_to_id[name] = id;
			name_to_tm_id[name] = 0;
			m_varlist->Append(name);
		}
	}

	y = NULL;
	x = NULL;
	m_varlist->SetSelection(0);
	EnablingItems();
}

void RegressionDlg::EnablingItems()
{
    bool m_Run1 = m_independentlist->GetCount() > 0;
	bool enable_run = (m_Run1 &&
					   (!m_CheckWeight->GetValue() ||
						(m_CheckWeight->GetValue() &&
						 (m_choice->GetSelection() !=wxNOT_FOUND))));
	FindWindow(XRCID("ID_RUN"))->Enable(enable_run);	

    wxString m_Yname = m_dependent->GetValue();

	FindWindow(XRCID("IDC_BUTTON2"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(m_Yname!=wxEmptyString);

	FindWindow(XRCID("IDC_LIST_VAROUT"))->Enable(m_Yname != wxEmptyString);	
	
	bool m_WeightCheck = m_CheckWeight->GetValue();

	FindWindow(XRCID("IDC_WEIGHT_CHECK"))->Enable(m_Run1);
	FindWindow(XRCID("IDC_CURRENTUSED_W"))->Enable(m_Run1 && m_WeightCheck);
	FindWindow(XRCID("ID_OPEN_WEIGHT"))->Enable(m_Run1 && m_WeightCheck);
	//Option to not use include constant removed.
	//FindWindow(XRCID("IDC_CHECK_CONSTANT"))->Enable(m_Run1);
	//FindWindow(XRCID("ID_STANDARDIZE"))->Enable(m_Run1);
	FindWindow(XRCID("ID_VIEW_RESULTS"))->Enable(!logReport.IsEmpty());	
	FindWindow(XRCID("IDC_SAVE_REGRESSION"))->Enable(m_Run);	
	FindWindow(XRCID("IDC_RADIO1"))->Enable(m_Run1);	
	FindWindow(XRCID("IDC_RADIO2"))->Enable(m_WeightCheck && m_Run1);	
	FindWindow(XRCID("IDC_RADIO3"))->Enable(m_WeightCheck && m_Run1);	

}

void RegressionDlg::OnCWeightCheckClick( wxCommandEvent& event )
{
	b_done1 = b_done2 = b_done3 = false;
	EnablingItems();

	if(!m_CheckWeight->IsChecked()) m_radio1->SetValue(true);
}

void RegressionDlg::UpdateMessageBox(wxString msg)
{
	m_gauge_text->SetLabelText(msg);
	m_gauge_text->Update();
}

void RegressionDlg::SetXVariableNames(DiagnosticReport *dr)
{
	for (int i = 0; i < nVarName; i++) {
		// MMM: this is a dangerous function with
		// no simple fix.  It will be safer to create
		// a small memory leak for now.
		char* name = new char[m_Xnames[i].size()+1];
		strcpy(name, (const char*)m_Xnames[i].mb_str(wxConvUTF8));
		dr->SetXVarNames(i, name);
	}
}

void RegressionDlg::printTail()
{
	char end1[40], end2[20];
	sprintf(end1, "\\par \\b\\f1\\fs20");
	sprintf(end2, "\\par }");

	(*logFile) << end1 << '\n' << end2;
}

void RegressionDlg::printHeader()
{
	char h1[255], h2[255], h3[255];
	sprintf(h1, "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fnil\\fprq12\\fcharset0 Courier New;}");
	sprintf(h2, "{\\f1\\fswiss\\fprq2\\fcharset0 System;}}\n");
	sprintf(h3, "\\viewkind4\\uc1\\pard\\lang1033\\f0\\fs24 \n");

	(*logFile) << h1 << h2 << h3;
}

void RegressionDlg::printAndShowClassicalResults(const char *datasetname,
												  const char* wname,
												  DiagnosticReport *r,
												  int Obs, int nX)
{
	int i = 0;
	wxString m_Yname = m_dependent->GetValue();
	wxString newLog;

	char par[10], buf[255];
	sprintf(par,"\\par ");

	char buf_m_title[512];
	strcpy( buf_m_title, (const char*)m_title.mb_str(wxConvUTF8) );
	sprintf(buf,"%s", buf_m_title);
	(*logFile) << par << "\\b \\ul " << buf << "\\ulnone \\b0 \n";
	sprintf(buf, "%s\n", buf_m_title);
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\\b \\ul SUMMARY OF OUTPUT: ORDINARY LEAST SQUARES ESTIMATION \\ulnone \\b0 \n";
	newLog += "SUMMARY OF OUTPUT: ORDINARY LEAST SQUARES ESTIMATION\n";
	(*logFile) << par << "Data set            : \\b " << datasetname << " \\b0 \n";
	sprintf(buf, "Data set            :  %s  \n", datasetname);
	newLog += wxString::Format("%s",buf);
	char buf_ynm[512];
	strcpy( buf_ynm, (const char*)m_Yname.mb_str(wxConvUTF8) );
	char* ynm = buf_ynm;
	sprintf(buf,"Dependent Variable  :\\b %12s \\b0  Number of Observations:%5d\n",ynm,Obs);
	(*logFile) << par << buf;
	sprintf(buf,"Dependent Variable  :%12s  Number of Observations:%5d\n",ynm,Obs);
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Mean dependent var  :%12.6g  Number of Variables   :%5d\n",r->GetMeanY(),nX);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.D. dependent var  :%12.6g  Degrees of Freedom    :%5d \n \n \n",r->GetSDevY(), Obs-nX);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "\n \n";
	newLog += "\n \n";

	sprintf(buf,"R-squared           :%12.6f  F-statistic           :%12.6g\n",r->GetR2(),r->GetFtest());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Adjusted R-squared  :%12.6f  Prob(F-statistic)     :%12.6g\n",r->GetR2_adjust(),r->GetFtestProb());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Sum squared residual:%12.6g  Log likelihood        :%12.6g\n",r->GetRSS(),r->GetLIK());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Sigma-square        :%12.6g  Akaike info criterion :%12.6g\n",r->GetSIQ_SQ(),r->GetAIC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.E. of regression  :%12.6g  Schwarz criterion     :%12.6g\n",sqrt(r->GetSIQ_SQ()),r->GetOLS_SC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Sigma-square ML     :%12.6g\n",r->GetSIQ_SQLM());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.E of regression ML:%12.6g\n\n \n \n",sqrt(r->GetSIQ_SQLM()));
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\\b     Variable    Coefficient     Std.Error    t-Statistic   Probability \\b0 "  << '\n';
	newLog += "     Variable    Coefficient     Std.Error    t-Statistic   Probability  \n";
	(*logFile) << par << "-----------------------------------------------------------------------"  << '\n';
	newLog += "-----------------------------------------------------------------------\n";

	for (i=0;i<nX;i++)
	{
		char * xnm= r->GetXVarName(i);
		sprintf(buf,"%12s  %12.7g   %12.7g   %12.7g   %10.7f\n",xnm,
			r->GetCoefficient(i), r->GetStdError(i), r->GetZValue(i), r->GetProbability(i));
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
	}
	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\n \n";
	newLog += "\n \n";
	(*logFile) << par << '\n';
	newLog += '\n';
	(*logFile) << par << "\\bREGRESSION DIAGNOSTICS \\b0 \n" ;
	newLog += "REGRESSION DIAGNOSTICS  \n";
	double *rr = r->GetBPtest();
	if (rr[1] > 1) {
	sprintf(buf, "MULTICOLLINEARITY CONDITION NUMBER   %7f\n", r->GetConditionNumber());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	} else
	{
	sprintf(buf, "MULTICOLLINEARITY CONDITION NUMBER   %7f\n", r->GetConditionNumber());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "                                      (Extreme Multicollinearity)";
	newLog += "                                      (Extreme Multicollinearity)";
	}
	(*logFile) << par << "TEST ON NORMALITY OF ERRORS\n";
	newLog += "TEST ON NORMALITY OF ERRORS\n";
	(*logFile) << par << "TEST                  DF          VALUE            PROB\n";
	newLog += "TEST                  DF          VALUE            PROB\n";
	rr = r->GetJBtest();
	 sprintf(buf,"Jarque-Bera           %2.0f        %11.7g        %9.7f\n",rr[0],rr[1],rr[2]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	
	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";
	(*logFile) << par << "\\b DIAGNOSTICS FOR HETEROSKEDASTICITY \\b0 \n";
	newLog += "DIAGNOSTICS FOR HETEROSKEDASTICITY  \n";
	(*logFile) << par << "RANDOM COEFFICIENTS\n";
	newLog += "RANDOM COEFFICIENTS\n";
	(*logFile) << par << "TEST                  DF          VALUE            PROB\n";
	newLog += "TEST                  DF          VALUE            PROB\n";
	rr = r->GetBPtest();
	if (rr[1] > 0)
		sprintf(buf,"Breusch-Pagan test    %2.0f        %11.7g        %9.7f\n",rr[0],rr[1],rr[2]);
	else
	{
		//wxMessageBox("Extreme multicollinearity!");
		sprintf(buf,"Breusch-Pagan test    %2.0f        %11.7g        N/A\n",rr[0],rr[1]);
	}

	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	rr = r->GetKBtest();
	if (rr[1]>0)
		sprintf(buf,"Koenker-Bassett test  %2.0f        %11.7g        %9.7f\n",rr[0],rr[1],rr[2]);
	else
		sprintf(buf,"Koenker-Bassett test  %2.0f        %11.7g        N/A\n",rr[0],rr[1]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "SPECIFICATION ROBUST TEST\n";
	newLog += "SPECIFICATION ROBUST TEST\n";
	rr = r->GetWhitetest();
	(*logFile) << par << "TEST                  DF          VALUE            PROB\n";
	newLog += "TEST                  DF          VALUE            PROB\n";
	if (rr[2]< 0.0)
		sprintf(buf,"White                 %2.0f            N/A            N/A\n",rr[0],rr[1],rr[2]);
	else
		sprintf(buf,"White                 %2.0f        %11.7g        %9.7f\n",rr[0],rr[1],rr[2]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	bool m_WeightCheck = m_CheckWeight->GetValue();
	if (m_WeightCheck)
	{
		(*logFile) << par << "\n \n \n";
		newLog += "\n \n \n";
		(*logFile) << par << "\\b \\ul DIAGNOSTICS FOR SPATIAL DEPENDENCE \\ulnone \\b0 \n";
		newLog += "DIAGNOSTICS FOR SPATIAL DEPENDENCE   \n";
		sprintf(buf, "FOR WEIGHT MATRIX : %s\n   (row-standardized weights)\n", wname);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetMoranI();
		(*logFile) << par << "\\b TEST                          MI/DF      VALUE          PROB \\b0 \n";
		newLog += "TEST                          MI/DF      VALUE          PROB  \n";
		if (m_moranz)
		   sprintf(buf,"Moran's I (error)           %8.6f   %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		else
		   sprintf(buf,"Moran's I (error)           %8.6f     N/A            N/A\n",rr[0]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetLMLAG();
		sprintf(buf,"Lagrange Multiplier (lag)      %2.0f      %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetLMLAGRob();
		sprintf(buf,"Robust LM (lag)                %2.0f      %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetLMERR();
		sprintf(buf,"Lagrange Multiplier (error)    %2.0f      %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetLMERRRob();
		sprintf(buf,"Robust LM (error)              %2.0f      %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		rr = r->GetLMSarma();
		sprintf(buf,"Lagrange Multiplier (SARMA)    %2.0f      %11.7f      %9.7f\n",rr[0],rr[1],rr[2]);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
    //		(*logFile) << par << "Kelejian-Robinson (error)";
    //		newLog += "Kelejian-Robinson (error)";
	}
	if (m_output2)
	{
		(*logFile) << par << "\n \n \n";
		newLog += "\n \n \n";
		(*logFile) << par << "\\b \\ul COEFFICIENTS VARIANCE MATRIX \\ulnone \\b0 \n";
		newLog += "  COEFFICIENTS VARIANCE MATRIX   \n";

		
		DoublePtr *cov = r->GetCovariance();
		char *xnm;
		int j = 0, start = 0;

		while (start < nX)
		{
			wxString st = wxEmptyString;
			for (j=start;j<nX && j<start+6;j++)
			{
				xnm= r->GetXVarName(j);
				sprintf(buf," %10s ",xnm);
				st += wxString::Format("%s",buf);
			}
			char buf_xnm[512];
			strcpy( buf_xnm, (const char*)st.mb_str(wxConvUTF8) );
			char* xnm = buf_xnm;

			sprintf(buf,"%s\n\n",xnm);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);

			for (i=0;i<nX;i++)
			{
				st = wxEmptyString;
				for (j=start;j<nX && j<start+6;j++)
				{
					sprintf(buf," %10.6f ",cov[i][j]);
					st += wxString::Format("%s",buf);
				}
				char buf_xnm[512];
				strcpy( buf_xnm, (const char*)st.mb_str(wxConvUTF8) );
				char* xnm = buf_xnm;

				sprintf(buf,"%s\n\n",xnm);
				(*logFile) << par << buf;
				newLog += wxString::Format("%s",buf);
			}
			(*logFile) << par << "\n \n";
			newLog += "\n \n";

			start += 6;
		}

	}
	if (m_output1)
	{
		(*logFile) << par << "\n \n";
		newLog += "\n \n";
		sprintf(buf,"  OBS    %12s        PREDICTED        RESIDUAL     \n",ynm);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		double *res  = r->GetResidual();
		double *yh = r->GetYHAT();
		for (i=0;i<m_obs;i++)
		{
			sprintf(buf,"%5d     %12.5f    %12.5f    %12.5f\n",i+1,y[i],yh[i],res[i]);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);
		}
		res = NULL;
		yh = NULL;
	}

	(*logFile) << par << "========================= END OF REPORT ==============================" << '\n';
	newLog += "========================= END OF REPORT ==============================\n";

	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";
	logReport = logReport + newLog;
	newLog= " ";
}

void RegressionDlg::printAndShowLagResults(char* datasetname, char* wname,
											DiagnosticReport *r,
											int Obs, int nX)
{
	wxString m_Yname = m_dependent->GetValue();
	wxString newLog;

	int i = 0;
	char par[10], buf[255];
	sprintf(par,"\\par ");

	char buf_m_title[512];
	strcpy( buf_m_title, (const char*)m_title.mb_str(wxConvUTF8) );
	
	sprintf(buf,"%s", buf_m_title);
	(*logFile) << par << "\\b \\ul " << buf << "\\ulnone \\b0 \n";
	sprintf(buf, "%s\n", buf_m_title);
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\\b \\ul SUMMARY OF OUTPUT: SPATIAL LAG MODEL - MAXIMUM LIKELIHOOD ESTIMATION \\ulnone \\b0 \n";
	newLog += "SUMMARY OF OUTPUT: SPATIAL LAG MODEL - MAXIMUM LIKELIHOOD ESTIMATION \n";
	(*logFile) << par << "Data set            : \\b " << datasetname << " \\b0 \n";
	sprintf(buf, "Data set            : %s \n", datasetname);
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "Spatial Weight      : \\b " << wname << " \\b0 \n";
	sprintf(buf, "Spatial Weight      : %s \n", wname);
	newLog += wxString::Format("%s",buf);
	
	char buf_ynm[512];
	strcpy( buf_ynm, (const char*)m_Yname.mb_str(wxConvUTF8) );
	char* ynm = buf_ynm;

	sprintf(buf,"Dependent Variable  :\\b %12s \\b0  Number of Observations:%5d\n",ynm,Obs);
	(*logFile) << par << buf;
	sprintf(buf, "Dependent Variable  :%12s  Number of Observations:%5d\n", ynm, Obs);
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Mean dependent var  :%12.6g  Number of Variables   :%5d\n",r->GetMeanY(),nX+1);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.D. dependent var  :%12.6g  Degrees of Freedom    :%5d\n",r->GetSDevY(), Obs-nX-1);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Lag coeff.   (Rho)  :%12.6g \n \n \n", r->GetCoefficient(0));
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\n \n";
	newLog += "\n \n";

	sprintf(buf,"R-squared           :%12.6f  Log likelihood        :%12.6g\n",r->GetR2(),r->GetLIK());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
//	sprintf(buf,"Sq. Correlation     :%12.6f  Akaike info criterion :%12.6g\n",r->GetR2_adjust(),r->GetAIC());
	sprintf(buf,"Sq. Correlation     : -            Akaike info criterion :%12.6g\n",r->GetAIC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Sigma-square        :%12.6g  Schwarz criterion     :%12.6g\n",r->GetSIQ_SQ(),r->GetOLS_SC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.E of regression   :%12.6g",sqrt(r->GetSIQ_SQ()));
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";

	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\\b     Variable    Coefficient     Std.Error    z-value      Probability \\b0 "  << '\n';
	newLog += "    Variable    Coefficient     Std.Error    z-value      Probability \n";
	(*logFile) << par << "-----------------------------------------------------------------------"  << '\n';
	newLog += "-----------------------------------------------------------------------\n";

	for (i=0;i<nX+1;i++)
	{
		char * xnm= r->GetXVarName(i);
		sprintf(buf,"%12s  %12.7g   %12.7g   %12.7g   %10.7f\n",xnm,
		r->GetCoefficient(i), r->GetStdError(i), r->GetZValue(i), r->GetProbability(i));
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
	}
	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\n \n";
	newLog += "\n \n";


	(*logFile) << par << "\\b REGRESSION DIAGNOSTICS \\b0 \n" ;
	newLog += "REGRESSION DIAGNOSTICS\n";
	(*logFile) << par << "\\b DIAGNOSTICS FOR HETEROSKEDASTICITY \\b0 \n";
	newLog += "DIAGNOSTICS FOR HETEROSKEDASTICITY \n";
	(*logFile) << par << "RANDOM COEFFICIENTS\n";
	newLog += "RANDOM COEFFICIENTS\n";
	(*logFile) << par << "\\b TEST                                     DF     VALUE         PROB \\b0 \n";
	newLog += "TEST                                     DF     VALUE         PROB \n";
	double *rr = r->GetBPtest();
	if (rr[1] >0)
	sprintf(buf,"Breusch-Pagan test                      %2.0f    %11.7g     %9.7f\n",rr[0],rr[1],rr[2]);
	else
	sprintf(buf,"Breusch-Pagan test                      %2.0f    N/A        N/A  \n",rr[0]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";
	(*logFile) << par << "\\b \\ul DIAGNOSTICS FOR SPATIAL DEPENDENCE \\ulnone \\b0 \n";
	newLog += "DIAGNOSTICS FOR SPATIAL DEPENDENCE \n";
	(*logFile) << par << "SPATIAL LAG DEPENDENCE FOR WEIGHT MATRIX : \\b " << wname << " \\b0 \n";
	sprintf(buf, "SPATIAL LAG DEPENDENCE FOR WEIGHT MATRIX : %s \n", wname);
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "\\b TEST                                     DF      VALUE        PROB \\b0 \n";
	newLog += "TEST                                     DF      VALUE        PROB \n";
	rr = r->GetLRTest();
	sprintf(buf,"Likelihood Ratio Test                   %2.0f    %11.7g     %9.7f\n",rr[0],rr[1],rr[2]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);


	if (m_output2)
	{

		(*logFile) << par << "\n \n \n";
		newLog += "\n \n \n";
		(*logFile) << par << "\\b \\ul COEFFICIENTS VARIANCE MATRIX \\ulnone \\b0 \n";
		newLog += "COEFFICIENTS VARIANCE MATRIX \n";

		
		DoublePtr *cov = r->GetCovariance();
		char **xnm = new char* [nX+1];
		int j = 0, start = 0;

		for (i=1;i<nX+1;i++)
			xnm[i-1] = r->GetXVarName(i);
		xnm[nX] = r->GetXVarName(0);
		
		while (start < nX+1)
		{
			wxString st = wxEmptyString;
			for (j=start;j<nX+1 && j<start+6;j++)
			{
				sprintf(buf," %10s ",xnm[j]);
				st += wxString::Format("%s",buf);
			}
			char buf_buf1[512];
			strcpy( buf_buf1, (const char*)st.mb_str(wxConvUTF8) );
			char* buf1 = buf_buf1;
			sprintf(buf,"%s\n\n",buf1);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);

			for (i=0;i<nX+1;i++)
			{
				st = wxEmptyString;
				for (j=start;j<nX+1 && j<start+6;j++)
				{
					sprintf(buf," %10.6f ",cov[i][j]);
					st += wxString::Format("%s",buf);
				}
				char buf_buf1[512];
				strcpy( buf_buf1, (const char*)st.mb_str(wxConvUTF8) );
				char* buf1 = buf_buf1;
				sprintf(buf,"%s\n\n",buf1);
				(*logFile) << par << buf;
				newLog += wxString::Format("%s",buf);
			}

			(*logFile) << par << "\n \n";
			newLog += "\n \n";

			start += 6;
		}

	}
	if (m_output1)
	{
		(*logFile) << par << "\n \n";
		newLog += "\n \n";
		sprintf(buf,"  OBS    %12s        PREDICTED        RESIDUAL        PRED ERROR\n",ynm);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		double *res  = r->GetResidual();
		double *yh = r->GetYHAT();
		double *pe = r->GetPredError();
		for (i=0;i<m_obs;i++)
		{
			sprintf(buf,"%5d     %12.5g    %12.5f    %12.5f    %12.5f\n",i+1,y[i],yh[i],res[i],pe[i]);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);
		}
		res = NULL;
		yh = NULL;
	}

	(*logFile) << par << "========================= END OF REPORT ==============================" << '\n';
	newLog += "========================= END OF REPORT ==============================\n";

	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";

	logReport = logReport + newLog;
	newLog= " ";

}

void RegressionDlg::printAndShowErrorResults(char *datasetname,
											  char *wname,
											  DiagnosticReport *r,
											  int Obs, int nX)
{
	wxString m_Yname = m_dependent->GetValue();
	wxString newLog;

	int i = 0;
	char par[10], buf[255];
	sprintf(par,"\\par ");

	char buf_m_title[512];
	strcpy( buf_m_title, (const char*)m_title.mb_str(wxConvUTF8) );
	sprintf(buf,"%s", buf_m_title);
	(*logFile) << par << "\\b \\ul " << buf << "\\ulnone \\b0 \n";
	sprintf(buf, "%s\n", buf_m_title);
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\\b \\ul SUMMARY OF OUTPUT: SPATIAL ERROR MODEL - MAXIMUM LIKELIHOOD ESTIMATION \\ulnone \\b0 \n";
	newLog += "SUMMARY OF OUTPUT: SPATIAL ERROR MODEL - MAXIMUM LIKELIHOOD ESTIMATION \n";
	(*logFile) << par << "Data set            : \\b " << datasetname << " \\b0 \n";
	sprintf(buf, "Data set            : %s \n", datasetname);
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "Spatial Weight      : \\b " << wname << " \\b0 \n";
	sprintf(buf, "Spatial Weight      : %s \n", wname);
	newLog += wxString::Format("%s",buf);

	char buf_ynm[512];
	strcpy( buf_ynm, (const char*)m_Yname.mb_str(wxConvUTF8) );
	char* ynm = buf_ynm;

	sprintf(buf,"Dependent Variable  :\\b %12s \\b0  Number of Observations:%5d\n",ynm,Obs);
	(*logFile) << par << buf;
	sprintf(buf, "Dependent Variable  :%12s   Number of Observations:%5d\n", ynm, Obs);
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Mean dependent var  :%12.6f  Number of Variables   :%5d\n",r->GetMeanY(),nX);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.D. dependent var  :%12.6f  Degrees of Freedom    :%5d\n",r->GetSDevY(), Obs-nX);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Lag coeff. (Lambda) :%12.6f \n \n \n", r->GetCoefficient(nX));
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\n \n";
	newLog += "\n \n";
	sprintf(buf,"R-squared           :%12.6f  R-squared (BUSE)      : - \n",r->GetR2());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	sprintf(buf,"Sq. Correlation     : -            Log likelihood        :%12.6f\n",r->GetLIK());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"Sigma-square        :%12.6g  Akaike info criterion :%12.6g\n",r->GetSIQ_SQ(),r->GetAIC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	sprintf(buf,"S.E of regression   :%12.6g  Schwarz criterion     :%12.6g\n",sqrt(r->GetSIQ_SQ()),r->GetOLS_SC());
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";

	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\\b     Variable    Coefficient     Std.Error    z-value      Probability \\b0 "  << '\n';
	newLog += "    Variable    Coefficient     Std.Error    z-value      Probability \n";
	(*logFile) << par << "-----------------------------------------------------------------------"  << '\n';
	newLog += "-----------------------------------------------------------------------\n";

	for (i=0;i<nX+1;i++)
	{
		char * xnm= r->GetXVarName(i);
		sprintf(buf,"%12s   %10.7g     %10.7g     %10.7g   %10.7f\n",xnm,
		r->GetCoefficient(i), r->GetStdError(i), r->GetZValue(i), r->GetProbability(i));
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
	}
	(*logFile) << par << "-----------------------------------------------------------------------" << '\n';
	newLog += "-----------------------------------------------------------------------\n";
	(*logFile) << par << "\n \n";
	newLog += "\n \n";


	(*logFile) << par << "\\b REGRESSION DIAGNOSTICS \\b0 \n" ;
	newLog += "REGRESSION DIAGNOSTICS \n";
	(*logFile) << par << "\\b DIAGNOSTICS FOR HETEROSKEDASTICITY \\b0 \n";
	newLog += "DIAGNOSTICS FOR HETEROSKEDASTICITY \n";
	(*logFile) << par << "RANDOM COEFFICIENTS\n";
	newLog += "RANDOM COEFFICIENTS\n";
	(*logFile) << par << "\\b TEST                                     DF     VALUE         PROB \\b0 \n";
	newLog += "TEST                                     DF     VALUE         PROB \n";
	double *rr = r->GetBPtest();
	if (rr[1] > 0)
	sprintf(buf,"Breusch-Pagan test                      %2.0f    %11.7g     %9.7f\n",rr[0],rr[1],rr[2]);
	else
	sprintf(buf,"Breusch-Pagan test                      %2.0f    N/A        N/A  \n",rr[0]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";
	(*logFile) << par << "\\b \\ul DIAGNOSTICS FOR SPATIAL DEPENDENCE \\ulnone \\b0 \n";
	newLog += "DIAGNOSTICS FOR SPATIAL DEPENDENCE \n";
	(*logFile) << par << "SPATIAL ERROR DEPENDENCE FOR WEIGHT MATRIX : \\b " << wname << " \\b0 \n";
	sprintf(buf, "SPATIAL ERROR DEPENDENCE FOR WEIGHT MATRIX : %s \n", wname);
	newLog += wxString::Format("%s",buf);
	(*logFile) << par << "\\b TEST                                     DF     VALUE         PROB \\b0 \n";
	newLog += "TEST                                     DF      VALUE        PROB \n";
	rr = r->GetLRTest();
	sprintf(buf,"Likelihood Ratio Test                   %2.0f    %11.7g     %9.7f\n",rr[0],rr[1],rr[2]);
	(*logFile) << par << buf;
	newLog += wxString::Format("%s",buf);

	if (m_output2)
	{

		(*logFile) << par << "\n \n \n";
		newLog += "\n \n \n";
		(*logFile) << par << "\\b \\ul COEFFICIENTS VARIANCE MATRIX \\ulnone \\b0 \n";
		newLog += "COEFFICIENTS VARIANCE MATRIX \n";

		
		DoublePtr *cov = r->GetCovariance();
		char **xnm = new char* [nX+1];
		int j = 0, start = 0;

		for (i=0;i<nX+1;i++)
			xnm[i] = r->GetXVarName(i);
		
		while (start < nX+1)
		{
			wxString st = wxEmptyString;
			for (j=start;j<nX+1 && j<start+6;j++)
			{
				sprintf(buf," %10s ",xnm[j]);
				st += wxString::Format("%s",buf);
			}
			
			char buf_buf1[512];
			strcpy( buf_buf1, (const char*)st.mb_str(wxConvUTF8) );
			char* buf1 = buf_buf1;

			sprintf(buf,"%s\n\n",buf1);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);

			for (i=0;i<nX+1;i++)
			{
				st = wxEmptyString;
				for (j=start;j<nX+1 && j<start+6;j++)
				{
					sprintf(buf," %10.6f ",cov[i][j]);
					st += wxString::Format("%s",buf);
				}
				char buf_buf1[512];
				strcpy( buf_buf1, (const char*)st.mb_str(wxConvUTF8) );
				char* buf1 = buf_buf1;
				sprintf(buf,"%s\n\n",buf1);
				(*logFile) << par << buf;
				newLog += wxString::Format("%s",buf);
			}

			(*logFile) << par << "\n \n";
			newLog += "\n \n";

			start += 6;
		}

	}
	if (m_output1)
	{
		(*logFile) << par << "\n \n";
		newLog += "\n \n";
		sprintf(buf,"  OBS    %12s        PREDICTED        RESIDUAL        PRED ERROR\n",ynm);
		(*logFile) << par << buf;
		newLog += wxString::Format("%s",buf);
		double *res  = r->GetResidual();
		double *yh = r->GetYHAT();
		double *pe = r->GetPredError();
		for (i=0;i<m_obs;i++)
		{
			sprintf(buf,"%5d     %12.5f    %12.5f    %12.5f    %12.5f\n",i+1,y[i],yh[i],res[i],pe[i]);
			(*logFile) << par << buf;
			newLog += wxString::Format("%s",buf);
		}
		res = NULL;
		yh = NULL;
	}

	(*logFile) << par << "========================= END OF REPORT ==============================" << '\n';
	newLog += "========================= END OF REPORT ==============================\n";

	(*logFile) << par << "\n \n \n";
	newLog += "\n \n \n";
	logReport = logReport + newLog;
	newLog= " ";

}

void RegressionDlg::OnCRadio1Selected( wxCommandEvent& event )
{
	m_Run = false;
	RegressModel = 1;
	UpdateMessageBox(" ");
    EnablingItems();
	m_gauge->SetValue(0);
}

void RegressionDlg::OnCRadio2Selected( wxCommandEvent& event )
{
	m_Run = false;
	RegressModel = 2;
	UpdateMessageBox(" ");
    EnablingItems();
	m_gauge->SetValue(0);
}

void RegressionDlg::OnCRadio3Selected( wxCommandEvent& event )
{
	m_Run = false;
	RegressModel = 3;
	UpdateMessageBox(" ");
    EnablingItems();
	m_gauge->SetValue(0);
}

void RegressionDlg::OnStandardizeClick( wxCommandEvent& event )
{
	wxMessageBox("row standardization is by default");
	// m_standardize->SetValue(true);
}

void RegressionDlg::update(FramesManager* o)
{
}

