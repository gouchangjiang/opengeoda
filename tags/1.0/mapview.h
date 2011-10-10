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

#ifndef __GEODA_CENTER_MAPVIEW_H__
#define __GEODA_CENTER_MAPVIEW_H__

#include <vector>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "OpenGeoDa.h"
#include "ShapeOperations/shp2cnt.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"
#include "TemplateLegend.h"
#include <fstream>

const int MAX_CATEGORY = 20;

const int SELECTION_RECT = 0;
const int SELECTION_LINE = 1;
const int SELECTION_CIRCLE = 2;

const int MODE_SELECT = 0;
const int MODE_ZOOMIN = 1;
const int MODE_ZOOMOUT = 2;
const int MODE_PANNING = 3;

extern long ReadBig(std::ifstream &input);
extern int gObservation;
extern MyFrame *frame;

extern Selection gSelection;
extern wxString gCompleteFileName;

extern bool m_VarDefault;
extern wxString	m_gVar1;
extern wxString	m_gVar2;
extern double* m_gX;
extern double* m_gY;

typedef int* pInt;

class MapSortElement // : public SortElement
{
	public:
	double value;
	double x;
	double y;
	int recordIndex;

public : 
	MapSortElement(double v, int i); 
	MapSortElement() {}; 
	bool operator > (MapSortElement& a);
	bool operator < (MapSortElement& a);
	MapSortElement& operator= (const MapSortElement& a);
};

class MapFrame;
class DbfGridTableBase;

class MapCanvas : public TemplateCanvas
{
public:
	MapCanvas(DbfGridTableBase* grid_base,
			  const wxString& fullPathShapefileName,
			  wxWindow *parent,
			  const wxPoint& pos, const wxSize& size,
			  ProgressDlg* p_dlg = 0);
	void OnPaint(wxPaintEvent& event);

	virtual ~MapCanvas();

	wxString fullPathShapefileName;
	DbfGridTableBase* grid_base;
	int zoomLevel;
	bool isLisaMap;
	bool isGetisOrdMap;
	bool isRedraw;

	void OnEvent(wxMouseEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnMapLButtonDown(wxMouseEvent& event, wxPoint& point); 
	void OnMapLButtonUp(wxMouseEvent& event, wxPoint& point); 
	void OnMapMouseMove(wxMouseEvent& event, wxPoint& point); 

    void MapDrawSelectionOutline();
    void MapEraseSelectionOutline();
	void ClearMapFrame();
	void MapReSizeSelection(wxMouseEvent& event, wxPoint& point);

	void ChoroplethMapUpdate(int select);
	void UserConfigUpdate();
	void OnSaveRates(wxString title);

	bool LineToLine(wxRealPoint p1, wxRealPoint p2, float a, float b, float c,
					wxRealPoint pp3, wxRealPoint pp4);
	bool pointInPolygon(int polySides, wxRealPoint poly[], int xx, int yy);


	int m_nSaveRateValue;
	int Hinge;
	int IDChoropleth;
	int IDSmoothType;   // This takes on a subset of IDChoropleth values
	int IDMapType;  // This takes on a subset of IDChoropleth values 
	int key_flag; 


	int ThemeID[9];	
	wxString m_fieldName, m_layerName, m_idName;  

	std::vector<MapSortElement> fieldValue; 

	void QuickSortFieldValT(std::vector<MapSortElement>& array, int low, int high);
	void SortT(MapSortElement*, MapSortElement*, MapSortElement*);
	void SwapT(MapSortElement*, MapSortElement*);
	void ComputeFrequency(const std::vector<double>& b,
						  int nC, std::vector<long>& freq);
	void ComputeFrequencyUnique(const std::vector<double>& b,
								int nC, std::vector<long>& freq);
	void DisplayMsg(int* lst, int n);

	int numClasses; 

	void PickColorSet(short coltype, short ncolor, bool reversed);
	void PickBrightSet(short ncolor, bool reversed);
	void DrawPolygons(wxDC* dc);
	void DrawSinglePolygon(wxDC* dc, int rec, bool highlight = false);

	virtual void Selection(wxDC* pDC);
	virtual void SelectByPoint(wxMouseEvent& event);
	virtual int SelectByRect(wxMouseEvent& event);
	virtual void CheckSize();
	virtual void Convert_wxPoint2wxRealPoint();
	virtual void Draw(wxDC* pDC);

	int SelectByType(wxMouseEvent& event);

	bool isPanning;
	int m_type;  // Map Type
	int work_mode;
	int sel_mode;
        
	double xMin, yMin, xMax, yMax;
	long n_total; // total number of polygon points.
	std::vector<int> idx; //size gObservation
	std::vector<double> xs; // size n_total
	std::vector<double> ys; // size n_total
	
	std::vector<wxPoint> location; // size n_total
	std::vector<wxPoint> center_location; //size gObservation
	std::vector<wxRealPoint> Wlocation; // size n_total
	std::vector<wxRealPoint> Wcenter_location; //size gObservation
	
	std::vector<double> cloc_x;  //size gObservation
	std::vector<double> cloc_y;  //size gObservation
	int Width, Height, Left, Right, Top, Bottom;
	bool bLink;
	bool IsObjSelected;

	int StarSize;

	std::vector<int> n_parts_per_cell; //size gObservation
	pInt* ppc; //size gObservation

	std::vector<double> breakVal;
	std::vector<double> breakVal_reserve;
	
	std::vector<wxColour> Color;
	std::vector<wxColor> colors;
	std::vector<int> c_id; //size gObservation
	
	std::vector<bool> selected; // size gObservation // MMM Selection bitvector
	
	int MapFrameSignificanceFilter;
	
	class MapFrame* myP;

	double max_x, max_y, min_x, min_y;
	
private:
	DECLARE_EVENT_TABLE()
};


class MapLegend : public TemplateLegend
{
public:
	MapLegend(wxWindow *parent, const wxPoint& pos, const wxSize& size);
	virtual ~MapLegend();
	virtual void OnDraw(wxDC& dc);

