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
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "Dbf2GaussDlg.h"

BEGIN_EVENT_TABLE( Dbf2GaussDlg, wxDialog )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VARIN"),
					   Dbf2GaussDlg::OnCListVarinDoubleClicked )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VAROUT"),
					   Dbf2GaussDlg::OnCListVaroutDoubleClicked )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_ALL"), Dbf2GaussDlg::OnCMoveoutAllClick )
    EVT_BUTTON( XRCID("IDC_MOVEOUT_ONE"), Dbf2GaussDlg::OnCMoveoutOneClick )
    EVT_BUTTON( XRCID("IDC_MOVEIN_ONE"), Dbf2GaussDlg::OnCMoveinOneClick )
    EVT_BUTTON( XRCID("IDC_MOVEIN_ALL"), Dbf2GaussDlg::OnCMoveinAllClick )
    EVT_BUTTON( XRCID("IDOK_EXPORT"), Dbf2GaussDlg::OnOkExportClick )
    EVT_BUTTON( XRCID("IDOK_RESET"), Dbf2GaussDlg::OnOkResetClick )
    EVT_BUTTON( XRCID("IDC_INPUTFILE"), Dbf2GaussDlg::OnCInputfileClick )
    EVT_BUTTON( XRCID("IDC_OUTPUTFILE"), Dbf2GaussDlg::OnCOutputfileClick )
END_EVENT_TABLE()

extern void GaussExIm(char* fnme, char* otfl, char * list, int cnt,
					  int outOption);

Dbf2GaussDlg::Dbf2GaussDlg( )
{
	lists = NULL;
}

