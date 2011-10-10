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

#include <wx/valtext.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../GenUtils.h"
#include "WeightCharacterDlg.h"

extern wxString gCompleteFileName;
extern int gObservation;
extern	GalElement	*ReadTxtGal(const char *fname, long gObs,
								const wxString& full_dbf_name);
extern	GwtElement	*ReadTxtGwt(const char *fname, long gObs,
								const wxString& full_dbf_name);


BEGIN_EVENT_TABLE( CWeightCharacterDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_OPEN_FILEWEIGHT"),
			   CWeightCharacterDlg::OnCOpenFileweightClick )
    EVT_BUTTON( XRCID("wxID_OK"), CWeightCharacterDlg::OnOkClick )
END_EVENT_TABLE()

CWeightCharacterDlg::CWeightCharacterDlg( )
{
	m_gal = NULL;
	m_gwt = NULL;
	m_freq = NULL;
}

CWeightCharacterDlg::CWeightCharacterDlg( wxWindow* parent, wxWindowID id,
										 const wxString& caption,
										 const wxPoint& pos, const wxSize& size,
										 long style )
{
	Create(parent, id, caption, pos, size, style);
	m_obs = gObservation;
	m_gwt = NULL;
	m_gal = NULL;
	m_WeightFile = wxEmptyString;
	m_freq = NULL;
}

bool CWeightCharacterDlg::Create( wxWindow* parent, wxWindowID id,
								 const wxString& caption, const wxPoint& pos,
								 const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
	FindWindow(XRCID("wxID_OK"))->Enable(false);

    return true;
}

void CWeightCharacterDlg::CreateControls()
{    

    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_WEIGHT_CHARACTERISTICS");
    m_name = XRCCTRL(*this, "IDC_EDIT_FILEWEIGHT", wxTextCtrl);
	m_name->SetMaxLength(0);
}


void CWeightCharacterDlg::OnOkClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_OK);
}


void CWeightCharacterDlg::OnCOpenFileweightClick( wxCommandEvent& event )
{
    wxFileDialog dlg(this, "Input Weights File", "", "",
                    "Weights Files (*.gal; *.gwt)|*.gal;*.gwt");

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;
	bool done = false;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		m_name->SetValue(m_path);
		
		char buf_flname[512];
		strcpy( buf_flname, (const char*)m_path.mb_str(wxConvUTF8) );
		char* flname = buf_flname;

		wxString ext = wxEmptyString;
		wxString fname = m_path;

		int pos = m_path.Find('.',true);
		if (pos > 0) {
			ext = m_path.Right(m_path.Length()-pos-1);
		}

		pos = m_path.Find('/', true);
		if(pos > 0) {
			fname = m_path.Right(m_path.Length()-pos-1);
		}
		this->m_WeightFile = fname;

		ext.MakeLower();

		m_freq = NULL;
		m_freq = new long[m_obs];
		for (int j=0;j<m_obs; j++) m_freq[j] = 0;

		if (ext == "gal") {
			if ((m_gal = ReadTxtGal(flname, m_obs,
				  GenUtils::swapExtension(gCompleteFileName, "dbf"))) == NULL) {
				wxMessageBox("Fail reading "+ m_path);
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				delete [] m_freq;
				m_freq = NULL;
			} else {
				m_freq = new long[m_obs];
				for (int i=0; i<m_obs;i++) m_freq[i] = m_gal[i].Size();
	
				FindWindow(XRCID("wxID_OK"))->Enable(true);
				done = true;
				delete [] m_gal; m_gal = NULL;
			}
		} else if(ext == "gwt") {
			int type = 0;
			if ((m_gwt = ReadTxtGwt(flname, m_obs,
				  GenUtils::swapExtension(gCompleteFileName, "dbf"))) == NULL) {
				wxMessageBox("Fail reading "+m_path);
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				delete [] m_freq;
				m_freq = NULL;
			} else {
				for (int i=0; i<m_obs;i++) m_freq[i] = m_gwt[i].nbrs; 
		
				FindWindow(XRCID("wxID_OK"))->Enable(true);
				done = true;
				delete [] m_gwt; m_gwt = NULL;
			}
		} else {
			wxMessageBox("Wrong file extension!");
		}
	}

}


bool CWeightCharacterDlg::ShowToolTips()
{
    return true;
}

