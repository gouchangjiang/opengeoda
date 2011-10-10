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
// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "ASC2SHPDlg.h"

extern bool CreatePointShapeFile(
                char* otfl,         // Target File name
                const int nPoint,   // Number of point or records
                double *x,          // x-coordinates vector
                double *y);          // y-ccordinates vector


BEGIN_EVENT_TABLE( CASC2SHPDlg, wxDialog )
    EVT_BUTTON( XRCID("IDOK_ADD"), CASC2SHPDlg::OnOkAddClick )
    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), CASC2SHPDlg::OnCOpenIascClick )
    EVT_BUTTON( XRCID("IDC_OPEN_OSHP"), CASC2SHPDlg::OnCOpenOshpClick )
    EVT_BUTTON( XRCID("IDCANCEL"), CASC2SHPDlg::OnCancelClick )
END_EVENT_TABLE()

CASC2SHPDlg::CASC2SHPDlg( )
{
}

CASC2SHPDlg::CASC2SHPDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR2"))->Enable(false);
	FindWindow(XRCID("IDOK_ADD"))->Enable(false);
}

bool CASC2SHPDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();

    return true;
}

void CASC2SHPDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_CONVERT_ASC2SHP");
    m_inputfile = XRCCTRL(*this, "IDC_FIELD_ASC", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_FIELD_SHP", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_X = XRCCTRL(*this, "IDC_KEYVAR", wxChoice);
    m_Y = XRCCTRL(*this, "IDC_KEYVAR2", wxChoice);
}

void CASC2SHPDlg::OnOkAddClick( wxCommandEvent& event )
{
	wxString m_iASC = m_inputfile->GetValue();
	wxString m_oSHP = m_outputfile->GetValue();
    int idx_x = m_X->GetSelection();
	int idx_y = m_Y->GetSelection();
	std::ifstream ias;
	ias.open(m_iASC.wx_str());
	int n_recs;
	int n_fields;
		
	// FORMAT: Line-1
	char name[10000];
	ias.getline(name,100);
	sscanf(name,"%d,%d",&n_recs, &n_fields);

    if( (n_recs <= 0) || (n_fields < 2) ) {
		wxString msg = "Number of columns has to be more than 2\n";
		msg += "At least it includes ID,X-Coord, and Y-Coord!";
		wxMessageBox(msg);
		return;
	}

	// FORMAT: Line-2
	ias.getline(name,10000);

	wxString tok = wxString(name, wxConvUTF8);
	//wxString tok = wxString::Format("%10000",name);
	wxString t;


	DBF_descr *dbfdesc;
	dbfdesc = new DBF_descr [n_fields];
	//char* tok_c;

	int i = 0, j = 0;
	for(i=0; i<n_fields-1; i++)
	{
		int pos = tok.Find(',');
		if( pos >= 0)
		{
			wxString tok_left = tok.Left(pos);
			j = tok_left.Find('\"');
			while (j >= 0)
			{
				t = tok_left.Left(j);
				tok_left = t + tok_left.Right(tok_left.Length() - j - 1);
				j = tok_left.Find('\"');
			}

			dbfdesc[i] = new DBF_field(tok_left, 'N', 20, 9);
			tok = tok.Right(tok.Length()-pos-1);
		}
		else
		{
			wxString msg = "Error: there was a problem reading the\n";
			msg +=         "    field names from the text file.";
			wxMessageBox(msg);
			return;
		}
	}
	j = tok.Find('\"');
	while (j >= 0)
	{
		t = tok.Left(j);
		tok = t + tok.Right(tok.Length() - j - 1);
		j = tok.Find('\"');
	}
	
	dbfdesc[i] = new DBF_field(tok, 'N', 20, 9);
	dbfdesc[0]->Precision = 0;
	
	double *x,*y;
	x = new double[n_recs];
	y = new double[n_recs];	

	double *buff;
	buff = new double[n_recs*n_fields];

	int row = 0, col = 0;
	for(row=0; row <n_recs; row++)
	{
		ias.getline(name, 10000);
		tok = wxString(name, wxConvUTF8);
		for(col=0; col<n_fields-1; col++)
		{
			int pos = tok.Find(',');
			if( pos >= 0)
			{
				wxString tok_left = tok.Left(pos);
				if(idx_x == col)
				{
					tok.ToCDouble(&x[row]);
				}
				if(idx_y == col)
				{
					tok.ToCDouble(&y[row]);
				}
				tok.ToCDouble(&buff[row*n_fields+col]);

				tok = tok.Right(tok.Length()-pos-1);
			}
			else
			{
				wxMessageBox("Error: Wrong format.");
				delete [] x; x = NULL;
				delete [] y; y = NULL;
				delete [] buff; buff = NULL;
				delete [] dbfdesc; dbfdesc = NULL;
				return;
			}
		}

		if(idx_x == n_fields-1)
		{
			tok.ToCDouble(&x[row]);
		}
		if(idx_y == n_fields-1)
		{
			tok.ToCDouble(&y[row]);
		}
		tok.ToCDouble(&buff[row*n_fields+col]);

	}

	char buf[512];
	strcpy( buf, (const char*)m_oSHP.mb_str(wxConvUTF8) );
	CreatePointShapeFile(buf, n_recs, x, y);

	wxString strResult = m_oSHP;
	wxString DirName;

	int pos = strResult.Find('.', true);
	if (pos >= 0) 
		DirName = strResult.Left(pos);
	else 
		DirName = strResult;

	DirName = DirName+ ".dbf";

	oDBF odbf(DirName, dbfdesc, n_recs, n_fields);

	if(odbf.fail)
	{
		wxMessageBox("Can't open output file!");

		delete [] x;
		x = NULL;
		delete [] y;
		y = NULL;
		delete [] buff;
		buff = NULL;
		delete [] dbfdesc;
		dbfdesc = NULL;
		return;
	}	

	for(row=0; row<n_recs; row++) {
		for(col=0; col<n_fields; col++) {
			if (col == 0) {
				odbf.Write((int)buff[row*n_fields+col]);
			} else {
				odbf.Write((double)buff[row*n_fields+col]);
			}
		}
	}
	
	odbf.close();

	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;
	delete [] buff;
	buff = NULL;
	delete [] dbfdesc;
	dbfdesc = NULL;

	event.Skip(); // wxDialog::OnOK(event);

}


