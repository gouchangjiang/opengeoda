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

#include <fstream>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/xrc/xmlres.h>

#include "Bnd2ShpDlg.h"

extern bool CreateSHPfromBoundary(wxString ifl, wxString otfl);

IMPLEMENT_CLASS( Bnd2ShpDlg, wxDialog )

/*!
 * Bnd2ShpDlg event table definition
 */

BEGIN_EVENT_TABLE( Bnd2ShpDlg, wxDialog )

////@begin Bnd2ShpDlg event table entries
    EVT_BUTTON( XRCID("ID_CREATE"), Bnd2ShpDlg::OnCreateClick )

    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), Bnd2ShpDlg::OnCOpenIascClick )

    EVT_BUTTON( XRCID("IDC_OPEN_OSHP"), Bnd2ShpDlg::OnCOpenOshpClick )

    EVT_BUTTON( XRCID("IDCANCEL"), Bnd2ShpDlg::OnCancelClick )

////@end Bnd2ShpDlg event table entries

END_EVENT_TABLE()

/*!
 * Bnd2ShpDlg constructors
 */

Bnd2ShpDlg::Bnd2ShpDlg( )
{
}

Bnd2ShpDlg::Bnd2ShpDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(false);
	FindWindow(XRCID("ID_CREATE"))->Enable(false);
}

/*!
 * Bnd2ShpDlg creator
 */

bool Bnd2ShpDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin Bnd2ShpDlg member initialisation
////@end Bnd2ShpDlg member initialisation

////@begin Bnd2ShpDlg creation
    SetParent(parent);
    CreateControls();
    Centre();
////@end Bnd2ShpDlg creation
    return true;
}

/*!
 * Control creation for Bnd2ShpDlg
 */

void Bnd2ShpDlg::CreateControls()
{    
////@begin Bnd2ShpDlg content construction

    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_CONVERT_BOUNDARY_TO_SHP");
    m_inputfile = XRCCTRL(*this, "IDC_FIELD_ASC", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_FIELD_SHP", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
////@end Bnd2ShpDlg content construction

    // Create custom windows not generated automatically here.

////@begin Bnd2ShpDlg content initialisation

////@end Bnd2ShpDlg content initialisation
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CREATE
 */

void Bnd2ShpDlg::OnCreateClick( wxCommandEvent& event )
{
	wxString m_iASC = m_inputfile->GetValue();
	wxString m_oSHP = m_outputfile->GetValue();

	if (!CreateSHPfromBoundary(m_iASC, m_oSHP))
	{
		wxMessageBox("Fail in reading the input file!");
			return;
	}


	event.Skip(); // wxDialog::OnOK(event);

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_IASC
 */

void Bnd2ShpDlg::OnCOpenIascClick( wxCommandEvent& event )
{


    wxFileDialog dlg
                 (
                    this,
                    "Input ASCII file",
                    "",
                    "",
                    "ASCII files (*.*)|*.*"
                 );

	wxString	m_path = wxEmptyString;// m_path contains the path and the file name + ext
	bool m_polyid = false;

    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString onlyFileName = dlg.GetPath();
		wxString InFile = m_path;
		wxString m_iASC = m_path;
		m_inputfile->SetValue(InFile);


                fn = dlg.GetFilename();
                int j = fn.Find('.', true);
                if (j >= 0) fn = fn.Left(j);
        

		std::ifstream ias;
		ias.open(m_iASC.mb_str());
		char name[1000];

		ias.getline(name,100);
		//wxString tok = wxString::Format("%100s", name);
		wxString tok = wxString(name, wxConvUTF8);
		wxString ID_name= wxEmptyString; 
		long nRows;

		int pos = tok.Find(',');
		if( pos >= 0)
		{
			wxString tl = tok.Left(pos);
			//nRows = (double) atof(tl.ToAscii());
			tl.ToCLong(&nRows);
			ID_name = tok.Right(tok.Length()-pos-1);
		}
		else
		{
			wxMessageBox("The first line should have comma separated number of rows and ID name!");
			return;
		}

		ID_name.Trim(true);
		ID_name.Trim(false);

		if (nRows < 1)
		{
			wxMessageBox("Wrong number of rows!");
	 			return;
		}
		else if(ID_name == wxEmptyString)
		{
			wxMessageBox("ID is not specified!");
			return;
		}


		FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(true);
		FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(true);

    }

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDC_OPEN_OSHP
 */

void Bnd2ShpDlg::OnCOpenOshpClick( wxCommandEvent& event )
{
    // Insert custom code here   
	wxFileDialog dlg
                 (
                    this,
                    "Output Shp file",
                    wxEmptyString,
                    fn + ".shp",
                    "Shp files (*.shp)|*.shp",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

	wxString	m_path = wxEmptyString;// m_path contains the path and the file name + ext


    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);

		FindWindow(XRCID("ID_CREATE"))->Enable(true);
	}
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for IDCANCEL
 */

void Bnd2ShpDlg::OnCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);	
}

/*!
 * Should we show tooltips?
 */

bool Bnd2ShpDlg::ShowToolTips()
{
    return true;
}

