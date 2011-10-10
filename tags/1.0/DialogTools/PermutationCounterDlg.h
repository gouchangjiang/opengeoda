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

#ifndef __GEODA_CENTER_PERMUTATION_COUNTER_DLG_H__
#define __GEODA_CENTER_PERMUTATION_COUNTER_DLG_H__

#include <wx/textctrl.h>

#define IDD_PERMUTATION_COUNT 10000
#define SYMBOL_CPERMUTATIONCOUNTERDLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CPERMUTATIONCOUNTERDLG_TITLE "Set Number of Permutation"
#define SYMBOL_CPERMUTATIONCOUNTERDLG_IDNAME IDD_PERMUTATION_COUNT
#define SYMBOL_CPERMUTATIONCOUNTERDLG_SIZE wxDefaultSize
#define SYMBOL_CPERMUTATIONCOUNTERDLG_POSITION wxDefaultPosition

class CPermutationCounterDlg: public wxDialog
{    
    DECLARE_CLASS( CPermutationCounterDlg )
    DECLARE_EVENT_TABLE()

public:
    CPermutationCounterDlg( );
    CPermutationCounterDlg( wxWindow* parent, wxWindowID id = -1,
						   const wxString& caption="Set Number of Permutation",
						   const wxPoint& pos = wxDefaultPosition,
						   const wxSize& size = wxDefaultSize,
						   long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Set Number of Permutation",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnOkClick( wxCommandEvent& event );

    wxBitmap GetBitmapResource( const wxString& name );

    wxIcon GetIconResource( const wxString& name );

    wxTextCtrl* m_number;
	wxString s_int;
};

#endif
