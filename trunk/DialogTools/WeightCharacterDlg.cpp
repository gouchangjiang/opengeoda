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
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../GenUtils.h"
#include "WeightCharacterDlg.h"

BEGIN_EVENT_TABLE( WeightCharacterDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_OPEN_FILEWEIGHT"),
			   WeightCharacterDlg::OnCOpenFileweightClick )
    EVT_BUTTON( XRCID("wxID_OK"), WeightCharacterDlg::OnOkClick )
END_EVENT_TABLE()

WeightCharacterDlg::WeightCharacterDlg( wxWindow* parent,
									   DbfGridTableBase* grid_base_s,
									   wxWindowID id,
									   const wxString& caption,
									   const wxPoint& pos, const wxSize& size,
									   long style )
: grid_base(grid_base_s)
{
	Create(parent, id, caption, pos, size, style);
	m_WeightFile = wxEmptyString;
	m_freq = NULL;
}

bool WeightCharacterDlg::Create( wxWindow* parent, wxWindowID id,
								 const wxString& caption, const wxPoint& pos,
								 const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
	FindWindow(XRCID("wxID_OK"))->Enable(false);

    return true;
}

void WeightCharacterDlg::CreateControls()
{    

    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_WEIGHT_CHARACTERISTICS");
    m_name = XRCCTRL(*this, "IDC_EDIT_FILEWEIGHT", wxTextCtrl);
	m_name->SetMaxLength(0);
}


void WeightCharacterDlg::OnOkClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_OK);
}


void WeightCharacterDlg::OnCOpenFileweightClick( wxCommandEvent& event )
{
    wxFileDialog dlg(this, "Input Weights File", "", "",
                    "Weights Files (*.gal; *.gwt)|*.gal;*.gwt");

	// m_path contains the path and the file name + ext
	wxString	m_path = wxEmptyString;
	bool done = false;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		m_name->SetValue(m_path);

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

		int num_obs = grid_base->GetNumberRows();
		m_freq = new long[num_obs];
		for (int i=0; i<num_obs; i++) m_freq[i]=0;
		
		if (ext == "gal") {
			GalElement* tempGal = WeightUtils::ReadGal(m_path, grid_base);
			if (tempGal == NULL) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				delete [] m_freq;
				m_freq = NULL;
				return;
			} else {
				for (int i=0; i<num_obs;i++) m_freq[i] = tempGal[i].Size();
				FindWindow(XRCID("wxID_OK"))->Enable(true);
				done = true;
				delete [] tempGal;
			}
		} else if (ext == "gwt") {
			GalElement* tempGal = WeightUtils::ReadGwtAsGal(m_path, grid_base);
			if (tempGal == NULL) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				delete [] m_freq;
				m_freq = NULL;
				return;
			} else {
				for (int i=0; i<num_obs;i++) m_freq[i] = tempGal[i].Size();
				FindWindow(XRCID("wxID_OK"))->Enable(true);
				done = true;
				delete [] tempGal;
			}
		} else {
			wxMessageBox("Wrong file extension!");
		}
	}

}


