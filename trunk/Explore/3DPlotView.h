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

#ifndef __3D_PLOT_VIEW_H__
#define __3D_PLOT_VIEW_H__

#include <wx/glcanvas.h>

class DbfGridTableBase;
class C3DPlotFrame;
class C3DControlPan;
class Arcball;

class C3DPlotCanvas: public wxGLCanvas
{
public:
	C3DPlotCanvas(DbfGridTableBase* grid_base, wxWindow *parent,
				  const wxWindowID id = -1,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = 0,
				  const int x_col_id = wxNOT_FOUND,
				  const int y_col_id = wxNOT_FOUND,
				  const int z_col_id = wxNOT_FOUND);
	virtual ~C3DPlotCanvas();
	
	C3DPlotFrame* pa;

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnMouse( wxMouseEvent& event );
    void InitGL(void);
   
	bool isInit;

	float m_ClearColorBlue;
	float m_ClearColorGreen;
	float m_ClearColorRed;
	
	int selection_mode;
	int last[2];
	bool m_bRButton;
	bool m_bLButton;
	int height, width;
	double bb_min[3], bb_max[3];

	Arcball* ball;
	double *world_x, *world_y, *world_z;
	double *Raw_x, *Raw_y, *Raw_z;

	double xMin, xMax, yMin, yMax, zMin, zMax;

	double xs, xp, ys, yp, zs, zp;

	void UpdateViewsDlg();
	bool ShowFirst;
	void UpdateSelect();
	bool m_d;
	bool m_z;
	bool m_y;
	bool m_x;
	bool b_select;
	bool m_brush;
	void SelectByRect();
	void NormalizeData();
	void RenderScene();
	void apply_camera();
	void end_redraw();
	void begin_redraw();

	wxPoint select_start;
	wxPoint select_end;
	bool bSelect;
	bool isMe;
	C3DControlPan *m_dlg;
	int n_obs;

	DECLARE_EVENT_TABLE()

};



class C3DPlotFrame: public TemplateFrame
{
public:
	C3DPlotCanvas *canvas;
	C3DControlPan *control;

	C3DPlotFrame(wxFrame *parent, Project* project,
				 const wxString& title, const wxPoint& pos,
				 const wxSize& size, const long style,
				 const int x_col_id,
				 const int y_col_id,
				 const int z_col_id);
	virtual ~C3DPlotFrame();	
    
	wxSplitterWindow* m_splitter;

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	
	DECLARE_EVENT_TABLE()
};




#endif