	void OnEvent(wxMouseEvent& event);

	int px, py, m_w, m_l; 

	int count_color;
	wxColour color;
	int colorSelection;

	int d_rect; 
	class MapFrame* myP;

	DECLARE_EVENT_TABLE()
};

class MapFrame: public TemplateFrame
{
public:
	MapCanvas *canvas;
	MapFrame(const wxString& fullPathShapefileName,
			 wxFrame *parent, Project* project,
			 const wxString& title, const wxPoint& pos, const wxSize& size,
			 const long style, ProgressDlg* p_dlg = 0);
	virtual ~MapFrame();

	wxScrolledWindow *m_left, *m_right;
	wxSplitterWindow* m_splitter;

	void Update();
	
	void OnActivate(wxActivateEvent& event);
	void UpdateMenuCheckMarks(wxMenu* menu);
	void UpdateMenuBarCheckMarks(wxMenuBar* menuBar);
	virtual void MapMenus();

	void OnQuit(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMove(wxMoveEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);

	void OnQuantile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnStddev(wxCommandEvent& event);
	void OnNaturalBreak(wxCommandEvent& event);
	void OnUniqueValue(wxCommandEvent& event);
	void OnEqualInterval(wxCommandEvent& event);

	void OnRawrate(wxCommandEvent& event);
	void OnExcessrisk(wxCommandEvent& event);
	void OnBayes(wxCommandEvent& event);
	void OnSmoother(wxCommandEvent& event);
	void OnEmpiricalBayes(wxCommandEvent& event);
	void OnSaveResults(wxCommandEvent& event);
	void OnReset(wxCommandEvent& event);

	void OnMapSelectionMode(wxCommandEvent& event);
	void OnMapSelectWithRect(wxCommandEvent& event);
	void OnMapSelectWithLine(wxCommandEvent& event);
	void OnMapSelectWithCircle(wxCommandEvent& event);

	void OnMapZoomIn(wxCommandEvent& event);
	void OnMapZoomOut(wxCommandEvent& event);
	void OnMapFitToWindowMode(wxCommandEvent& event);

	void OnMapPanMode(wxCommandEvent& event);

    void OnMapSelectableOutlineVisible(wxCommandEvent& event);
	void OnMapSelectableFillColor(wxCommandEvent& event);
	void OnMapHighlightColor(wxCommandEvent& event);

	void OnAddMeanCenters(wxCommandEvent& event);
	void OnAddCentroids(wxCommandEvent& event);

	void UpdateMenuBar();

	int numBreaks;
	int numBreaks2;
	wxString m_str[MAX_CATEGORY];
	wxString m_str2[MAX_CATEGORY];

	wxString title2;
	void SetTitle2(wxString title) { title2 = title; }
	wxString GetTitle2() { return title2; }

	DECLARE_EVENT_TABLE()
};

#include "DialogTools/MovieControlPan.h"

class MapMovieFrame;
class CMovieControlPan;
class MapMovieCanvas : public TemplateCanvas, public Conditionable
{
public:
	MapMovieCanvas(wxWindow *parent=NULL, const wxPoint& pos=wxDefaultPosition,
				   const wxSize& size=wxDefaultSize,
				   bool conditional_view = false);
	virtual ~MapMovieCanvas();
	void OnPaint(wxPaintEvent& event);
	void AddMap(wxString& path);
	void Remove();

	void OnSize(wxSizeEvent& event);
	void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	
	bool isThereMap;

	void CheckSize();

	int m_type;  // Map Type
	double xMin, yMin, xMax, yMax;
	long n_total;
	std::vector<int> idx;
	std::vector<double> xs;
	std::vector<double> ys;
	std::vector<wxPoint> location;
	int Width, Height, Left, Right, Top, Bottom;
	double* RawData;
	int* hidx;
	bool bLink;

	int obs;
	int StarSize;

	std::vector<int> n_parts_per_cell;
	pInt* ppc;

	MapMovieFrame* myP;

	int current_frame;
	int type;

	void InvalidatePolygon(int id, bool s);
	void TimerCall();

	void UpdateCondition(int *flags);
    void Selection(wxDC* pDC);

	DECLARE_EVENT_TABLE()
};

class MapMovieTimer: public wxTimer
{
public:
	MapMovieTimer():wxTimer() {}

	MapMovieFrame* myP;
	virtual void Notify();
};

class MapMovieFrame: public TemplateFrame
{
public:
	MapMovieCanvas *canvas;
	CMovieControlPan *control;

	MapMovieTimer *timer;
	
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	void UpdateMenuCheckMarks(wxMenu* menu);

	MapMovieFrame(wxFrame *parent, Project* project, int type,
				  const wxString& title, const wxPoint& pos,
				  const wxSize& size, const long style);
	virtual ~MapMovieFrame();
    
	wxSplitterWindow* m_splitter;
	wxSplitterWindow* m_splitterMap;

	DECLARE_EVENT_TABLE()
};

#endif

