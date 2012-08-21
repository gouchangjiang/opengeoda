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

#ifndef __GEODA_CENTER_DBF_2_GAUSS_DLG_H__
#define __GEODA_CENTER_DBF_2_GAUSS_DLG_H__

class Dbf2GaussDlg: public wxDialog
{
    DECLARE_EVENT_TABLE()

public:
    Dbf2GaussDlg();
    Dbf2GaussDlg( int option, wxWindow* parent, wxWindowID id = -1,
				  const wxString& caption = "Exporting Data",
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = wxCAPTION|wxSYSTEM_MENU );
    Dbf2GaussDlg( wxWindow* parent, wxWindowID id = -1,
				  const wxString& caption = "Exporting Data",
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = wxCAPTION|wxSYSTEM_MENU );
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Exporting Data",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnCListVarinDoubleClicked( wxCommandEvent& event );
    void OnCListVaroutDoubleClicked( wxCommandEvent& event );
    void OnCMoveoutAllClick( wxCommandEvent& event );
    void OnCMoveoutOneClick( wxCommandEvent& event );
    void OnCMoveinOneClick( wxCommandEvent& event );
    void OnCMoveinAllClick( wxCommandEvent& event );
    void OnOkExportClick( wxCommandEvent& event );
    void OnOkResetClick( wxCommandEvent& event );
    void OnCInputfileClick( wxCommandEvent& event );
    void OnCOutputfileClick( wxCommandEvent& event );

    static bool ShowToolTips();

    wxListBox* m_inputfield;
    wxTextCtrl* m_inputfile;
    wxListBox* m_outputfield;
    wxTextCtrl* m_outputfile;

	wxString *lists, fname;
	int nfields;
	int m_option;
};

#endif
