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

#ifndef __GEODA_CENTER_OPENGEODA_H__
#define __GEODA_CENTER_OPENGEODA_H__

#include <wx/docview.h>
#include <wx/clipbrd.h>
#include <wx/colordlg.h>
#include <wx/app.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/filename.h>
#include "Project.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"  // Temporarily for HiddenCanvas
#include <string>
#include <list>
#include <wx/toolbar.h>
#include "ShapeOperations/GalWeight.h"

// Forward Declarations
class TemplateCanvas;
class HiddenCanvas;
class HiddenFrame;
class ProgressDlg;

/**
 * Main appilcation class.
 */
class MyApp: public wxApp
{
public:
	//MyApp(void);
	virtual bool OnInit(void);
	virtual int OnExit(void);
	virtual void OnFatalException(void);
	//void OnTimer(wxTimerEvent& event);
private:
	//static wxTimer* timer;
	//DECLARE_EVENT_TABLE()
};

DECLARE_APP(MyApp)

/**
 * Main toolbar frame.
 */
class MyFrame: public wxFrame
{
public:
	MyFrame(const wxString& title,
			const wxPoint& pos, const wxSize& size, long style);
	virtual ~MyFrame();
	
	void EnableTool(const wxString& id_str, bool enable);
	void EnableTool(int xrc_id, bool enable);
	GalWeight* GetGal();

	void OnKeyEvent(wxKeyEvent& event);
	void OnToolOpenNewTable(wxCommandEvent& event);
	void OnOpenNewTable();
	/** Opens a new SHP file and initializes many global variables. */
	void OnOpenShapefile(wxCommandEvent& event);
	void OnOpenTableOnly(wxCommandEvent& event);
	void OnOpenSpTmShapefile(wxCommandEvent& event);
	void OnOpenSpTmTableOnly(wxCommandEvent& event);
	void OpenProject(bool table_only, bool space_time);
	bool OnCloseMap(bool ignore_unsaved_changes = false);
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnCloseAll(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& WXUNUSED(event));

