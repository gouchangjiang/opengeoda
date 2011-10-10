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

#include <cctype>
#include <string>
#include <vector>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/DbfFile.h"
#include "AddIdVariable.h"

BEGIN_EVENT_TABLE( AddIdVariable, wxDialog )
    EVT_BUTTON( wxID_OK, AddIdVariable::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, AddIdVariable::OnCancelClick )
END_EVENT_TABLE()

AddIdVariable::AddIdVariable(const wxString& dbf_fnm,
							   wxWindow* parent, wxWindowID id,
							   const wxString& caption,
							   const wxPoint& pos, const wxSize& size,
							   long style )
{
	Create(dbf_fnm, parent, id, caption, pos, size, style);
}

bool AddIdVariable::Create(const wxString& dbf_fnm,
							wxWindow* parent, wxWindowID id,
							const wxString& caption,
							const wxPoint& pos, const wxSize& size,
							long style )
{
	dbf_fname = dbf_fnm;
	DbfFileReader dbf_file(dbf_fnm);
	fields = dbf_file.getFieldDescs();
	
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
	
    return true;
}

void AddIdVariable::CreateControls()
{    
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_ADD_ID_VARIABLE");
	if (FindWindow(wxXmlResource::GetXRCID("IDC_NEW_ID_VAR"))) {
		new_id_var = 
			wxDynamicCast(FindWindow(XRCID("IDC_NEW_ID_VAR")),
						  wxTextCtrl);
	}	
	if (FindWindow(XRCID("IDC_EXISTING_VARS_LIST"))) {
		existing_vars_list = 
			wxDynamicCast(FindWindow(XRCID("IDC_EXISTING_VARS_LIST")),
						  wxListBox);
		existing_vars_list->Clear();	
		for(int i=0, iend=fields.size(); i<iend; i++) {
			existing_vars_list->Append(fields[i].name);
		}		
	}
}

void AddIdVariable::OnOkClick( wxCommandEvent& event )
{
	new_id_var_name = new_id_var->GetValue().MakeUpper();
	
	if ( !DbfFileUtils::isValidFieldName(new_id_var_name) ) {
		wxString msg;
		msg += "Error: \"" + new_id_var_name + "\" is an invalid ";
		msg += "field name.  A valid field name is between one and ten ";
		msg += "characters long.  The first character must be alphabetic,";
		msg += " and the remaining characters can be either alphanumeric ";
		msg += "or underscores.";
		wxMessageDialog dlg(this, msg, "Error",
							wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
		
	for (int i=0, iend=fields.size(); i<iend; i++) {
		if (fields[i].name.CmpNoCase(new_id_var_name) == 0) {
			wxString msg;
			msg += "Error: Field name \"" + new_id_var_name;
			msg	+= "\" already exists in the DBF ";
			msg	+= "file.  Please choose a different name.";
			wxMessageBox(msg);
			return;
		}
	}
	
	// double check that the user wants to overwrite the DBF
	wxString conf_msg = "Are you sure you want to add the new id ";
	conf_msg+="variable to the DBF file associated with the chosen input";
	conf_msg+=" SHP file \"";
	conf_msg+=wxFileName(dbf_fname).GetName() + "\".";
	wxMessageDialog confirm_dlg(this, conf_msg,
								"Add id variable to DBF file?",
								wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	if (confirm_dlg.ShowModal() != wxID_YES ) {
		return;
	}
	
	wxString err_msg;
	bool ret=DbfFileUtils::insertIdFieldDbf(dbf_fname, dbf_fname,
											new_id_var_name, 0,
											err_msg);
	if (!ret) {
		wxMessageBox(err_msg);
		return;
	}
	
	wxString msg = "ID field " + new_id_var_name + " successfully";
	msg += " added to DBF part of shape file.";
	wxMessageBox(msg);
	
	event.Skip();
	EndDialog(wxID_OK);
}

void AddIdVariable::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
}

bool AddIdVariable::ShowToolTips()
{
	return true;
}

wxString AddIdVariable::GetIdVarName()
{
	return new_id_var_name;
}
