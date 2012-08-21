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
#include <wx/image.h>
#include <wx/filedlg.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../OpenGeoDa.h"
#include "../Thiessen/VorDataType.h"

#include "AddCentroidsDlg.h"

extern bool AddCentroidsDBF(wxString fname, wxString odbf, wxString keyname, 
							double* key, const std::vector<wxString>& keyst,
							bool keytype, bool m_mean_center);
extern long GetShpFileSize(const wxString& fname);

IMPLEMENT_CLASS( AddCentroidsDlg, wxDialog )

BEGIN_EVENT_TABLE( AddCentroidsDlg, wxDialog )
    EVT_BUTTON( XRCID("IDOK_ADD"), AddCentroidsDlg::OnOkAddClick )
    EVT_BUTTON( XRCID("IDC_OPEN_ISHAPE"), AddCentroidsDlg::OnCOpenIshapeClick )
    EVT_BUTTON( XRCID("IDCANCEL"), AddCentroidsDlg::OnCancelClick )
    EVT_BUTTON( XRCID("IDC_OPEN_ODBF"), AddCentroidsDlg::OnCOpenOdbfClick )
END_EVENT_TABLE()

AddCentroidsDlg::AddCentroidsDlg( )
{
}

AddCentroidsDlg::AddCentroidsDlg( bool mean_center, wxWindow* parent,
								   wxWindowID id, const wxString& caption,
								   const wxPoint& pos, const wxSize& size,
								   long style )
{
	m_mean_center = mean_center;
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_ODBF"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_ODBF"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR"))->Enable(false);
	FindWindow(XRCID("IDOK_ADD"))->Enable(false);

}

bool AddCentroidsDlg::Create( wxWindow* parent, wxWindowID id,
							  const wxString& caption, const wxPoint& pos,
							  const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}


void AddCentroidsDlg::CreateControls()
{    

	if (m_mean_center)
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_ADD_MEANCENTERS");
	else
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_ADD_CENTROIDS");
    m_inputfile = XRCCTRL(*this, "IDC_EDIT_FILEWEIGHT", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_EDIT_ODBF", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_X = XRCCTRL(*this, "IDC_KEYVAR", wxChoice);
}


void AddCentroidsDlg::OnOkAddClick( wxCommandEvent& event )
{
	double		*key;
	wxString	keyname;
	std::vector<wxString> keyst;
	bool		keytype;

	wxString m_iShape = m_inputfile->GetValue();
	wxString m_oDBF = m_outputfile->GetValue();
	
	keyname = m_X->GetString(m_X->GetSelection());
	long obs = GetShpFileSize(m_iShape);

	iDBF tb(m_iShape);

	if (!tb.IsConnectedToFile()) {
		wxMessageBox("Can't open data!");
		return;
	}

	char ty = tb.GetFieldType(tb.GetIndexField(keyname));
	int i = 0;

	if (ty == 'C') {
		keytype = false;
		charPtr *dt = new charPtr[obs + 1];
		bool f = tb.GetStrDataArray(keyname, dt);
		if (!f) {
			wxMessageBox("Failed adding centroids!!");
			delete [] dt;
			dt = NULL;
			return;
		}
		keyst.resize(obs);
		for (int i=0; i<obs; i++) keyst[i] = wxString(dt[i], wxConvUTF8);

		delete [] dt;
		dt = NULL;
	} else {
		keytype = true;
		key = new double[obs+1];
		bool f = tb.GetDblDataArray(keyname, key);
		if (!f) 
		{
			wxMessageBox("Failed adding centroids!!");
			delete [] key;
			key = NULL;
			return;
		}
	}
	
	if (!AddCentroidsDBF(m_iShape, m_oDBF, keyname, key, keyst,
						 keytype, m_mean_center))
	{
		wxMessageBox("Failed adding centroids!!");
	} else {
		FindWindow(XRCID("IDCANCEL"))->SetLabel("Done");
	}

	if (key) delete [] key;
	key = NULL;

	event.Skip();
}

void AddCentroidsDlg::OnCOpenIshapeClick( wxCommandEvent& event )
{
    wxFileDialog dlg(this,
                    "Input Shp file",
                    "",
                    "",
                    "Shp files (*.shp)|*.shp");

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;
	bool m_polyid = false;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString onlyFileName = dlg.GetPath();
		wxString InFile = m_path;
		wxString m_iShape = m_path;
		m_inputfile->SetValue(InFile);

		int pos = onlyFileName.Find('.',true);
		if(pos >= 0)
			onlyFileName = onlyFileName.Left(pos);
		fn = dlg.GetFilename();
		pos = fn.Find('.', true);
		if (pos >= 0) fn = fn.Left(pos);

		iDBF tb(onlyFileName);

		int pid = 0;
		if (tb.IsConnectedToFile()) {
			int numfields; 
			numfields = tb.GetNumOfField();

			m_X->Clear();

			for (int i=0; i<numfields; i++) {
				if (tb.GetFieldType(i)== 'N' ||  
					tb.GetFieldType(i)== 'C') {
					m_X->Append(wxString::Format("%s",tb.GetFieldName(i)));
					if (tb.GetFieldName(i) == "POLYID") {
						m_polyid = true;
						pid = m_X->GetCount()-1;
					}
				}
			}
			if (numfields > 0) {
				if (m_polyid) {
					keyname = "POLYID";
					m_X->SetSelection(pid);
				} else {
					m_X->SetSelection(0);
					keyname = m_X->GetString(m_X->GetSelection());
				}
			}
		}

		FindWindow(XRCID("IDC_OPEN_ODBF"))->Enable(true);
		FindWindow(XRCID("IDC_EDIT_ODBF"))->Enable(true);
		FindWindow(XRCID("IDC_KEYVAR"))->Enable(true);
	}	
}

void AddCentroidsDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);

}

void AddCentroidsDlg::OnCOpenOdbfClick( wxCommandEvent& event )
{
    wxFileDialog dlg
                 (
                    this,
                    "Output Dbf file",
                    wxEmptyString,
                    fn + ".dbf",
                    "Dbf files (*.dbf)|*.dbf",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;


    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);


		FindWindow(XRCID("IDOK_ADD"))->Enable(true);
	}
}


bool AddCentroidsDlg::ShowToolTips()
{
    return true;
}