Dbf2GaussDlg::Dbf2GaussDlg( int  option, wxWindow* parent, wxWindowID id,
							 const wxString& caption, const wxPoint& pos,
							 const wxSize& size, long style )
{
	m_option = option;
	lists = NULL;
	Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_MOVEOUT_ALL"))->Enable(false);
	FindWindow(XRCID("IDC_MOVEOUT_ONE"))->Enable(false);
	FindWindow(XRCID("IDC_MOVEIN_ONE"))->Enable(false);
	FindWindow(XRCID("IDC_MOVEIN_ALL"))->Enable(false);
	FindWindow(XRCID("IDC_LIST_VARIN"))->Enable(false);
	FindWindow(XRCID("IDC_LIST_VAROUT"))->Enable(false);
	FindWindow(XRCID("IDC_OUTPUTFILE"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_DAT"))->Enable(false);
	FindWindow(XRCID("IDOK_EXPORT"))->Enable(false);

}

Dbf2GaussDlg::Dbf2GaussDlg( wxWindow* parent, wxWindowID id,
							 const wxString& caption, const wxPoint& pos,
							 const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

bool Dbf2GaussDlg::Create( wxWindow* parent, wxWindowID id,
						   const wxString& caption, const wxPoint& pos,
						   const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}


void Dbf2GaussDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_GAUSS_EXIM");
    m_inputfield = XRCCTRL(*this, "IDC_LIST_VARIN", wxListBox);
    m_inputfile = XRCCTRL(*this, "IDC_EDIT_DBF", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfield = XRCCTRL(*this, "IDC_LIST_VAROUT", wxListBox);
    m_outputfile = XRCCTRL(*this, "IDC_EDIT_DAT", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
}


void Dbf2GaussDlg::OnCListVarinDoubleClicked( wxCommandEvent& event )
{
	m_outputfield->Append(
					m_inputfield->GetString(m_inputfield->GetSelection()));
	m_inputfield->Delete(m_inputfield->GetSelection());
    event.Skip();
}


void Dbf2GaussDlg::OnCListVaroutDoubleClicked( wxCommandEvent& event )
{
	m_inputfield->Append(
					m_outputfield->GetString(m_outputfield->GetSelection()));
	m_outputfield->Delete(m_outputfield->GetSelection());
    event.Skip();
}

void Dbf2GaussDlg::OnCMoveoutAllClick( wxCommandEvent& event )
{
	for(unsigned int i=0; i<m_inputfield->GetCount(); i++) {
		m_outputfield->Append(m_inputfield->GetString(i));
	}
	m_inputfield->Clear();
    
    event.Skip();
}

void Dbf2GaussDlg::OnCMoveoutOneClick( wxCommandEvent& event )
{
	if(m_inputfield->GetSelection() >= 0) {
		m_outputfield->Append(
						m_inputfield->GetString(m_inputfield->GetSelection()));
		m_inputfield->Delete(m_inputfield->GetSelection());
	}
    event.Skip();
}

void Dbf2GaussDlg::OnCMoveinOneClick( wxCommandEvent& event )
{
 	if(m_outputfield->GetSelection() >= 0) {
		m_inputfield->Append(
					m_outputfield->GetString(m_outputfield->GetSelection()));
		m_outputfield->Delete(m_outputfield->GetSelection());
	}
    event.Skip();
}


void Dbf2GaussDlg::OnCMoveinAllClick( wxCommandEvent& event )
{
	int i = 0;
	for(unsigned i=0; i<m_outputfield->GetCount(); i++) {
		m_inputfield->Append(m_outputfield->GetString(i));
	}
	m_outputfield->Clear();
    
    event.Skip();
}


void Dbf2GaussDlg::OnOkExportClick( wxCommandEvent& event )
{
	wxString m_dbf = m_inputfile->GetValue();
	wxString m_dat = m_outputfile->GetValue();
	
	char buf_infl[512];
	strcpy( buf_infl, (const char*)m_dbf.mb_str(wxConvUTF8) );
	char* infl = buf_infl;

	char buf_otfl[512];
	strcpy( buf_otfl, (const char*)m_dat.mb_str(wxConvUTF8) );
	char* otfl = buf_otfl;

	wxString buf2=wxEmptyString; 

	int cnt=0;
	for (unsigned int i=0; i<m_outputfield->GetCount();i++) {
		buf2 += wxString::Format("%s,", m_outputfield->GetString(i).GetData());
		cnt++;
	}

	char buf_list[512];
	strcpy( buf_list, (const char*)buf2.mb_str(wxConvUTF8) );
	char* list = buf_list;

	GaussExIm(infl, otfl, list, cnt, m_option);

	event.Skip(); // wxDialog::OnOK(event);
	EndDialog(wxID_OK);
}


void Dbf2GaussDlg::OnOkResetClick( wxCommandEvent& event )
{
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}


void Dbf2GaussDlg::OnCInputfileClick( wxCommandEvent& event )
{
	LOG_MSG("Entering Dbf2GaussDlg::OnCInputfileClick"); 
    wxFileDialog dlg(this, "Input DBF file", "", "",
					 "DBF files (*.dbf)|*.dbf");

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString InputFile = m_path;
		m_inputfile->SetValue(InputFile);

		fname = dlg.GetFilename();
		int pos = fname.Find('.', true);
		if (pos >= 0) {
			fname = fname.Left(pos);
		}
        
		iDBF dbf(InputFile);
		if (!dbf.IsConnectedToFile()) {
			wxMessageBox("Error: DBF file doesn't exist,");
			return;
		}

		nfields = dbf.GetNumOfField();

		lists = new wxString[nfields];
		std::vector<wxString> temp(dbf.GetFieldNames());
		m_inputfield->Clear();
		m_outputfield->Clear();
		int i = 0, k = 0;

		for (i=0; i<nfields; i++) {
			if (dbf.GetFieldType(i) == 'N' || dbf.GetFieldType(i) == 'F') {
				lists[k] = temp[i];
				k++;
			}
		}	
		nfields = k;

		for (i=0; i<nfields; i++) {
			m_inputfield->Append(lists[i]);
		}
		delete [] lists;

		FindWindow(XRCID("IDC_MOVEOUT_ALL"))->Enable(true);
		FindWindow(XRCID("IDC_MOVEOUT_ONE"))->Enable(true);
		FindWindow(XRCID("IDC_MOVEIN_ONE"))->Enable(true);
		FindWindow(XRCID("IDC_MOVEIN_ALL"))->Enable(true);
		FindWindow(XRCID("IDC_LIST_VARIN"))->Enable(true);
		FindWindow(XRCID("IDC_LIST_VAROUT"))->Enable(true);
		FindWindow(XRCID("IDC_OUTPUTFILE"))->Enable(true);
		FindWindow(XRCID("IDC_EDIT_DAT"))->Enable(true);
	}

	LOG_MSG("Exiting Dbf2GaussDlg::OnCInputfileClick");
}


void Dbf2GaussDlg::OnCOutputfileClick( wxCommandEvent& event )
{
	wxString t1,t2,t3;

	if (m_option == 1) {
		t1 = "Gauss file";
		t2 = fname + ".dat";
		t3 = "Gauss files (*.dat)|*.dat";
	}
	else {	
		t1 = "ASCII file";
		t2 = fname + ".txt";
		t3 = "ASCII files (*.txt)|*.txt";
	}

    wxFileDialog dlg (this, t1, wxEmptyString, t2, t3,
					  wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);	

		FindWindow(XRCID("IDOK_EXPORT"))->Enable(true);
	}
}


bool Dbf2GaussDlg::ShowToolTips()
{
    return true;
}
