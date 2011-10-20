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
#include "RegressionTitleDlg.h"

BEGIN_EVENT_TABLE( CRegressionTitleDlg, wxDialog )
    EVT_BUTTON( wxID_OK, CRegressionTitleDlg::OnOkClick )
    EVT_BUTTON( XRCID("ID_BROWSE"), CRegressionTitleDlg::OnBrowseClick )
END_EVENT_TABLE()

CRegressionTitleDlg::CRegressionTitleDlg( )
{
}

CRegressionTitleDlg::CRegressionTitleDlg( wxWindow* parent, wxString fname,
										 wxWindowID id, 
										 const wxString& caption,
										 const wxPoint& pos,
										 const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	m_title->SetValue("Regression");
	m_outputfile->SetValue(fname);

	FindWindow(XRCID("wxID_OK"))->Enable(true);
}

bool CRegressionTitleDlg::Create( wxWindow* parent, wxWindowID id,
								 const wxString& caption, const wxPoint& pos,
								 const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}

void CRegressionTitleDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_REGRESSION_SAVE");
    m_title = XRCCTRL(*this, "IDC_EDIT1", wxTextCtrl);
    m_outputfile = XRCCTRL(*this, "IDC_EDIT2", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_check1 = XRCCTRL(*this, "IDC_CHECK1", wxCheckBox);
    m_check2 = XRCCTRL(*this, "IDC_CHECK2", wxCheckBox);
    m_check3 = XRCCTRL(*this, "IDC_CHECK3", wxCheckBox);
}

void CRegressionTitleDlg::OnOkClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_OK);
}

bool CRegressionTitleDlg::ShowToolTips()
{
    return true;
}

void CRegressionTitleDlg::OnBrowseClick( wxCommandEvent& event )
{
    wxFileDialog dlg(this,
                    "Output file",
                    wxEmptyString,
                    fn + "Regression.txt",
                    "All files (*.txt)|*.txt",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// m_path contains the path and the file name + ext
	wxString m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);

	}
}
