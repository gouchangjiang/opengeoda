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

#ifndef __GEODA_CENTER_LISA_BOX_VIEW_H__
#define __GEODA_CENTER_LISA_BOX_VIEW_H__

class GalWeight;
#include <vector>

class LisaBoxCanvas : public TemplateCanvas
{
public:
    LisaBoxCanvas(wxWindow* parent,
				  double* Data1,
				  double* Data2, // only not-null when isMultivariateLISA true
				  GalWeight* gal,
				  bool isMultivariateLISA,
				  bool isMoranEBRate,
				  const wxPoint& pos, const wxSize& size);
	virtual ~LisaBoxCanvas();
    virtual void OnDraw(wxDC& dc);

    void OnEvent(wxMouseEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnSize(wxSizeEvent& event);

	virtual int	SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC);
    virtual void CheckSize();
	virtual void Draw(wxDC* pDC);

protected:
	int		circRadius;
    double	Density;
	double	Upper;
	double	Lower;
    int		mainBox; // pixel width of box

	int		Q1Ind, Q3Ind;
	double  MinVal, MaxVal, MedianVal, Q1Val, Q3Val, IQR, Mean;
	double	UpperWhisker, LowerWhisker;
	int		MedianIndex;	
	
	double*	RawData;
	double*	RRawData;
    int*	Index;	
    int*	Invert;
	int*	mapper;
	int*	inv_mapper;
	
	wxString FieldName;
	
	bool isMoranEBRate;

public:
    void CheckHinge(bool hinge3);
    void DrawBox(wxDC* pDC);
    void DrawRectangle(wxDC* dc, int left, int top, int right, int bottom,
					   const wxColour color, bool frameOnly);
    int FindProxy(const double Proxy);
    void DrawPoint(wxDC* pDC, const int elt);
    void DrawMedian(wxDC* pDC);
    void UpdateSizeOption();
    bool Init();

	std::vector<bool> has_neighbors;
	int gcObs;  // keeps track of the number of displayed observations.  This
				// can be less than gObservation when there are isolates
				// in the weights file.

	bool hinge3;

	void OnOptionsHinge15();
	void OnOptionsHinge30();

    DECLARE_EVENT_TABLE()
};

class LisaBoxFrame: public TemplateFrame
{
public:
    LisaBoxCanvas *canvas;
    LisaBoxFrame(wxFrame *parent, Project* project,
				 double* Data1,
				 double* Data2, // only not-null when isMultivariateLISA true
				 GalWeight* gal,
				 bool isMultivariateLISA,
				 bool isMoranEBRate,
				 const wxString& title,
				 const wxPoint& pos, const wxSize& size, const long style);
    virtual ~LisaBoxFrame();

	void Update();

	void OnQuit(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	void UpdateMenuCheckMarks(wxMenu* menu);
	void UpdateMenuBarCheckMarks(wxMenuBar* menuBar);
	virtual void MapMenus();

	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif
