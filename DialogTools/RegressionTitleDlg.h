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

#ifndef __GEODA_CENTER_REGRESSION_TITLE_DLG_H__
#define __GEODA_CENTER_REGRESSION_TITLE_DLG_H__

#define IDD_REGRESSION_SAVE 10000
#define ID_BROWSE 10002

/*!
 * CRegressionTitleDlg class declaration
 */

class CRegressionTitleDlg: public wxDialog
{
public:
    CRegressionTitleDlg( );
    CRegressionTitleDlg( wxWindow* parent, wxString fname, wxWindowID id = -1,
						const wxString& caption = "Regression Title & Output",
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Regression Title & Output",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();

    void OnOkClick( wxCommandEvent& event );
    void OnBrowseClick( wxCommandEvent& event );
    static bool ShowToolTips();

    wxTextCtrl* m_title;
    wxTextCtrl* m_outputfile;
    wxCheckBox* m_check1;
    wxCheckBox* m_check2;
    wxCheckBox* m_check3;

	wxString fn;
	
	DECLARE_EVENT_TABLE()
};

#endif

