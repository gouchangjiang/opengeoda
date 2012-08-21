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

#ifndef __GEODA_CENTER_HIST_VIEW_H__
#define __GEODA_CENTER_HIST_VIEW_H__

#include <vector>

class HistCanvas : public TemplateCanvas, public Conditionable
{
public:
    HistCanvas(bool ViewCC, wxWindow *parent,
			   double* v1, int num_obs, const wxString& v1_name,
			   const wxPoint& pos, const wxSize& size,
			   bool conditional_view = false,
			   wxString hist_weight_file = wxEmptyString,
			   long* hist_weight_freq = 0);
	virtual ~HistCanvas();
	
    virtual void OnDraw(wxDC& dc);
    void OnEvent(wxMouseEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
    void OnLButtonDblClk(wxMouseEvent& event, wxPoint& point); 
    void OnLButtonDown(wxMouseEvent& event, wxPoint& point); 
    void OnLButtonUp(wxMouseEvent& event, wxPoint& point); 
    void OnMouseMove(wxMouseEvent& event, wxPoint& point); 
    void OnSize(wxSizeEvent& event);

	virtual int	SelectByRect(wxMouseEvent& event);
    virtual void SelectByPoint(wxMouseEvent& event);
    virtual void Selection(wxDC* pDC = NULL);
    virtual void CheckSize();
	virtual void Draw(wxDC* pDC);
	wxColour GetColor(float data, bool IntData);
	void DrawRectangle(wxDC *pDC,int left, int top, int right, int bottom,
					   const wxColour color, const bool frameOnly);
	void DrawLegend(wxDC *pDC);
	static void MakeHistoLegend(char ** Legend, double* BreakVals,
								const int* counts, const double minData,
								const double rangeData,
								const int ranges, const bool intData);
    double* RawData;
    double Density;
	double* BreakVals;
    int Ranges;
	int MaxCount;
    int* Counts;
    int* Selected;
    int* Grouping;
    int* Groups;
	bool m_ViewCC;
	int gcObs; // number of objects if m_ViewCC == true
    unsigned char* Range;
    char** LegendItems;
    wxColour* RangeColor;
	double minData, maxData;
	int num_obs;

	void UpdateBins();
protected:
	bool	SetRanges;
	bool    intData; // is integer data?
	bool	isWeightsHistogram;  // true if Weights Characteristics
	std::vector<int> isolates; // list of isolates when Weights Histogram
	wxSize isolates_button_size;
	
	double RangeData;
	wxString FieldName;

// Attributes
public:
    enum { minGroups = 1, maxGroups = 100};
    void	DrawHisto(wxDC *pDC);
    bool	Init();
    bool	hasLegend()  const  {  return true;  };
    int		ranges()  const     {  return Ranges;  };
    void	clear();
    bool	grouping();

	void HistogramIntervals();

	void UpdateCondition(int *flags);

    DECLARE_EVENT_TABLE()
};

class HistFrame: public TemplateFrame
{
public:
    HistCanvas *canvas;
    HistFrame(wxFrame *parent,
			  double* v1, int num_obs, const wxString& v1_name,
			  Project* project, const wxString& title,
			  const wxPoint& pos, const wxSize& size, const long style,
			  wxString hist_weight_file = wxEmptyString,
			  long* hist_weight_freq = 0);
    virtual ~HistFrame();

	void Update();

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();

    void OnQuit(wxCommandEvent& event);
	void OnHistogramIntervals(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};




#endif