	void OnSelectWithRect(wxCommandEvent& event);
	void OnSelectWithCircle(wxCommandEvent& event);
	void OnSelectWithLine(wxCommandEvent& event);
	void OnSelectionMode(wxCommandEvent& event);
	void OnFitToWindowMode(wxCommandEvent& event);
	void OnFixedAspectRatioMode(wxCommandEvent& event);
	void OnZoomMode(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	void OnPanMode(wxCommandEvent& event);
	void OnPrintCanvasState(wxCommandEvent& event);
	
	void OnSaveCanvasImageAs(wxCommandEvent& event);
	void OnSaveSelectedToColumn(wxCommandEvent& event);
	void OnCanvasBackgroundColor(wxCommandEvent& event);
	void OnLegendBackgroundColor(wxCommandEvent& event);
	void OnSelectableFillColor(wxCommandEvent& event);
	void OnSelectableOutlineColor(wxCommandEvent& event);
	void OnSelectableOutlineVisible(wxCommandEvent& event);
	void OnHighlightColor(wxCommandEvent& event);
	
	void OnSetDefaultVariableSettings(wxCommandEvent& WXUNUSED(event));
	void OnCopyImageToClipboard(wxCommandEvent& event);
	void OnCopyLegendToClipboard(wxCommandEvent& event);
	
	void OnToolsWeightsOpen(wxCommandEvent& event);  
	void OnToolsWeightsCreate(wxCommandEvent& event);
	void OnToolsWeightsChar(wxCommandEvent& event);

	void OnMapChoices(wxCommandEvent& event);
	void OnOpenChoices(wxCommandEvent& event);
	
	void OnShapePointsToPolygons(wxCommandEvent& event);
	void OnShapePolygonsToCentroids(wxCommandEvent& event);
	void OnShapePolygonsToMeanCenters(wxCommandEvent& event);
	void OnShapePointsFromDBF(wxCommandEvent& event);
	void OnShapePointsFromASCII(wxCommandEvent& event);
	void OnShapePolygonsFromGrid(wxCommandEvent& event);
	void OnShapePolygonsFromBoundary(wxCommandEvent& event);
	void OnShapeToBoundary(wxCommandEvent& event);

	void OnToolsDataExportSpaceStat(wxCommandEvent& event);
	void OnExportDataToASCII(wxCommandEvent& event);
	void OnToolsDataExportMeanCenters(wxCommandEvent& event);
	void OnToolsDataExportCentroids(wxCommandEvent& event);

	void OnShowTimeChooser(wxCommandEvent& event);
	void OnSpaceTimeTool(wxCommandEvent& event);
	void OnMoveSelectedToTop(wxCommandEvent& event);
	void OnClearSelection(wxCommandEvent& event);
	void OnRangeSelection(wxCommandEvent& event);
	void OnFieldCalculation(wxCommandEvent& event);
	void OnAddCol(wxCommandEvent& event);
	void OnDeleteCol(wxCommandEvent& event);
	void OnEditFieldProperties(wxCommandEvent& event);
	void OnMergeTableData(wxCommandEvent& event);
	void OnSaveProject(wxCommandEvent& event);
	void OnSaveAsProject(wxCommandEvent& event);
	void OnExportToCsvFile(wxCommandEvent& event);
	bool SaveTableSpace();
	bool SaveTableSpaceTime();
	void OnGeneratePointShpFile(wxCommandEvent& event);
	
	void OnRegressionClassic(wxCommandEvent& event);

	void OnShowCartogramMap(wxCommandEvent& event );
	void OnMoreIter100(wxCommandEvent& event);
	void OnMoreIter500(wxCommandEvent& event);
	void OnMoreIter1000(wxCommandEvent& event);
	void OnMapMovieSingle(wxCommandEvent& event );
	void OnMapMovieCumul(wxCommandEvent& event ); 

	void OnExploreHist(wxCommandEvent& event);
	void OnExploreScatterplot(wxCommandEvent& event);
	void OnExploreScatterNewPlot(wxCommandEvent& event);
	void OnExploreBubbleChart(wxCommandEvent& event);
	void OnExploreTestMap(wxCommandEvent& event);
	void OnExploreTestTable(wxCommandEvent& event);
	void OnExploreBox(wxCommandEvent& event);
	void OnExploreNewBox(wxCommandEvent& event);
	void OnExplorePCP(wxCommandEvent& event);
	void OnExplore3DP(wxCommandEvent& event);
	void OnExploreCC(wxCommandEvent& event);

	void OnOpenMSPL(wxCommandEvent& event);
	void OnOpenGMoran(wxCommandEvent& event);
	void OnOpenMoranEB(wxCommandEvent& event);
	void OnOpenUniLisa(wxCommandEvent& event);
	void OnOpenMultiLisa(wxCommandEvent& event);
	void OnOpenLisaEB(wxCommandEvent& event);
	void OnOpenGetisOrd(wxCommandEvent& event);

	void OnOpenThemelessMap(wxCommandEvent& event);
	void OnThemelessMap(wxCommandEvent& event);
	void OnOpenQuantile(wxCommandEvent& event);
	void OnQuantile(wxCommandEvent& event);
	void OnOpenPercentile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnOpenHinge15(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnOpenHinge30(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnOpenStddev(wxCommandEvent& event);
	void OnStddev(wxCommandEvent& event);
	void OnOpenNaturalBreaks(wxCommandEvent& event);
	void OnNaturalBreaks(wxCommandEvent& event);
	void OnOpenEqualIntervals(wxCommandEvent& event);
	void OnEqualIntervals(wxCommandEvent& event);
	void OnOpenUniqueValues(wxCommandEvent& event);
	void OnUniqueValues(wxCommandEvent& event);
	void OnSaveCategories(wxCommandEvent& event);
	
	void OnOpenRawrate(wxCommandEvent& event);
	void OnRawrate(wxCommandEvent& event);
	void OnOpenExcessrisk(wxCommandEvent& event);
	void OnExcessrisk(wxCommandEvent& event);
	void OnOpenEmpiricalBayes(wxCommandEvent& event);
	void OnEmpiricalBayes(wxCommandEvent& event);
	void OnOpenSpatialRate(wxCommandEvent& event);
	void OnSpatialRate(wxCommandEvent& event);
	void OnOpenSpatialEmpiricalBayes(wxCommandEvent& event);
	void OnSpatialEmpiricalBayes(wxCommandEvent& event);
	
	void OnSaveResults(wxCommandEvent& event);
	
	void OnHistogramIntervals(wxCommandEvent& event);
	
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);
	
	void OnSaveMoranI(wxCommandEvent& event);
	
	void OnSigFilter05(wxCommandEvent& event);
	void OnSigFilter01(wxCommandEvent& event);
	void OnSigFilter001(wxCommandEvent& event);
	void OnSigFilter0001(wxCommandEvent& event);
	
	void OnSaveGetisOrd(wxCommandEvent& event);
	void OnSaveLisa(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	void OnAddNeighborsToSelection(wxCommandEvent& event);
	
	void OnAddMeanCenters(wxCommandEvent& event);
	void OnAddCentroids(wxCommandEvent& event);
	void OnDisplayMeanCenters(wxCommandEvent& event);
	void OnDisplayCentroids(wxCommandEvent& event);
	
	// ScatterPlot and PCP specific callbacks
	void OnViewStandardizedData(wxCommandEvent& event);
	void OnViewOriginalData(wxCommandEvent& event);
	// ScatterPlot specific callbacks
	void OnViewRegressionSelectedExcluded(wxCommandEvent& event);
	void OnViewRegressionSelected(wxCommandEvent& event);
	void OnDisplayStatistics(wxCommandEvent& event);
	void OnShowAxesThroughOrigin(wxCommandEvent& event);
	// BoxPlot and Histogram specific callback
	void OnShowAxes(wxCommandEvent& event);
	
	void OnTimeSyncVariable(int var_index);
	void OnTimeSyncVariable1(wxCommandEvent& event);
	void OnTimeSyncVariable2(wxCommandEvent& event);
	void OnTimeSyncVariable3(wxCommandEvent& event);
	void OnTimeSyncVariable4(wxCommandEvent& event);
	
	void OnFixedScaleVariable(int var_index);
	void OnFixedScaleVariable1(wxCommandEvent& event);
	void OnFixedScaleVariable2(wxCommandEvent& event);
	void OnFixedScaleVariable3(wxCommandEvent& event);
	void OnFixedScaleVariable4(wxCommandEvent& event);
	
	void OnPlotsPerView(int plots_per_view);
	void OnPlotsPerView1(wxCommandEvent& event);
	void OnPlotsPerView2(wxCommandEvent& event);
	void OnPlotsPerView3(wxCommandEvent& event);
	void OnPlotsPerView4(wxCommandEvent& event);
	void OnPlotsPerView5(wxCommandEvent& event);
	void OnPlotsPerView6(wxCommandEvent& event);
	void OnPlotsPerView7(wxCommandEvent& event);
	void OnPlotsPerView8(wxCommandEvent& event);
	void OnPlotsPerView9(wxCommandEvent& event);
	void OnPlotsPerView10(wxCommandEvent& event);
	void OnPlotsPerViewOther(wxCommandEvent& event);
	void OnPlotsPerViewAll(wxCommandEvent& event);
	
	void OnDisplayStatusBar(wxCommandEvent& event);
	
	void OnHelpAbout(wxCommandEvent& event);

	void DisplayRegression(const wxString dump);

	void UpdateWholeView(wxFrame* caller);
	void UpdateToolbarAndMenus();
	void SetMenusToDefault();

	static Project* project_p;
	static Project* GetProject() { return projectOpen ? project_p : 0; }
	static HiddenFrame* hidden_frame;
		
	// Getter/Setter methods
	static bool IsProjectOpen();
	static void SetProjectOpen(bool open);
	static MyFrame* theFrame;
	
	static std::list<wxToolBar*> toolbar_list;
private:
	static bool projectOpen;
	DECLARE_EVENT_TABLE()
};

/**
 * This is used for the Conditional Plot View.
 */
class Conditionable
{
public:
	Conditionable(bool conditional_view, int num_obs);
	virtual ~Conditionable();

	bool isConditional;
	int cWhere;
	
	int num_obs;
	bool* conditionFlag;
	int numCondObs; // number of observations in conditionalFlag set to true
	
	void UpdateCondition(int *flags);
	static int cLocator;
	static int cViewType;  //1-MapView. 2-Boxplot, 3-Histogram 4-Scatter Plot
private:
	Conditionable();
};


/** 
 This canvas will stay hidden at all times.  It serves as
 an intermediary between HighlightState and Selection classes.
 */
class HiddenCanvas : public TemplateCanvas {
	DECLARE_CLASS(HiddenCanvas)
public:
	HiddenCanvas(wxWindow *parent, const wxPoint& pos,
				  const wxSize& size);
	virtual ~HiddenCanvas();

	void OnPaint(wxPaintEvent& event);
	
	/** implementation of HighlightStateObserver interface. */
	virtual void update(HighlightState* o);
protected:
	
	DECLARE_EVENT_TABLE()
};


/** 
 This frame will stay hidden at all times.  It serves as
 an intermediary between HighlightState and Selection classes. 
 */
class HiddenFrame : public TemplateFrame {
	DECLARE_CLASS(HiddenFrame)
public:
	HiddenFrame(wxFrame *parent, Project* project, const wxString& title,
				 const wxPoint& pos, const wxSize& size,
				 const long style);
	virtual ~HiddenFrame();
protected:
	HiddenCanvas* canvas;
	
public:
	virtual void Update(); // old style

private:

};


#endif