void CASC2SHPDlg::OnCOpenIascClick( wxCommandEvent& event )
{
    wxFileDialog dlg( this, "Input ASCII file", "", "",
					 "ASCII files (*.txt)|*.txt");

	// m_path contains the path and the file name + ext
	wxString m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path = dlg.GetPath();
		wxString InFile = m_path;
		m_inputfile->SetValue(InFile);
		wxString m_iASC = m_path;

		fn = dlg.GetFilename();
		int pos = fn.Find('.', true);
		if (pos >= 0) fn = fn.Left(pos);

		m_X->Clear();
		m_Y->Clear();

		ifstream ias;
		ias.open(m_iASC.wx_str());

		int n_recs;
		int n_fields;
		
		char name[1000];

		ias.getline(name,100);
		sscanf(name,"%d,%d",&n_recs, &n_fields);

		if( (n_recs <= 0) ) {
			wxString msg = "Error: number of records must be > 0,\nbut only ";
			msg += n_recs + " records found.";
			wxMessageBox(msg);
		}

		if( (n_fields <= 2) ) {
			wxString msg = "Error: number of fields must be > 2,\nbut only ";
			msg += n_fields + " fields found.";
			wxMessageBox(msg);
		}

		ias.getline(name,1000);

		wxString tok, t, k;
		tok = wxString(name, wxConvUTF8);
		int i = 0, j = 0;
		for(i=0; i<n_fields-1; i++)
		{
			int pos = tok.Find(',');
			if( pos >= 0)
			{
				t = tok.Left(pos);
				j = t.Find('\"');
				while (j >= 0)
				{
					k = t.Left(j);
					t = k + t.Right(t.Length() - j - 1);
					j = t.Find('\"');
				}
				m_X->Append(t);
				m_Y->Append(t);
				tok = tok.Right(tok.Length()-pos-1);
			}
			else
			{
				m_X->Clear();
				m_Y->Clear();
				wxMessageBox("Error: field names listed in wrong format.");
				return;
			}
		}

		t = tok;
		j = t.Find('\"');
		while (j >= 0)
		{
			k = t.Left(j);
			t = k + t.Right(t.Length() - j - 1);
			j = t.Find('\"');
		}
		m_X->Append(t);
		m_Y->Append(t);

        m_X->SetSelection(0);
		m_Y->SetSelection(0);

		FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(true);
		FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(true);
		FindWindow(XRCID("IDC_KEYVAR"))->Enable(true);
		FindWindow(XRCID("IDC_KEYVAR2"))->Enable(true);
	}		 
}

void CASC2SHPDlg::OnCOpenOshpClick( wxCommandEvent& event )
{
    wxFileDialog dlg(this, "Output Shp file", wxEmptyString, fn + ".shp",
                    "Shp files (*.shp)|*.shp",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);
		FindWindow(XRCID("IDOK_ADD"))->Enable(true);
	}
}

void CASC2SHPDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip(); // wxDialog::OnCancel(event);
	EndDialog(wxID_CANCEL);
}

bool CASC2SHPDlg::ShowToolTips()
{
    return true;
}

