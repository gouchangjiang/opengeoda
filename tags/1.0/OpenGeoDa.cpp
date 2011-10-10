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

#undef check // undefine needed for Xcode compilation and Boost.Geometry
#include <iostream>
#include <sstream>
#include <string>

#include <boost/foreach.hpp>

#include <wx/wxprec.h>
#include <wx/valtext.h>
#include <wx/image.h>
#include <wx/dcsvg.h>		// SVG DC
#include <wx/dcps.h>		// PostScript DC
#include <wx/dcmemory.h>	// Memory DC
#include <wx/xrc/xmlres.h>	// XRC XML resouces
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/app.h>
#include <wx/sysopt.h>
#include <wx/position.h>
#include <wx/progdlg.h>
#include <wx/filefn.h> // for wxCopyFile and wxFileExists
#include <wx/msgdlg.h>

#include "DialogTools/FieldNewCalcSheetDlg.h"
#include "DataViewer/DbfGridTableBase.h"
#include "DataViewer/DataViewerAddColDlg.h"
#include "DataViewer/DataViewerDeleteColDlg.h"
#include "DataViewer/DataViewerEditFieldPropertiesDlg.h"
#include "DataViewer/MergeTableDlg.h"
#include "DialogTools/RangeSelectionDlg.h"
#include "ShapeOperations/shp.h"
#include "ShapeOperations/shp2cnt.h"
#include "ShapeOperations/ShpFile.h"
//#include "ShapeOperations/ShapeUtils.h"
#include "FramesManager.h"
#include "OpenGeoDa.h"
#include "NewTableViewer.h"
#include "TemplateCanvas.h"
#include "Thiessen/VorDataType.h"
#include "DialogTools/Statistics.h"
#include "DialogTools/MapQuantileDlg.h"
#include "DialogTools/SaveSelectionDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionTitleDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "Explore/GetisOrdMapView.h"
#include "DialogTools/RateSmootherDlg.h"
#include "DialogTools/LisaWhat2OpenDlg.h"
#include "Explore/LisaBoxView.h"
#include "Explore/LisaMapView.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/MoranGView.h"
#include "Explore/MoranScatterPlotView.h"

#include "mapview.h"
#include "Explore/CartogramView.h"
#include "Explore/ScatterPlotView.h"
//#include "Explore/ScatterNewPlotView.h"
#include "DialogTools/ScatterPlotVarsDlg.h"
//#include "Generic/TestMapView.h"
//#include "Generic/TestTableView.h"
//#include "Generic/TestScrollWinView.h"
#include "Explore/PCPView.h"
#include "Explore/HistView.h"
#include "Explore/BoxPlotView.h"
#include "Explore/3DPlotView.h"
#include "Explore/ConditionalView.h"
#include "Regression/DiagnosticReport.h"
#include "DialogTools/RegressionDlg.h"
//#include "DialogTools/SpRegimesDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/ProgressDlg.h"
#include "DialogTools/GetisOrdChoiceDlg.h"
#include "Explore/GStatCoordinator.h"
#include "ShapeOperations/WeightsManager.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include "GeoDaConst.h"
#include "logger.h"

MyFrame* frame = 0;
MyFrame* MyFrame::theFrame = 0;

// Will go away once everthing is converted to using HighlightSate
extern GeoDaEventType gEvent;

// The following is defined in rc/MyAppResouces.cpp.  This file was
// compiled with:
/*
 wxrc dialogs.xrc menus.xrc toolbar.xrc \
   --cpp-code --output=MyAppResources.cpp --function=MyInitXmlResource
*/
// and combines all resouces file into single source file that is linked into
// the application binary.
extern void MyInitXmlResource();

bool MyFrame::table_only_proj = false;
bool MyFrame::projectOpen = false;

Selection gSelection;  // the currently selected objects, an array of bool
double* m_gX = 0;
double*	m_gY = 0;
wxString m_gVar1 = wxEmptyString;
wxString m_gVar2 = wxEmptyString;
wxString m_gVar3 = wxEmptyString;
wxString m_gVar1_default = wxEmptyString;
wxString m_gVar2_default = wxEmptyString;
int gObservation = 0;  // the number of observations

// Should be moved into Project
bool m_VarDefault = false;
wxString gCompleteFileName = wxEmptyString;
wxString gWeightTitle = wxEmptyString;

extern bool CheckDataValidity(int obs, double* x);  

void ggcvt(double d, int n, char* str) 
{
	int i = 0; // initial value
	long j = (int) floor(d);
	char r[2];

	if (d == 0) {
		str[0] = '0';
		str[1] = '\0';
		return; }
	if (n <= 0) {
		str[0] = '\0';
		return;
	}

	r[1] = '\0';
	if (d < 0) j = -1 * j;
	d = fabs(d) - fabs((double)j);
	GenUtils::longToString(j, str, 10);
	strcat(str, ".");
	if (j == 0) {
		for (i = strlen(str) - 2; i < n; i++) {
			d *= 10;
			r[0] = (int)floor(d) + '0';
			strcat(str, r);
			d -= floor(d);
		}
	} else {
		for (i = strlen(str) - 1; i < n; i++) {
			d *= 10;
			r[0] = (int)floor(d) + '0';
			strcat(str, r);
			d -= floor(d);
		}
	}
}

const int TIMER_ID = wxID_HIGHEST + 1;
const int ID_TEST_MAP_FRAME = wxID_HIGHEST + 10;
const int ID_TEST_TABLE_FRAME = wxID_HIGHEST + 11;

//Event table for MyApp
//BEGIN_EVENT_TABLE(MyApp, wxApp)
	//EVT_TIMER(TIMER_ID, MyApp::OnTimer)
//END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

//MyApp::MyApp(void)
//{
//	LOG_MSG("Entering MyApp::MyApp");
	//Don't call wxHandleFatalExceptions so that a core dump file will be
	//produced for debugging.
	//wxHandleFatalExceptions();
//	LOG_MSG("Exiting MyApp::MyApp");
//}

//wxTimer* MyApp::timer = NULL;

//void MyApp::OnTimer(wxTimerEvent& event)
//{
	// This method is called once every second
	//ExitMainLoop(); // This will force the program to exit immediately
//}

#include "rc/oGeoDaIcon-16x16.xpm"

bool MyApp::OnInit(void)
{
	LOG_MSG("Entering MyApp::OnInit");
	if (!wxApp::OnInit()) return false;

	GeoDaConst::init();
	
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
	wxXmlResource::Get()->InitAllHandlers();
	
    MyInitXmlResource();  // call the init function in MyAppResources.cpp	
	
	//int majorVsn = 0;
	//int minorVsn = 0;
	//wxGetOsVersion(&majorVsn, &minorVsn);
	//LOG_MSG(wxString::Format("OS Version: %d.%d", majorVsn, minorVsn));
	//LOG_MSG(wxString::Format("XP? %d, Vista? %d", GeneralWxUtils::isXP(),
	//        GeneralWxUtils::isVista()));
	
	int frameWidth = 800;
	int frameHeight = 80;
	if (GeneralWxUtils::isMac()) {
		frameWidth = 643;
		frameHeight = 46;
	}
	if (GeneralWxUtils::isWindows()) {
		// The default is assumed to be Vista family, but can check with
		//   GeneralWxUtils::isVista()
		frameWidth = 600;
		frameHeight = 76;
		// Override default in case XP family of OSes is detected
		if (GeneralWxUtils::isXP()) {
			frameWidth = 600;
			frameHeight = 76;
		}
	}
	if (GeneralWxUtils::isUnix()) {  // assumes GTK
		frameWidth = 748;
 		frameHeight = 84;
	}

	wxPoint appFramePos = wxDefaultPosition;
	if (GeneralWxUtils::isUnix() || GeneralWxUtils::isMac()) {
		appFramePos = wxPoint(80,60);
	}

	MyFrame::theFrame = new MyFrame("OpenGeoDa",
									appFramePos,
									wxSize(frameWidth, frameHeight),
									wxDEFAULT_FRAME_STYLE &
									~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
	frame = MyFrame::theFrame;
	
	MyFrame::theFrame->Show(true);
	SetTopWindow(MyFrame::theFrame);
	
	// The following code will insert a "Test Map Frame" menu item
	// under the file menu.  Comment this section out when not
	// testing.
	// BEGIN TestMapFrame TEST
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int fm_index = mb->FindMenu("File");
	wxMenu* fm = mb->GetMenu(fm_index);
//	fm->Append(ID_TEST_MAP_FRAME, "Test Map Frame",
//			   "Open a test frame");
//	GeneralWxUtils::EnableMenuItem(mb, "File",
//								   ID_TEST_MAP_FRAME, false);
//	fm->Append(ID_TEST_TABLE_FRAME, "Test Table",
//			   "Open a test table");
//	GeneralWxUtils::EnableMenuItem(mb, "File",
//								   ID_TEST_TABLE_FRAME, false);
	// END TestMapFrame TEST
	
	//TestScrollWinFrame *subframe =
	//new TestScrollWinFrame(NULL, "Test Scrolled Window Frame",
	//					   wxDefaultPosition, wxSize(500,600),
	//					   wxDEFAULT_FRAME_STYLE);
    //subframe->Show(true);
	
	// Create the timer and set check every 1000 milliseconds.
	//timer = new wxTimer(this, TIMER_ID);
	//timer->Start(1000);
	
	LOG_MSG("Exiting MyApp::OnInit");
	return true;
}

int MyApp::OnExit(void)
{
	LOG_MSG("Entering MyApp::OnExit");
	LOG_MSG("Exiting MyApp::OnExit");
	return 0;
}

void MyApp::OnFatalException()
{
	LOG_MSG("In MyApp::OnFatalException");
	wxMessageBox("Fatal Excption.  Program will likely close now.");
}


/*
 * This is the top-level window of the application.
 */

BEGIN_EVENT_TABLE(MyFrame, wxFrame)

	EVT_MENU(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnProjectOpen)
	EVT_TOOL(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnProjectOpen)
	EVT_BUTTON(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnProjectOpen)
	EVT_MENU(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_TOOL(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_BUTTON(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_MENU(XRCID("wxID_CLOSE"), MyFrame::OnMenuClose)
	EVT_MENU(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_TOOL(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_BUTTON(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_CLOSE(MyFrame::OnClose)
	EVT_MENU(XRCID("wxID_EXIT"), MyFrame::OnQuit)

	EVT_MENU(XRCID("ID_SELECT_WITH_RECT"), MyFrame::OnSelectWithRect)
	EVT_MENU(XRCID("ID_SELECT_WITH_CIRCLE"), MyFrame::OnSelectWithCircle)
	EVT_MENU(XRCID("ID_SELECT_WITH_LINE"), MyFrame::OnSelectWithLine)
	EVT_MENU(XRCID("ID_SELECTION_MODE"), MyFrame::OnSelectionMode)
	EVT_MENU(XRCID("ID_FIT_TO_WINDOW_MODE"), MyFrame::OnFitToWindowMode)
	// Fit-To-Window Mode
	EVT_MENU(XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
						MyFrame::OnFixedAspectRatioMode)
	EVT_MENU(XRCID("ID_ZOOM_MODE"), MyFrame::OnZoomMode)
	EVT_MENU(XRCID("ID_ZOOM_IN"), MyFrame::OnZoomIn)
	EVT_MENU(XRCID("ID_ZOOM_OUT"), MyFrame::OnZoomOut)
	EVT_MENU(XRCID("ID_PAN_MODE"), MyFrame::OnPanMode)
	// Print Canvas State to Log File.  Used for debugging.
	EVT_MENU(XRCID("ID_PRINT_CANVAS_STATE"), MyFrame::OnPrintCanvasState)

	EVT_MENU(XRCID("ID_SAVE_CANVAS_IMAGE_AS"), MyFrame::OnSaveCanvasImageAs)
	EVT_MENU(XRCID("ID_SAVE_SELECTED_TO_COLUMN"),
						MyFrame::OnSaveSelectedToColumn)
	EVT_MENU(XRCID("ID_CANVAS_BACKGROUND_COLOR"),
			 MyFrame::OnCanvasBackgroundColor)
	EVT_MENU(XRCID("ID_LEGEND_BACKGROUND_COLOR"),
		 MyFrame::OnLegendBackgroundColor)
	EVT_MENU(XRCID("ID_SELECTABLE_FILL_COLOR"),
		 MyFrame::OnSelectableFillColor)
	EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_COLOR"),
			 MyFrame::OnSelectableOutlineColor)
	EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
		 MyFrame::OnSelectableOutlineVisible)
	EVT_MENU(XRCID("ID_HIGHLIGHT_COLOR"), MyFrame::OnHighlightColor)

	// Former Edit menu items.
	EVT_TOOL(XRCID("ID_SHOW_CONVERTED_FEATURES"), MyFrame::OnNewMap)
	EVT_BUTTON(XRCID("ID_SHOW_CONVERTED_FEATURES"), MyFrame::OnNewMap)
	EVT_TOOL(XRCID("ID_NEW_MAP_WINDOW"), MyFrame::OnOpenMapwindow)
	EVT_BUTTON(XRCID("ID_NEW_MAP_WINDOW"), MyFrame::OnOpenMapwindow)
	EVT_MENU(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettingss)
	EVT_TOOL(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettingss)
	EVT_BUTTON(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettingss)
	EVT_MENU(XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
			 MyFrame::OnCopyImageToClipboard)
	EVT_MENU(XRCID("ID_COPY_LEGEND_TO_CLIPBOARD"),
			 MyFrame::OnCopyLegendToClipboard)

	EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)
	EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)
	EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)

	EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_CHAR"), MyFrame::OnToolsWeightsChar)
	EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_CHAR"), MyFrame::OnToolsWeightsChar)
	EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_CHAR"), MyFrame::OnToolsWeightsChar)

	EVT_TOOL(XRCID("ID_MAP_CHOICES"), MyFrame::OnMapChoices)

	EVT_MENU(XRCID("ID_SHAPE_POINTS_TO_POLYGONS"),
			 MyFrame::OnShapePointsToPolygons)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_TO_CENTROIDS"),
			 MyFrame::OnShapePolygonsToCentroids)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_TO_MEAN_CENTERS"),
			 MyFrame::OnShapePolygonsToMeanCenters)
	EVT_MENU(XRCID("ID_SHAPE_POINTS_FROM_DBF"),
			 MyFrame::OnShapePointsFromDBF)
	EVT_MENU(XRCID("ID_SHAPE_POINTS_FROM_ASCII"),
			 MyFrame::OnShapePointsFromASCII)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_GRID"),
			 MyFrame::OnShapePolygonsFromGrid)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_BOUNDARY"),
			 MyFrame::OnShapePolygonsFromBoundary)
	EVT_MENU(XRCID("ID_SHAPE_TO_BOUNDARY"), MyFrame::OnShapeToBoundary)
 
	EVT_MENU(XRCID("ID_EXPORT_DATA_TO_ASCII"),
			 MyFrame::OnExportDataToASCII)
	EVT_MENU(XRCID("ID_EXPORT_MEAN_CENTERS"),
			 MyFrame::OnToolsDataExportMeanCenters)
	EVT_TOOL(XRCID("ID_EXPORT_MEAN_CENTERS"),
			 MyFrame::OnToolsDataExportMeanCenters)
	EVT_BUTTON(XRCID("ID_EXPORT_MEAN_CENTERS"),
		 MyFrame::OnToolsDataExportMeanCenters)
	EVT_MENU(XRCID("ID_EXPORT_CENTROIDS"), MyFrame::OnToolsDataExportCentroids)
	EVT_TOOL(XRCID("ID_EXPORT_CENTROIDS"), MyFrame::OnToolsDataExportCentroids)
	EVT_BUTTON(XRCID("ID_EXPORT_CENTROIDS"),
			   MyFrame::OnToolsDataExportCentroids)

	// Table menu items
	EVT_MENU(XRCID("ID_NEW_TABLE_MOVE_SELECTED_TO_TOP"),
			 MyFrame::OnMoveSelectedToTop)
	EVT_MENU(XRCID("ID_NEW_TABLE_CLEAR_SELECTION"),
			 MyFrame::OnClearSelection)
	EVT_MENU(XRCID("ID_NEW_TABLE_RANGE_SELECTION"),
			 MyFrame::OnRangeSelection)
	EVT_MENU(XRCID("ID_NEW_TABLE_FIELD_CALCULATION"),
			 MyFrame::OnFieldCalculation)
	EVT_MENU(XRCID("ID_NEW_TABLE_ADD_COLUMN"), MyFrame::OnAddCol)
	EVT_MENU(XRCID("ID_NEW_TABLE_DELETE_COLUMN"), MyFrame::OnDeleteCol)
	EVT_MENU(XRCID("ID_NEW_TABLE_EDIT_FIELD_PROP"),
			 MyFrame::OnEditFieldProperties)
	EVT_MENU(XRCID("ID_NEW_TABLE_MERGE_TABLE_DATA"),
			 MyFrame::OnMergeTableData)
	EVT_MENU(XRCID("ID_NEW_TABLE_SAVE_COPY_OF_SHP_FILE"),
			 MyFrame::OnSaveCopyOfShpFile)
	EVT_MENU(XRCID("ID_NEW_TABLE_SAVE_COPY_OF_TABLE"),
			 MyFrame::OnSaveCopyOfTable)
	EVT_MENU(XRCID("ID_NEW_TABLE_SAVE"),
			 MyFrame::OnSaveTable)
	EVT_MENU(XRCID("ID_ADD_NEIGHBORS_TO_SELECTION"),
			 MyFrame::OnAddNeighborsToSelection)

	EVT_MENU(XRCID("ID_REGRESSION_CLASSIC"), MyFrame::OnRegressionClassic)
	//EVT_MENU(XRCID("ID_REGRESSION_SP_REGIMES"), MyFrame::OnSpatialRegimes)

	EVT_MENU(XRCID("ID_SHOW_CARTOGRAM_MAP"), MyFrame::OnShowCartogramMap)
	EVT_TOOL(XRCID("ID_SHOW_CARTOGRAM_MAP"), MyFrame::OnShowCartogramMap)
	EVT_BUTTON(XRCID("ID_SHOW_CARTOGRAM_MAP"), MyFrame::OnShowCartogramMap)
	EVT_MENU(XRCID("ID_CARTOGRAM_MOREITERATIONS_100"),
			 MyFrame::OnMoreIter100)
	EVT_MENU(XRCID("ID_CARTOGRAM_MOREITERATIONS_500"),
			 MyFrame::OnMoreIter500)
	EVT_MENU(XRCID("ID_CARTOGRAM_MOREITERATIONS_1000"),
			 MyFrame::OnMoreIter1000)
	EVT_MENU(XRCID("ID_OPTIONS_HINGE_15"), MyFrame::OnHinge15)
	EVT_MENU(XRCID("ID_OPTIONS_HINGE_30"), MyFrame::OnHinge30)

	EVT_MENU(XRCID("ID_THEMATICMAP_MAPMOVIE_SINGLE"), MyFrame::OnMapMovieSingle)
	// Formerly in XRC file:
	//<object class="tool" name="ID_THEMATICMAP_MAPMOVIE_SINGLE">
	//  <bitmap>ToolBarBitmaps_31.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_31_disabled.png</bitmap2>
	//  <tooltip>Map Movie (single selection)</tooltip>
	//</object>
	EVT_BUTTON(XRCID("ID_THEMATICMAP_MAPMOVIE_SINGLE"),
			   MyFrame::OnMapMovieSingle)
	EVT_MENU(XRCID("ID_MAPANALYSIS_MAPMOVIE"), MyFrame::OnMapMovieCumul)
	EVT_TOOL(XRCID("ID_MAPANALYSIS_MAPMOVIE"), MyFrame::OnMapMovieCumul)
	EVT_BUTTON(XRCID("ID_MAPANALYSIS_MAPMOVIE"), MyFrame::OnMapMovieCumul)

	EVT_MENU(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_TOOL(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_BUTTON(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_MENU(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterplot)
	EVT_MENU(XRCID("IDM_SCATTER_NEW_PLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_TOOL(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_BUTTON(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_MENU(ID_TEST_MAP_FRAME, MyFrame::OnExploreTestMap)
	EVT_MENU(ID_TEST_TABLE_FRAME, MyFrame::OnExploreTestTable)
	EVT_MENU(XRCID("IDM_BOX"), MyFrame::OnExploreBox)
	EVT_TOOL(XRCID("IDM_BOX"), MyFrame::OnExploreBox)
	EVT_BUTTON(XRCID("IDM_BOX"), MyFrame::OnExploreBox)
	EVT_MENU(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_TOOL(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_BUTTON(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_MENU(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	EVT_TOOL(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	EVT_BUTTON(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	EVT_MENU(XRCID("IDM_CC"), MyFrame::OnExploreCC)
	EVT_TOOL(XRCID("IDM_CC"), MyFrame::OnExploreCC)
	EVT_BUTTON(XRCID("IDM_CC"), MyFrame::OnExploreCC)

	EVT_TOOL(XRCID("IDM_NEW_TABLE"), MyFrame::OnToolOpenNewTable)
	EVT_BUTTON(XRCID("IDM_NEW_TABLE"), MyFrame::OnToolOpenNewTable)

	EVT_MENU(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_TOOL(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_BUTTON(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_MENU(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_TOOL(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_BUTTON(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_MENU(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_TOOL(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_BUTTON(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_MENU(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_TOOL(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_BUTTON(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_MENU(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_TOOL(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_BUTTON(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_MENU(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_TOOL(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_BUTTON(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_TOOL(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)
	EVT_BUTTON(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)
	EVT_MENU(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)

	EVT_MENU(XRCID("ID_HISTOGRAM_INTERVALS"), MyFrame::OnHistogramIntervals)

	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_99PERMUTATION"),
			 MyFrame::OnRan99Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_199PERMUTATION"),
			 MyFrame::OnRan199Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_499PERMUTATION"),
			 MyFrame::OnRan499Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_999PERMUTATION"),
			 MyFrame::OnRan999Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_OTHER"),
			 MyFrame::OnRanOtherPer)

	EVT_MENU(XRCID("ID_OPTION_ENVELOPESLOPES"), MyFrame::OnEnvelopeSlopes)
	EVT_MENU(XRCID("ID_SAVE_MORANI"), MyFrame::OnSaveMoranI)

	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_05"), MyFrame::OnSigFilter05)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_01"), MyFrame::OnSigFilter01)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_001"), MyFrame::OnSigFilter001)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_0001"), MyFrame::OnSigFilter0001)

	EVT_MENU(XRCID("ID_SAVE_GETIS_ORD"), MyFrame::OnSaveGetisOrd)
	EVT_MENU(XRCID("ID_SAVE_LISA"), MyFrame::OnSaveLisa)
	EVT_MENU(XRCID("ID_SELECT_CORES"), MyFrame::OnSelectCores)
	EVT_MENU(XRCID("ID_SELECT_NEIGHBORS_OF_CORES"),
			 MyFrame::OnSelectNeighborsOfCores)
	EVT_MENU(XRCID("ID_SELECT_CORES_AND_NEIGHBORS"),
			 MyFrame::OnSelectCoresAndNeighbors)

	EVT_MENU(XRCID("ID_MAP_ADDMEANCENTERS"), MyFrame::OnAddMeanCenters)
	EVT_MENU(XRCID("ID_MAP_ADDCENTROIDS"), MyFrame::OnAddCentroids)

	//EVT_TOOL(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
	//		 MyFrame::OnOpenQuantile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"), MyFrame::OnQuantile)
	//EVT_TOOL(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
	//		 MyFrame::OnOpenPercentile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnPercentile)
	//EVT_TOOL(XRCID("ID_MAP_HINGE_15"), MyFrame::OnOpenBox15)
	EVT_MENU(XRCID("ID_MAP_HINGE_15"), MyFrame::OnHinge15)
	//EVT_TOOL(XRCID("ID_MAP_HINGE_30"), MyFrame::OnOpenBox30)
	EVT_MENU(XRCID("ID_MAP_HINGE_30"), MyFrame::OnHinge30)
	//EVT_TOOL(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), MyFrame::OnOpenStddev)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), MyFrame::OnStddev)
	//EVT_TOOL(XRCID("ID_MAPANALYSIS_NATURAL_BREAK"),
	//	MyFrame::OnOpenNaturalBreak)
	EVT_MENU(XRCID("ID_MAPANALYSIS_NATURAL_BREAK"), MyFrame::OnNaturalBreak)
	//EVT_TOOL(XRCID("ID_MAPANALYSIS_UNIQUE_VALUE"),
	//		 MyFrame::OnOpenUniqueValue)
	EVT_MENU(XRCID("ID_MAPANALYSIS_UNIQUE_VALUE"), MyFrame::OnUniqueValue)
	//EVT_TOOL(XRCID("ID_MAPANALYSIS_EQUALINTERVAL"),
	//		 MyFrame::OnOpenEqualInterval)
	EVT_MENU(XRCID("ID_MAPANALYSIS_EQUALINTERVAL"), MyFrame::OnEqualInterval)

	EVT_MENU(XRCID("IDM_SMOOTH_RAWRATE"), MyFrame::OnRawrate)
	EVT_MENU(XRCID("IDM_SMOOTH_EXCESSRISK"), MyFrame::OnExcessrisk)
	EVT_MENU(XRCID("IDM_EMPERICAL_BAYES_SMOOTHER"), MyFrame::OnBayes)
	EVT_MENU(XRCID("IDM_SPATIAL_RATE_SMOOTHER"), MyFrame::OnSmoother)
	EVT_MENU(XRCID("IDM_SPATIAL_EMPIRICAL_BAYES"), MyFrame::OnEmpiricalBayes)
	EVT_MENU(XRCID("ID_MAPANALYSIS_SAVERESULTS"), MyFrame::OnSaveResults)
	EVT_MENU(XRCID("ID_MAP_RESET"), MyFrame::OnReset)

	EVT_MENU(XRCID("ID_VIEW_STANDARDIZED_DATA"),
			 MyFrame::OnViewStandardizedData)
	EVT_MENU(XRCID("ID_VIEW_ORIGINAL_DATA"), MyFrame::OnViewOriginalData)
	EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED"),
			 MyFrame::OnViewRegressionSelected)
	EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
			 MyFrame::OnViewRegressionSelectedExcluded)
	EVT_MENU(XRCID("ID_DISPLAY_STATISTICS"), MyFrame::OnDisplayStatistics)
	EVT_MENU(XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
			 MyFrame::OnShowAxesThroughOrigin)

	EVT_MENU(XRCID("wxID_ABOUT"), MyFrame::OnHelpAbout)
END_EVENT_TABLE()


void MyFrame::UpdateToolbarAndMenus()
{
	// This method is called when no particular window is currently active.
	// In this case, the close menu item should be disabled.

	LOG_MSG("In MyFrame::UpdateToolbarAndMenus");
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), "File", wxID_CLOSE,
								   false);

	bool proj_open = IsProjectOpen();
	bool shp_proj = IsProjectOpen() && !IsTableOnlyProject();
	bool table_proj = IsProjectOpen() && IsTableOnlyProject();

	wxMenuBar* mb = GetMenuBar();

	// Reset the toolbar frame title to default.
	SetTitle("OpenGeoDa");
	
	//MMM: the following two item states are set elsewhere.  This should be
	// unified.
	EnableTool(XRCID("ID_OPEN_SHAPE_FILE"), !proj_open);
	EnableTool(XRCID("ID_OPEN_TABLE_ONLY"), !proj_open);
	EnableTool(XRCID("ID_CLOSE_ALL"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_SHAPE_FILE"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_TABLE_ONLY"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_CLOSE_ALL"),
								   proj_open);

	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), true);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), false);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), false);
		
	EnableTool(XRCID("ID_EXPORT_MEAN_CENTERS"), shp_proj);
	EnableTool(XRCID("ID_EXPORT_CENTROIDS"), shp_proj);

	EnableTool(XRCID("ID_TOOLS_WEIGHTS_OPEN"), proj_open);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_CREATE"), true);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_CHAR"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_TOOLS_WEIGHTS_OPEN"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_TOOLS_WEIGHTS_CREATE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_TOOLS_WEIGHTS_CHAR"), proj_open);

	EnableTool(XRCID("ID_SHOW_CONVERTED_FEATURES"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CONVERTED_FEATURES"),
								   shp_proj);
	EnableTool(XRCID("ID_NEW_MAP_WINDOW"), shp_proj);	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_NEW_MAP_WINDOW"),
								   shp_proj);
	EnableTool(XRCID("ID_MAP_CHOICES"), shp_proj);
	
	EnableTool(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"), proj_open);	
	GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
								   proj_open);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   false);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_PAN_MODE"), proj_open);	
	
	EnableTool(XRCID("IDM_BOX"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BOX"), proj_open);

	EnableTool(XRCID("IDM_HIST"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_HIST"), proj_open);

	EnableTool(XRCID("IDM_SCATTERPLOT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTERPLOT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTER_NEW_PLOT"),
								   proj_open);
	
	EnableTool(XRCID("IDM_PCP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_PCP"), proj_open);

	EnableTool(XRCID("IDM_CC"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_CC"), shp_proj);

	EnableTool(XRCID("IDM_3DP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_3DP"), proj_open);

	EnableTool(XRCID("IDM_NEW_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, "Table", proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_NEW_TABLE_SAVE"),
				proj_open && project_p->GetGridBase()->ChangedSinceLastSave());
	
	EnableTool(XRCID("IDM_MSPL"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MSPL"), proj_open);
	EnableTool(XRCID("IDM_GMORAN"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_GMORAN"), proj_open);
	EnableTool(XRCID("IDM_MORAN_EBRATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MORAN_EBRATE"), proj_open);
	EnableTool(XRCID("IDM_UNI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_UNI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_MULTI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MULTI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_LISA_EBRATE"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LISA_EBRATE"), shp_proj);
	EnableTool(XRCID("IDM_GETIS_ORD"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_GETIS_ORD"), shp_proj);
	
	// Disable all items in Map menu except for Map Movie items and Cartogram
	// and New Map Layer and Duplicate Map
	GeneralWxUtils::EnableMenuAll(mb, "Map", false);
	EnableTool(XRCID("ID_MAP_CHOICES"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), false);
	//EnableTool(XRCID("ID_MAP_HINGE_15"), false);
	//EnableTool(XRCID("ID_MAP_HINGE_30"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_UNIQUE_VALUE"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_EQUALINTERVAL"), false);
	//EnableTool(XRCID("ID_MAPANALYSIS_NATURAL_BREAK"), false);

	EnableTool(XRCID("ID_SHOW_CARTOGRAM_MAP"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, "Map", XRCID("ID_SHOW_CARTOGRAM_MAP"),
								   shp_proj);
	//EnableTool(XRCID("ID_THEMATICMAP_MAPMOVIE_SINGLE"), shp_proj);	
	//GeneralWxUtils::EnableMenuItem(mb, "Map",
	//							   XRCID("ID_THEMATICMAP_MAPMOVIE_SINGLE"),
	//							   shp_proj);
	EnableTool(XRCID("ID_MAPANALYSIS_MAPMOVIE"), shp_proj);	
	GeneralWxUtils::EnableMenuItem(mb, "Map",
								   XRCID("ID_MAPANALYSIS_MAPMOVIE"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CONVERTED_FEATURES"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, "Map",
								   XRCID("ID_SHOW_CONVERTED_FEATURES"),
								   shp_proj);
	EnableTool(XRCID("ID_NEW_MAP_WINDOW"), shp_proj);	
	GeneralWxUtils::EnableMenuItem(mb, "Map",
								   XRCID("ID_NEW_MAP_WINDOW"), shp_proj);
	
	//Empty out the Options menu:
	wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
}

void MyFrame::SetMenusToDefault()
{
	LOG_MSG("Entering MyFrame::SetMenusToDefault");
	// This method disables all menu items that are not
	// in one of File, Tools, Methods, or Help menus.
	wxMenuBar* mb = GetMenuBar();
	if (!mb) return;
	wxMenu* menu = NULL;
	wxString menuText = wxEmptyString;
	int menuCnt = mb->GetMenuCount();
	for (int i=0; i<menuCnt; i++) {
		menu = mb->GetMenu(i);
		menuText = mb->GetMenuLabelText(i);
		if ( (menuText != "File") &&
			 (menuText != "Tools") &&
			 (menuText != "Methods") &&
			 (menuText != "Help") ) {
			GeneralWxUtils::EnableMenuAll(mb, menuText, false);
		}
	}

	LOG_MSG("Entering MyFrame::SetMenusToDefault");
}


Project* MyFrame::project_p = 0;
HiddenFrame* MyFrame::hidden_frame = 0;
std::list<wxToolBar*> MyFrame::toolbar_list(0);

MyFrame::MyFrame(const wxString& title,
				 const wxPoint& pos, const wxSize& size, long style):
wxFrame(NULL, -1, title, pos, size, style)
{
	LOG_MSG("Entering MyFrame::MyFrame");
	
	SetIcon(wxIcon(oGeoDaIcon_16x16_xpm));
	SetMenuBar(wxXmlResource::Get()->LoadMenuBar("ID_SHARED_MAIN_MENU"));

	wxToolBar* tb1;
	if (GeneralWxUtils::isMac()) {
		// for some reason the toolbar icons become disabled when
		// MyFrame loses focus on Mac.  So, for now we are not
		// making the toolbar a child of the Panel.
		tb1 = wxXmlResource::Get()->LoadToolBar(this, "ToolBar");
		SetToolBar(tb1);
	} else {
		wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
									 size, wxNO_BORDER);
		wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
		
		tb1 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar");
		//wxToolBar* tb2 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar2");
		//wxToolBar* tb3 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar3");
		wxASSERT(tb1);
		////wxASSERT(tb2);
		if (GeneralWxUtils::isUnix()) {
			// unfortunately, just GTK needs the toolbar to be added to the
			// topSizer rather than the panel itself.
			topSizer->Add(tb1, 0, 0, 0, 0);
			////topSizer->Add(tb2, 0, 0, 0, 0);
			////topSizer->Add(tb3, 0, 0, 0, 0);
		} else {
		    topSizer->Add(panel, 0, 0, 0, 0);
		}
		SetSizer(topSizer);
		topSizer->Fit(panel);
	}

	toolbar_list.push_front(tb1);
	////toolbar_list.push_front(tb2);
	////toolbar_list.push_front(tb3);
	
	//wxXmlResource::Get()->LoadPanel(appFrame, "ID_CONTROL_PANEL");
	
	SetMenusToDefault();
 	UpdateToolbarAndMenus();
		
	LOG_MSG("Exiting MyFrame::MyFrame");
}

MyFrame::~MyFrame()
{
	LOG_MSG("Entering MyFrame::~MyFrame()");
	frame = 0;
	theFrame = 0;
	LOG_MSG("Exiting MyFrame::~MyFrame()");
}

/** Call the virtual TemplateCanvas::Update function for every TemplateFrame
 child. */
void MyFrame::UpdateWholeView(wxFrame* caller)
{
	//LOG_MSG("Entering MyFrame::UpdateWholeView");	
	wxFrame* fr;
	int n = TemplateFrame::my_children.GetCount();
	for(int i = 0; i < n; i++) {
		fr = (wxFrame*) TemplateFrame::my_children[i];
		if (fr == caller) {
			LOG_MSG("MyFrame::UpdateWholeView: skipping caller");
		} else {
			fr->Update();
		}
	}
	//LOG_MSG("Exiting MyFrame::UpdateWholeView");		
}

void MyFrame::EnableTool(int xrc_id, bool enable)
{
	BOOST_FOREACH( wxToolBar* tb, toolbar_list ) {
		if (tb)	tb->EnableTool(xrc_id, enable);
	}
}

void MyFrame::EnableTool(const wxString& id_str, bool enable)
{
	BOOST_FOREACH( wxToolBar* tb, toolbar_list ) {
		if (tb)	tb->EnableTool(wxXmlResource::GetXRCID(id_str), enable);
	}
}

bool MyFrame::IsTableOnlyProject()
{
	return table_only_proj;
}

void MyFrame::SetTableOnlyProject(bool table_only_proj_s)
{
	table_only_proj = table_only_proj_s;
}

bool MyFrame::IsProjectOpen()
{
	return projectOpen;
}

void MyFrame::SetProjectOpen(bool open)
{
	table_only_proj = false;
	projectOpen = open;
}

#include "DialogTools/SelectWeightDlg.h"

GalWeight* MyFrame::GetGal()
{
	if (!project_p->GetWManager()->IsDefaultWeight()) {
		SelectWeightDlg dlg(project_p, this);
		if (dlg.ShowModal()!= wxID_OK) return 0;
	}
	gWeightTitle = project_p->GetWManager()->GetCurrWeightTitle();
	GeodaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeodaWeight::gal_type) {
		wxMessageBox("Error: Only GAL type weights are currently supported. "
					 "Other weight types are internally converted to GAL.");
		return 0;
	}
	return (GalWeight*) w;
}

void MyFrame::OnOpenNewTable()
{
	LOG_MSG("Entering MyFrame::OnOpenNewTable");
	NewTableViewerFrame* tvf = 0;
	wxGrid* g = project_p->GetGridBase()->GetView();
	if (g) tvf = (NewTableViewerFrame*) g->GetParent();
	if (!tvf) {
		wxString msg("The Table should always be open, although somtimes it "
					 "is hidden while the project is open.  This condition "
					 "has been violated.  Please report this to "
					 "the program developers.");
		wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING);
		dlg.ShowModal();
		tvf = new NewTableViewerFrame(frame, project_p,
									  GeoDaConst::table_frame_title,
									  wxDefaultPosition,
									  GeoDaConst::table_default_size,
									  wxDEFAULT_FRAME_STYLE);
	}
	tvf->Show(true);
	
	LOG_MSG("Exiting MyFrame::OnOpenNewTable");
}

#include "DialogTools/VariableSettingsDlg.h"
bool MyFrame::GetVariableSetting(bool IsU)
{
	LOG_MSG("Entering MyFrame::GetVariableSetting");
	
	if (m_VarDefault && IsU) return true;
	
	
	VariableSettingsDlg VS(IsU, gCompleteFileName,	gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project_p->GetGridBase());
	
	if (VS.ShowModal() != wxID_OK) return false;
	
	m_VarDefault = VS.m_CheckDefault;
	m_gVar1 = VS.m_Var1;
	m_gVar2 = VS.m_Var2;
	
	LOG_MSG("Exiting MyFrame::GetVariableSetting");
	return true;
}

/** returns false if user wants to abort the operation */
bool MyFrame::OnCloseMap()
{
	LOG_MSG("Entering MyFrame::OnCloseMap");
	
	wxString msg;
	if (IsProjectOpen() && project_p->GetGridBase()->ChangedSinceLastSave()) {
		msg = "Ok to close current project?  There have been changes to the "
			"Table since it was last saved. To save your work, "
			"go to Table > Save";
		wxMessageDialog msgDlg(this,
							   msg,
							   "Close with unsaved Table changes?",
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
		if (msgDlg.ShowModal() != wxID_YES) return false;
	}
		
	SetProjectOpen(false);
	if (project_p) {
		project_p->GetFramesManager()->closeAndDeleteWhenEmpty();
		std::list<FramesManagerObserver*> observers(
						project_p->GetFramesManager()->getCopyObservers());
		std::list<FramesManagerObserver*>::iterator it;
		for (it=observers.begin(); it != observers.end(); it++) {
			if (wxTopLevelWindow* w = dynamic_cast<wxTopLevelWindow*>(*it)) {
				wxString msg = "Calling Close(true) for window: "+w->GetTitle();
				LOG_MSG(msg);
				w->Close(true);
			}
		}
	}
	
	//int cnt = TemplateFrame::my_children.GetCount();
	//for (int i=0; i<cnt; i++) {
	//	((wxFrame*) TemplateFrame::my_children[cnt-i-1])->Close(true);
	//}
	TemplateFrame::my_children.Clear();
	
	if (project_p && project_p->regression_dlg) {
		((wxDialog*) project_p->regression_dlg)->Close(true);
		project_p->regression_dlg = 0;
	}
	if (project_p) delete project_p; project_p = 0;
	if (hidden_frame) hidden_frame->Destroy(); hidden_frame = 0;
	
	
	delete [] m_gX;	m_gX = 0;
	delete [] m_gY;	m_gY = 0;
	
	gObservation = -1;
	gCompleteFileName = wxEmptyString;
	gWeightTitle =wxEmptyString;
	m_VarDefault = false;
	m_gVar1 = wxEmptyString;
	m_gVar2 = wxEmptyString;
	
	UpdateToolbarAndMenus();
	EnableTool(XRCID("ID_OPEN_SHAPE_FILE"), true);
	EnableTool(XRCID("ID_OPEN_TABLE_ONLY"), true);
	EnableTool(XRCID("ID_CLOSE_ALL"), false);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   ID_TEST_MAP_FRAME, false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   ID_TEST_TABLE_FRAME, false);
	
	return true;
}

void MyFrame::OnProjectOpen(wxCommandEvent& WXUNUSED(event) )
{
	LOG_MSG("Entering MyFrame::OnProjectOpen");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseMap()");
		if (!OnCloseMap()) return;
	}
	
	wxString defaultDir;
	wxString defaultFile;
    wxFileDialog dlg(this,
					 "Choose a Shape file to open.",
					 defaultDir,
					 defaultFile,
					 "Shape files (*.shp)|*.shp");	
	
    if (dlg.ShowModal() == wxID_OK) {
		// dlg.GetPath returns the selected filename with complete path.
		wxFileName ifn(dlg.GetPath());
		// dlg.GetFullPath returns the filename with path and extension.
		gCompleteFileName = ifn.GetFullPath();
		
		if (IsLineShapeFile(ifn.GetFullPath())) {
			wxMessageBox("Error: OpenGeoDa does not support Shapefiles "
						 "with line data at this time.  Please choose a "
						 "Shapefile with either point or polygon data.");
			return;
		}
		
		bool shx_found;
		bool dbf_found;
		if (!ExistsShpShxDbf(ifn, 0, &shx_found, &dbf_found)) {
			wxString msg;
			msg << "Error: " <<  ifn.GetName() << ".shp, ";
			msg << ifn.GetName() << ".shx, and " << ifn.GetName() << ".dbf ";
			msg << "were not found together in the same file directory. ";
			msg << "Could not find ";
			if (!shx_found && dbf_found) {
				msg << ifn.GetName() << ".shx.";
			} else if (shx_found && !dbf_found) {
				msg << ifn.GetName() << ".dbf.";
			} else {
				msg << ifn.GetName() << ".shx and " << ifn.GetName() << ".dbf.";
			}
			wxMessageBox(msg);
			return;
		}
		
		DbfFileReader dbf_reader(ifn.GetPathWithSep()+ifn.GetName() + ".dbf");
		if (!dbf_reader.isDbfReadSuccess()) {
			wxString msg("There was a problem reading in the DBF file.");
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		
		project_p = new Project(dbf_reader.getNumRecords());
		DbfGridTableBase* grid_base =
			new DbfGridTableBase(dbf_reader, project_p->highlight_state);
		project_p->Init(grid_base, ifn);
		if (!project_p->IsValid()) {
			wxString msg("Could not open new project: ");
			msg << project_p->GetErrorMessage();
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			delete grid_base;
			delete project_p; project_p = 0;
			return;
		}
		// By this point, we know that project has created as
		// TopFrameManager object with delete_if_empty = false
		
		// This call is very improtant because we need the wxGrid to
		// take ownership of grid_base
		NewTableViewerFrame* tvf = 0;
		tvf = new NewTableViewerFrame(this, project_p,
									  GeoDaConst::table_frame_title,
									  wxDefaultPosition,
									  GeoDaConst::table_default_size,
									  wxDEFAULT_FRAME_STYLE);
		
		SetProjectOpen(true);	
		SetTableOnlyProject(false);
		UpdateToolbarAndMenus();
		
		gObservation = grid_base->GetNumberRows();
		
		// These should eventually go away
		m_gX = new double[gObservation];  // global variable from Var Dialog
		m_gY = new double[gObservation];  // global variable from Var Dialog
		
		// This will go away as soon as everything starts using
		// the new Highlight State
		gSelection.Init(gObservation+1); // Initialize the bitmap
		gSelection.Update();
		
		ProgressDlg* prog_dlg = new ProgressDlg(this, wxID_ANY,
												"Project Loading Progress");
		prog_dlg->Show();
		prog_dlg->StatusUpdate(0, "Loading Map...");		
		
		MapFrame* mapframe = new MapFrame(gCompleteFileName, this, project_p,
										  "Canvas Frame",
										  wxDefaultPosition,
										  GeoDaConst::map_default_size,
										  wxDEFAULT_FRAME_STYLE,
										  prog_dlg);
		EnableTool(XRCID("ID_OPEN_SHAPE_FILE"), false);
		EnableTool(XRCID("ID_OPEN_TABLE_ONLY"), false);
		EnableTool(XRCID("ID_CLOSE_ALL"), true);
		wxMenuBar* mb = GetMenuBar();
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_OPEN_SHAPE_FILE"), false);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_OPEN_TABLE_ONLY"), false);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_CLOSE_ALL"), true);
		
		// HidenFrame can go away as soon as everything is convernted to
		// using HighlightState
		if (hidden_frame) hidden_frame->Destroy(); hidden_frame = 0;
		hidden_frame = new HiddenFrame(frame, project_p,
											  "Hidden Frame",
											  wxDefaultPosition,
											  GeoDaConst::map_default_size,
											  wxDEFAULT_FRAME_STYLE);
		prog_dlg->StatusUpdate(1, "Finished");
		prog_dlg->Destroy();
		
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   ID_TEST_MAP_FRAME, true);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   ID_TEST_TABLE_FRAME, true);
	}
	
	LOG_MSG("Exiting MyFrame::OnProjectOpen");	
}

void MyFrame::OnOpenTableOnly(wxCommandEvent& WXUNUSED(event) )
{
	LOG_MSG("Entering MyFrame::OnOpenTableOnly");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseMap()");
		if (!OnCloseMap()) return;
	}
	
	wxString defaultDir;
	wxString defaultFile;
    wxFileDialog dlg(this,
					 "Choose a Table DBF file to open",
					 defaultDir,
					 defaultFile,
					 "DBF files (*.dbf)|*.dbf");	
	
    if (dlg.ShowModal() == wxID_OK) {
		// dlg.GetPath returns the selected filename with complete path.
		wxFileName ifn(dlg.GetPath());
		// dlg.GetFullPath returns the filename with path and extension.
		gCompleteFileName = ifn.GetFullPath();
		
		DbfFileReader dbf_reader(ifn.GetPathWithSep()+ifn.GetName() + ".dbf");
		if (!dbf_reader.isDbfReadSuccess()) {
			wxString msg("There was a problem reading in the DBF file.");
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		
		project_p = new Project(dbf_reader.getNumRecords());
		DbfGridTableBase* grid_base =
			new DbfGridTableBase(dbf_reader, project_p->highlight_state);
		project_p->Init(grid_base);		
		if (!project_p->IsValid()) {
			wxString msg("Could not open new project: ");
			msg << project_p->GetErrorMessage();
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			delete grid_base;
			delete project_p; project_p = 0;
			return;
		}
		
		// This call is very improtant because we need the wxGrid to
		// take ownership of grid_base
		NewTableViewerFrame* tvf = 0;
		tvf = new NewTableViewerFrame(frame, project_p,
									  GeoDaConst::table_frame_title,
									  wxDefaultPosition,
									  GeoDaConst::table_default_size,
									  wxDEFAULT_FRAME_STYLE);
		tvf->Show(true);
		
		SetProjectOpen(true);	
		SetTableOnlyProject(true);
		UpdateToolbarAndMenus();
		
		gObservation = grid_base->GetNumberRows();
		
		// These should eventually go away
		m_gX = new double[gObservation];  // global variable from Var Dialog
		m_gY = new double[gObservation];  // global variable from Var Dialog
		
		// This will go away as soon as everything starts using
		// the new Highlight State
		gSelection.Init(gObservation+1); // Initialize the bitmap
		gSelection.Update();		
		
		EnableTool(XRCID("ID_OPEN_SHAPE_FILE"), false);
		EnableTool(XRCID("ID_OPEN_TABLE_ONLY"), false);
		EnableTool(XRCID("ID_CLOSE_ALL"), true);
		wxMenuBar* mb = GetMenuBar();
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_OPEN_SHAPE_FILE"), false);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_OPEN_TABLE_ONLY"), false);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   XRCID("ID_CLOSE_ALL"), true);
		
		// HidenFrame can go away as soon as everything is convernted to
		// using HighlightState
		if (hidden_frame) hidden_frame->Destroy(); hidden_frame = 0;
		hidden_frame = new HiddenFrame(frame, project_p,
									   "Hidden Frame",
									   wxDefaultPosition,
									   GeoDaConst::map_default_size,
									   wxDEFAULT_FRAME_STYLE);
		
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   ID_TEST_MAP_FRAME, true);
		GeneralWxUtils::EnableMenuItem(mb, "File",
									   ID_TEST_TABLE_FRAME, true);		
	}
	LOG_MSG("Exiting MyFrame::OnOpenTableOnly");
}


void MyFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering MyFrame::OnClose");
	
	wxString msg;
	wxString title;
	if (IsProjectOpen() && project_p->GetGridBase()->ChangedSinceLastSave()) {
		msg = "Ok to Exit OpenGeoDa?  The current Table has unsaved changes."
		" To save your work, go to Table > Save";
		title = "Exit with unsaved Table changes?";
	} else {
		msg = "Ok to Exit?";
		title = "Exit?";
	}
	
	// Construct a message dialog.
	wxMessageDialog msgDlg(this,
						   msg,
						   title,
						   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
	
    // Show the message dialog, and if it returns wxID_YES...
	if (msgDlg.ShowModal() == wxID_YES) {
		if (IsProjectOpen()) {
			project_p->GetFramesManager()->closeAndDeleteWhenEmpty();
		}
		Destroy();
    }

	LOG_MSG("Exiting MyFrame::OnClose");
}

void MyFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnMenuClose");
	Close(); // This will result in a call to OnClose
    LOG_MSG("Exiting MyFrame::OnMenuClose");
}

void MyFrame::OnCloseAll(wxCommandEvent& event)
{
	LOG_MSG("In MyFrame::OnCloseAll");
	OnCloseMap();
}

void MyFrame::OnSelectWithRect(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectWithRect(event);
	} else {
		t->OnSelectWithRect(event);
	}	
}

void MyFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectWithCircle(event);
	} else {
		t->OnSelectWithCircle(event);
	}
}

void MyFrame::OnSelectWithLine(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectWithLine(event);
	} else {
		t->OnSelectWithLine(event);
	}
}

void MyFrame::OnSelectionMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectionMode(event);
	} else {
		t->OnSelectionMode(event);
	}
}

void MyFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapFitToWindowMode(event);
	} else {
		t->OnFitToWindowMode(event);
	}	
}

void MyFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnFixedAspectRatioMode(event);
}

void MyFrame::OnZoomMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnZoomMode(event);
}

void MyFrame::OnZoomIn(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapZoomIn(event);
	}
	// MMM: Not implemented in new style framework
}

void MyFrame::OnZoomOut(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapZoomOut(event);
	}
	// MMM: Not implemented in new style framework
}


void MyFrame::OnPanMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapPanMode(event);
	} else {
		t->OnPanMode(event);
	}
}

void MyFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called MyFrame::OnPrintCanvasState");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnPrintCanvasState(event);
}

void MyFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnSaveCanvasImageAs(event);
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnQuit");
	// Generate a wxCloseEvent for MyFrame.  MyFrame::OnClose will
	// be called and will give the user a chance to not exit program.
	Close();
	LOG_MSG("Exiting MyFrame::OnQuit");
}

void MyFrame::OnSaveSelectedToColumn(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnSaveSelectedToColumn");
	SaveSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
	LOG_MSG("Exiting MyFrame::OnSaveSelectedToColumn");
}

void MyFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (t->IsOldStyle()) {
		t->OnOldStyleCanvasBackgroundColor(event);
	} else {
		t->OnCanvasBackgroundColor(event);
	}
}

void MyFrame::OnLegendBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (t->IsOldStyle()) {
		t->OnOldStyleLegendBackgroundColor(event);
	}
	// MMM we don't have a New Style method for Legends yet.
}

void MyFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectableFillColor(event);
	} else {
		t->OnSelectableFillColor(event);	
	}
}

void MyFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineColor(event);
}

void MyFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapSelectableOutlineVisible(event);
	} else {
		t->OnSelectableOutlineVisible(event);
	}
}

void MyFrame::OnHighlightColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* m = dynamic_cast<MapFrame*>(t)) {
		m->OnMapHighlightColor(event);
	} else {
		t->OnHighlightColor(event);	
	}
}

void MyFrame::OnOpenMapwindow(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnOpenMapwindow");	
	MapFrame *subframe = new MapFrame(gCompleteFileName,
									  frame, project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnOpenMapwindow");
}

void MyFrame::OnNewMap(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnNewMap");
	wxFileDialog dlg(this,
		"Input Shp file",
		wxEmptyString,
		wxEmptyString,
		"Shp files (*.shp)|*.shp");
	
	if (dlg.ShowModal() == wxID_OK)	{
		wxString m_path = dlg.GetPath();

		if (gObservation != GetShpFileSize(m_path)) {
			wxMessageBox(m_path + " has different size!");
			return;
		}

		MapFrame *subframe = new MapFrame(m_path,
										  frame, project_p, "Canvas Frame",
										  wxDefaultPosition,
										  GeoDaConst::map_default_size,
										  wxDEFAULT_FRAME_STYLE);
	}
	LOG_MSG("Exiting MyFrame::OnNewMap");
}

void MyFrame::OnSetDefaultVariableSettingss(wxCommandEvent& WXUNUSED(event) )
{	
	VariableSettingsDlg VS(false, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2 , m_gX, m_gY,
						   m_VarDefault,
						   project_p->GetGridBase());

	if (VS.ShowModal() == wxID_OK) {
		m_VarDefault = VS.m_CheckDefault;
		m_gVar1 = VS.m_Var1;
		m_gVar2 = VS.m_Var2;

	}
}

void MyFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnCopyImageToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (t->IsOldStyle()) {
		t->OnOldStyleCopyImageToClipboard(event);
	} else {
		t->OnCopyImageToClipboard(event);
	}
	LOG_MSG("Exiting MyFrame::OnCopyImageToClipboard");
}


void MyFrame::OnCopyLegendToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnCopyLegendToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnOldStyleCopyLegendToClipboard(event);
	// MMM: there are only Old Style legends at present
	LOG_MSG("Exiting MyFrame::OnCopyLegendToClipboard");
}


void MyFrame::OnToolsWeightsOpen(wxCommandEvent& WXUNUSED(event) )
{
	SelectWeightDlg dlg(project_p, this);
	if (dlg.ShowModal()!= wxID_OK) return;
	gWeightTitle = project_p->GetWManager()->GetCurrWeightTitle();
	GeodaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeodaWeight::gal_type) {
		wxMessageBox("Error: Only GAL format supported internally.");
	}
}


#include "DialogTools/CreatingWeightDlg.h"
void MyFrame::OnToolsWeightsCreate(wxCommandEvent& WXUNUSED(event) )
{
	// This is the only call to CreatingWeightDlg where the project
	// can be NULL.  If the project is not NULL, then only allow this
	// dialog to open if the current Table has no unsaved changes.
	CreatingWeightDlg dlg(NULL, project_p); // project_p can be NULL
	dlg.ShowModal();
}


#include "DialogTools/WeightCharacterDlg.h"
void MyFrame::OnToolsWeightsChar(wxCommandEvent& event )
{
	LOG_MSG("Entering MyFrame::OnToolsWeightsChar");
	CWeightCharacterDlg dlg(this);
	if(dlg.ShowModal() == wxID_OK) {
		HistFrame *subframe = new HistFrame(frame, project_p,
											"Weights Connectivity Histogram",
											wxDefaultPosition,
											GeoDaConst::hist_default_size,
											wxDEFAULT_FRAME_STYLE,
											dlg.m_WeightFile,
											dlg.m_freq);
	}
	LOG_MSG("Exiting MyFrame::OnToolsWeightsChar");
}


void MyFrame::OnMapChoices(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnMapChoices");
	wxMenu* popupMenu = 0;
	if (GeneralWxUtils::isMac()) {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES");
	} else {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES_NO_ICONS");
	}
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnMapChoices");
}

#include "Thiessen/Thiessen.h"
#include "DialogTools/ThiessenPolygonDlg.h"
void MyFrame::OnShapePointsToPolygons(wxCommandEvent& WXUNUSED(event) )
{
	CThiessenPolygonDlg dlg(false, true, this);
	dlg.ShowModal();

}
void MyFrame::OnShapePolygonsToCentroids(wxCommandEvent& WXUNUSED(event) )
{
	CThiessenPolygonDlg dlg(false, false, this);
	dlg.ShowModal();
}
void MyFrame::OnShapePolygonsToMeanCenters(wxCommandEvent& WXUNUSED(event))
{
	CThiessenPolygonDlg dlg(true, false, this);
	dlg.ShowModal();
}


#include "DialogTools/DBF2SHPDlg.h"
void MyFrame::OnShapePointsFromDBF(wxCommandEvent& WXUNUSED(event) )
{
	CDBF2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/ASC2SHPDlg.h"
void MyFrame::OnShapePointsFromASCII(wxCommandEvent& WXUNUSED(event) )
{
	CASC2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/CreateGridDlg.h"
void MyFrame::OnShapePolygonsFromGrid(wxCommandEvent& WXUNUSED(event) )
{
	CCreateGridDlg dlg(this);
	dlg.ShowModal();
}


#include "DialogTools/Bnd2ShpDlg.h"
void MyFrame::OnShapePolygonsFromBoundary(wxCommandEvent& WXUNUSED(event) )
{
	CBnd2ShpDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/SHP2ASCDlg.h" 
void MyFrame::OnShapeToBoundary(wxCommandEvent& WXUNUSED(event) )
{
	CSHP2ASCDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/Dbf2GaussDlg.h" 
void MyFrame::OnExportDataToASCII(wxCommandEvent& WXUNUSED(event) )
{
	CDbf2GaussDlg dlg(0,this);
	dlg.ShowModal();
}


#include "DialogTools/AddCentroidsDlg.h"
void MyFrame::OnToolsDataExportMeanCenters(wxCommandEvent& WXUNUSED(event) )
{
	CAddCentroidsDlg dlg(true, this);
	dlg.ShowModal();
}

void MyFrame::OnToolsDataExportCentroids(wxCommandEvent& WXUNUSED(event) )
{
	CAddCentroidsDlg dlg(false, this);
	dlg.ShowModal();
}

void MyFrame::OnMoveSelectedToTop(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	grid_base->MoveSelectedToTop();
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}

void MyFrame::OnClearSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	grid_base->DeselectAll();
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}

void MyFrame::OnRangeSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	RangeSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnFieldCalculation(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	FieldNewCalcSheetDlg dlg(project_p, this,
							 wxID_ANY, "Field Calculation",
							 wxDefaultPosition,
							 wxSize(700, 500) );
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnAddCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerAddColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnDeleteCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerDeleteColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnEditFieldProperties(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerEditFieldPropertiesDlg dlg(project_p->GetGridBase(),
										 wxDefaultPosition,
										 wxSize(600, 400));
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnMergeTableData(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	MergeTableDlg dlg(project_p->GetGridBase(), wxDefaultPosition);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnSaveCopyOfShpFile(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()
		|| project_p->IsTableOnlyProject()) return;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	DbfFileHeader t_header = grid_base->orig_header;
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxFileDialog dlg( this, "Name of SHP File Copy", wxEmptyString,
					 wxEmptyString,
					 "SHP files (*.shp)|*.shp",
					 wxFD_SAVE );
	dlg.SetPath(project->GetMainDir());
	if (dlg.ShowModal() == wxID_OK) {
		wxFileName new_shp_fname(dlg.GetPath());
		wxString new_main_dir = new_shp_fname.GetPathWithSep();
		wxString new_main_name = new_shp_fname.GetName();
		wxString new_dbf = new_main_dir + new_main_name + ".dbf";
		wxString new_shp = new_main_dir + new_main_name + ".shp";
		wxString new_shx = new_main_dir + new_main_name + ".shx";
		wxString curr_dbf = project->GetMainDir()
			+ project->GetMainName() + ".dbf";
		wxString curr_shp = project->GetMainDir()
			+ project->GetMainName() + ".shp";
		wxString curr_shx = project->GetMainDir()
			+ project->GetMainName() + ".shx";
		
		// Prompt for overwrite permission
		if (curr_shp != new_shp) {
			int exists_cnt = 0;
			wxString exists_msg = "";
			if (wxFileExists(new_shx)) {
				exists_msg = new_shx;
				exists_cnt++;
			}
			if (wxFileExists(new_shp)) {
				if (exists_cnt++ > 0) exists_msg += " and ";
				exists_msg += new_shp;
			}
			if (wxFileExists(new_dbf)) {
				if (exists_cnt++ > 0) exists_msg += " and ";
				exists_msg += new_dbf;
				if (exists_cnt == 3) {
					exists_msg = new_shx + ", " + new_shp + " and " + new_dbf;
				}
			}
			if (exists_cnt > 0) {
				exists_msg += " already ";
				exists_msg += (exists_cnt == 1) ? "exists" : "exist";
				exists_msg += ". Ok to overwrite?";
				wxMessageDialog dlg (this, exists_msg, "Overwrite?",
									 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
				if (dlg.ShowModal() != wxID_YES) return;
			}
		}
		
		// Automatically overwrite existing dbf, shx and shp counterparts
		// since we have permissio to overwrite

		if (curr_shp != new_shp && wxFileExists(new_shp)) {
			if (!wxRemoveFile(new_shp)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name + ".shp";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		if (curr_shp != new_shp) {
			if (!wxCopyFile(curr_shp, new_shp, true)) {
				wxString msg("There was an unexpected problem while copying "
							 "the current SHP file to the new SHP file.");
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		
		if (curr_shx != new_shx && wxFileExists(new_shx)) {
			if (!wxRemoveFile(new_shp)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name + ".shx";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		if (curr_shx != new_shx) {
			if (!wxCopyFile(curr_shx, new_shx, true)) {
				wxString msg("There was an unexpected problem while copying "
							 "the current SHX file to the new SHX file.");
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}

		if (wxFileExists(new_dbf)) {
			if (!wxRemoveFile(new_dbf)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name + ".dbf";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		
		wxString err_msg;
		bool success = grid_base->WriteToDbf(new_dbf, err_msg);
		if (!success) {
			grid_base->orig_header = t_header;
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			LOG_MSG(err_msg);
			return;
		} else {
			wxString msg;
			if (curr_shp != new_shp) {
				msg = "Copy of SHP file saved successfully";
			} else {
				msg = "SHP file saved successfully";
			}
			LOG_MSG(msg);
			wxMessageDialog dlg (this, msg, "Success",
								 wxOK | wxICON_INFORMATION);
			dlg.ShowModal();
			GeneralWxUtils::EnableMenuItem(GetMenuBar(),
										   XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
		}
	}
}

void MyFrame::OnSaveCopyOfTable(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	DbfFileHeader t_header = grid_base->orig_header;
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxFileDialog dlg( this, "Name of DBF File Copy", wxEmptyString,
					 wxEmptyString,
					 "DBF files (*.dbf)|*.dbf",
					 wxFD_SAVE );
	dlg.SetPath(project->GetMainDir());
	if (dlg.ShowModal() == wxID_OK) {
		wxFileName new_dbf_fname(dlg.GetPath());
		wxString new_main_dir = new_dbf_fname.GetPathWithSep();
		wxString new_main_name = new_dbf_fname.GetName();
		wxString new_dbf = new_main_dir + new_main_name + ".dbf";
		wxString curr_dbf = project->GetMainDir() + project->GetMainName()
			+ ".dbf";
		
		// Prompt for overwrite permission
		if (curr_dbf != new_dbf) {
			wxString msg;
			if (wxFileExists(new_dbf)) {
				msg << new_dbf << " already exists.  Ok to overwrite?";
				wxMessageDialog dlg (this, msg, "Overwrite?",
									 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
				if (dlg.ShowModal() != wxID_YES) return;
			}
		}
		
		// Automatically overwrite existing dbf since we have 
		// permission to overwrite.
		
		if (wxFileExists(new_dbf)) {
			if (!wxRemoveFile(new_dbf)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name + ".dbf";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		
		wxString err_msg;
		bool success = grid_base->WriteToDbf(new_dbf, err_msg);
		if (!success) {
			grid_base->orig_header = t_header;
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			LOG_MSG(err_msg);
			return;
		} else {
			wxString msg;
			if (curr_dbf != new_dbf) {
				msg = "Copy of DBF file saved successfully";
			} else {
				msg = "DBF file saved successfully";
			}
			LOG_MSG(msg);
			wxMessageDialog dlg (this, msg, "Success",
								 wxOK | wxICON_INFORMATION);
			dlg.ShowModal();
			GeneralWxUtils::EnableMenuItem(GetMenuBar(),
										   XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
		}
	}
}

void MyFrame::OnSaveTable(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	DbfFileHeader t_header = grid_base->orig_header;
	
	wxString curr_dbf = project->GetMainDir() + project->GetMainName() + ".dbf";
	
	wxString err_msg;
	bool success = grid_base->WriteToDbf(curr_dbf, err_msg);
	if (!success) {
		grid_base->orig_header = t_header;
		wxMessageBox(err_msg);
		LOG_MSG(err_msg);
	} else {
		GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_NEW_TABLE_SAVE"),
							project_p->GetGridBase()->ChangedSinceLastSave());
		wxString msg("Table saved successfully");
		wxMessageDialog dlg (this, msg, "Success",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		LOG_MSG(msg);
	}	
}

void MyFrame::OnRegressionClassic(wxCommandEvent& event)
{
	wxString regFileName;
	if (IsProjectOpen()) {
		if (project_p->regression_dlg) {
			LOG_MSG("Project::regression_dlg pointer is not null!");
			return;
		}
	} else {
		OnOpenTableOnly(event);		
		if (!IsProjectOpen()) return;
	}
	regFileName = project_p->GetMainDir() + project_p->GetMainName()
		+ "Regression.txt";

	// Dialog to get the filename and title for Regression Report Output.
	CRegressionTitleDlg dlg(this, regFileName);
	if (dlg.ShowModal() != wxID_OK) return;
		
	RegressionDlg* regDlg;
	regDlg = new RegressionDlg(project_p, this, 1, 
							   dlg.m_title->GetValue(),
							   dlg.m_outputfile->GetValue(),
							   dlg.m_check1->GetValue(),
							   dlg.m_check2->GetValue(),
							   dlg.m_check3->GetValue());
	project_p->regression_dlg = regDlg;
	regDlg->Show(true);
}


void MyFrame::DisplayRegression(const wxString dump)
{
	CRegressionReportDlg *regReportDlg = new CRegressionReportDlg(this, dump);
	regReportDlg->Show(true);
	regReportDlg->m_textbox->SetSelection(0, 0);
}

void MyFrame::OnShowCartogramMap(wxCommandEvent& WXUNUSED(event) )
{
	if (GetVariableSetting(true)) {
		wxString title = "Cartogram - " + m_gVar1;
		CartogramFrame *subframe =
			new CartogramFrame(frame, project_p, title, wxDefaultPosition,
							   GeoDaConst::map_default_size,
							   wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnMoreIter100(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnMoreIter100(event);
	}
}

void MyFrame::OnMoreIter500(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnMoreIter500(event);
	}
}

void MyFrame::OnMoreIter1000(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnMoreIter1000(event);
	}
}

void MyFrame::OnMapMovieSingle(wxCommandEvent& WXUNUSED(event) )
{
	if (GetVariableSetting(true)) {
		wxString title = "Map Movie - " + m_gVar1;
		MapMovieFrame *subframe =
		new MapMovieFrame(frame, project_p, 1, title,
						  wxDefaultPosition,
						  wxSize(750, 450),
						  wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnMapMovieCumul(wxCommandEvent& WXUNUSED(event) )
{
	if (GetVariableSetting(true)) {
		wxString title = "Map Movie - " + m_gVar1;
		MapMovieFrame *subframe =
			new MapMovieFrame(frame,  project_p, 2, title,
							  wxDefaultPosition,
							  wxSize(750, 450), wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnExploreHist(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreHist");
	if (!GetVariableSetting(true)) return;
	
	HistFrame *subframe = new HistFrame(frame,  project_p, 
										"Histogram - " + m_gVar1,
										wxDefaultPosition,
										GeoDaConst::hist_default_size,
										wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnExploreHist");
}

void MyFrame::OnExploreScatterplot(wxCommandEvent& WXUNUSED(event))
{
	if (GetVariableSetting(false)) {
		if (!CheckDataValidity(gObservation, m_gX)) {
			wxMessageBox("It has no regression!");
			return;
		}
		ScatterPlotFrame *subframe =
			new ScatterPlotFrame(frame, project_p,
								 "Scatter Plot - " + m_gVar2
									+ " vs " + m_gVar1,
								 wxDefaultPosition,
								 GeoDaConst::scatterplot_default_size,
								 wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnExploreScatterNewPlot(wxCommandEvent& WXUNUSED(event))
{
	ScatterPlotVarsDlg dlg(m_gVar1 , m_gVar2, m_gVar3,
							project_p->GetGridBase() );
	if(dlg.ShowModal() == wxID_OK) {
//		ScatterNewPlotFrame* subframe =
//		new ScatterNewPlotFrame(frame, project_p,
//								"New Scatter Plot - " + dlg.var_Y
//								+ " vs "+ dlg.var_X,
//								wxDefaultPosition,
//								GeoDaConst::scatterplot_default_size,
//								wxDEFAULT_FRAME_STYLE,
//								dlg.var_X, dlg.var_Y, dlg.var_Z,
//								dlg.is_include_bubble_size);
	}
}

void MyFrame::OnExploreTestMap(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreTestMap");
//	TestMapFrame *subframe = new TestMapFrame(frame, project_p,
//											  "Test Map Frame",
//											  wxDefaultPosition,
//											  GeoDaConst::map_default_size,
//											  wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnExploreTestMap");
}

void MyFrame::OnExploreTestTable(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreTestTable");	
	//TestTableFrame *subframe = new TestTableFrame(frame, project_p,
	//											"Test Table Frame",
	//											wxDefaultPosition,
	//											GeoDaConst::map_default_size,
	//											wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnExploreTestTable");
}

void MyFrame::OnExploreBox(wxCommandEvent& WXUNUSED(event))
{
	if (GetVariableSetting(true)) {
		BoxPlotFrame *subframe =
			new BoxPlotFrame(frame, project_p,
							 "Box Plot (Hinge = 1.5) - " + m_gVar1,
							 wxDefaultPosition,
							 GeoDaConst::boxplot_default_size,
							 wxDEFAULT_FRAME_STYLE);
	}
}

#include "DialogTools/PCPDlg.h"
void MyFrame::OnExplorePCP(wxCommandEvent& WXUNUSED(event))
{
	PCPDlg dlg(project_p->GetGridBase(),this);
	if (dlg.ShowModal() == wxID_OK) {
		PCPFrame* s = new PCPFrame(dlg.pcp_col_ids,
								   frame, project_p,
								   "Parallel Coordinate Plot",
								   wxDefaultPosition,
								   GeoDaConst::pcp_default_size,
								   wxDEFAULT_FRAME_STYLE);
	}
}

#include "DialogTools/3DDlg.h"
void MyFrame::OnExplore3DP(wxCommandEvent& WXUNUSED(event))
{
	C3DDlg dlg(project_p->GetGridBase(), this);
	if(dlg.ShowModal() == wxID_OK) {
		C3DPlotFrame *subframe =
			new C3DPlotFrame(frame, project_p,
							 "3D Plot", wxDefaultPosition,
							 GeoDaConst::three_d_default_size,
							 wxDEFAULT_FRAME_STYLE,
							 dlg.x_col_id, dlg.y_col_id, dlg.z_col_id);
	}
}

#include "DialogTools/CCVariableDlg.h"
#include "DialogTools/ConditionViewDlg.h"

void MyFrame::OnExploreCC(wxCommandEvent& WXUNUSED(event))
{
	CConditionViewDlg dlg(this);
	if(dlg.ShowModal() == wxID_OK) {
		CCVariableDlg dlg2(project_p->GetGridBase(), this);
		if(dlg2.ShowModal() == wxID_OK) {
			wxString title = "Conditional View Plot";
			if (dlg2.cc4 == "_dummy_name_")
				title += " (X:" + dlg2.cc1 + ", Y:"+
					dlg2.cc2 + ", Var:" + dlg2.cc3 + ")";
			else
				title += " (X:" + dlg2.cc1 + ", Y:" +
					dlg2.cc2 + ", Var1:" + dlg2.cc3 +", Var2:" + dlg2.cc4 + ")";
			
			ConditionalViewFrame *subframe =
				new ConditionalViewFrame(frame,  project_p,
										 dlg2.cc1, dlg2.cc2, dlg2.cc3, dlg2.cc4,
										 title,
										 wxDefaultPosition,
										 GeoDaConst::cond_view_default_size,
										 wxDEFAULT_FRAME_STYLE);
			subframe->SetSize(wxDefaultCoord, wxDefaultCoord, 800, 600);
		}
	}
}

void MyFrame::OnToolOpenNewTable(wxCommandEvent& WXUNUSED(event))
{
	OnOpenNewTable();
}

void MyFrame::OnOpenMSPL(wxCommandEvent& event)
{
	if (GetVariableSetting(true)) {
		if (!CheckDataValidity(gObservation, m_gX)) {
			wxMessageBox("Error: Chosen observation has no regression.");
			return;
		}

		GalWeight* gal = GetGal();
		if (!gal) return;
		
		m_gVar2 = "W_" + m_gVar1;

		wxString title = "Moran Scatter Plot " + wxString("(")
			+ gWeightTitle + ") - " + m_gVar1;	

		MoranScatterPlotFrame *subframe =
			new MoranScatterPlotFrame(frame,  project_p, gal,
									  false, // isMoranEBRate
									  title,
									  wxDefaultPosition,
									  GeoDaConst::scatterplot_default_size,
									  wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnOpenGMoran(wxCommandEvent& event)
{
	if (GetVariableSetting(false)) {
		if (!CheckDataValidity(gObservation, m_gX)) {
			wxMessageBox("Error: Chosen observation has no regression.");
			return;
		}
		GalWeight* gal = GetGal();
		if (!gal) return;

		m_gVar2 = "W_" + m_gVar2;

		wxString title = "Bivariate Moran " + wxString("(")
			+ gWeightTitle + ") - " + m_gVar2 + " vs " + m_gVar1;
		
		MoranGFrame *subframe = new MoranGFrame(frame, project_p,
												gal,
												title,
												wxDefaultPosition,
												wxSize(350, 350),
												wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnOpenMoranEB(wxCommandEvent& event)
{
	RateSmootherDlg RS(GetProject()->GetGridBase(),
						m_gVar1, m_gVar2, m_gX,
						m_VarDefault, 9);
	if (RS.ShowModal() != wxID_OK) return;
	
	m_gVar1 = RS.m_Var1;
	m_gVar2 = RS.m_Var2;
	m_VarDefault = (bool) RS.m_CheckDefault;

	GalWeight* gal = GetGal();
	if (!gal) return;
	
	wxString title = "Moran's I with EB Rate Standardization: "
		+ m_gVar1 + " / " + m_gVar2;
	MoranScatterPlotFrame *subframe =
		new MoranScatterPlotFrame(frame, project_p, gal,
								  true, // isMoranEBRate
								  title,
								  wxDefaultPosition,
								  GeoDaConst::scatterplot_default_size,
								  wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnOpenUniLisa(wxCommandEvent& event)
{
	if (!GetVariableSetting(true)) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
		
	CLisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;

	if (!LWO.m_BoxPlot && !LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) {
		return;
	}

	m_gVar2 = "W_" + m_gVar1;

	if (LWO.m_Moran) {
		wxString title = "Moran Scatter Plot ";
		title += "(" + gWeightTitle + ") - " + m_gVar1;
		MoranScatterPlotFrame *subframe =
			new MoranScatterPlotFrame(frame, project_p, gal,
									  false, // isMoranEBRate
									  title,
									  wxDefaultPosition,
									  GeoDaConst::scatterplot_default_size,
									  wxDEFAULT_FRAME_STYLE);
	}
		
	if (LWO.m_BoxPlot) {
		wxString title = "Local Moran (Hinge = 1.5) - ";
		title += "(" + gWeightTitle + ") - " + m_gVar1;
		LisaBoxFrame *subframe =
			new LisaBoxFrame(frame, project_p, m_gX, NULL,
							 gal,
							 false, // isMultivariateLISA
							 false, // isMoranEBRate
							 title,
							 wxDefaultPosition, wxSize(350, 350),
							 wxDEFAULT_FRAME_STYLE);
	}

	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
	LisaCoordinator* lc = new LisaCoordinator(gal, m_gX, NULL,
											  m_gVar1, m_gVar2, false);
		
	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											true, false, false,
											"Lisa Cluster Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}
		
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											false, false, false,
											"Lisa Significance Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnOpenMultiLisa(wxCommandEvent& event)
{
	if (!GetVariableSetting(false)) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
		
	CLisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;

	if (!LWO.m_BoxPlot && !LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) {
		return;
	}

	m_gVar2 = "W_" + m_gVar2;
	
	if (LWO.m_Moran) {
		wxString title = "Bivariate Moran ";
		title +=  "(" + gWeightTitle + ") - " + m_gVar2 + " vs " + m_gVar1;	
		MoranGFrame *subframe =
			new MoranGFrame(frame, project_p, gal,
							title, wxDefaultPosition,
							wxSize(350, 350), wxDEFAULT_FRAME_STYLE);
	}
		
	if (LWO.m_BoxPlot) {
		wxString title = "Bivariate LISA: ";
		title += m_gVar1 + " w/ " + m_gVar2;	
		LisaBoxFrame *subframe =
			new LisaBoxFrame(frame,  project_p, m_gX, m_gY, gal,
							 true, // isMultivariateLISA
							 false, // isMoranEBRate
							 title, wxDefaultPosition,
							 wxSize(350, 350), wxDEFAULT_FRAME_STYLE);
	}

	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
	LisaCoordinator* lc = new LisaCoordinator(gal, m_gX, m_gY,
											  m_gVar1, m_gVar2, true);

	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											true, true, false,
											"Lisa Cluster Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}
	
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											false, true, false,
											"Lisa Significance Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}
}

void MyFrame::OnOpenLisaEB(wxCommandEvent& event)
{
	RateSmootherDlg RS(GetProject()->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 9);
	if (RS.ShowModal() != wxID_OK) return;
	
	m_gVar1 = RS.m_Var1;
	m_gVar2 = RS.m_Var2;
	m_VarDefault = (bool) RS.m_CheckDefault;

	GalWeight* gal = GetGal();
	if (!gal) return;
	
	CLisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_BoxPlot && !LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) {
		return;
	}

	if (LWO.m_Moran) {
		wxString title = "Moran's I with EB Rate Standardization: ";
		title += m_gVar1 + " / "  + m_gVar2;
		MoranScatterPlotFrame *subframe =
			new MoranScatterPlotFrame(frame, project_p, gal,
									  true, // isMoranEBRate
									  title, wxDefaultPosition,
									  GeoDaConst::scatterplot_default_size,
									  wxDEFAULT_FRAME_STYLE);
	}
	
	if (LWO.m_BoxPlot) {
		wxString title = "LISA with EB Rate Standardization: ";
		title += m_gVar1 + " / " + m_gVar2;
		LisaBoxFrame *subframe =
			new LisaBoxFrame(frame, project_p, m_gX, NULL, gal,
							 false, // isMultivariateLISA
							 true, // isMoranEBRate
							 title, wxDefaultPosition,
							 wxSize(350, 350), wxDEFAULT_FRAME_STYLE);
	}

	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
	LisaCoordinator* lc = new LisaCoordinator(gal, m_gX, NULL,
											  m_gVar1, m_gVar2, false);

	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											true, false, true,
											"Lisa Cluster Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}
	
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(frame, project_p, lc,
											false, false, true,
											"Lisa Significance Map",
											wxDefaultPosition,
											GeoDaConst::map_default_size,
											wxDEFAULT_FRAME_STYLE);
	}	
}

void MyFrame::OnOpenGetisOrd(wxCommandEvent& event)
{
	if (!GetVariableSetting(true)) return;
	
	GalWeight* gal = GetGal();
	if (!gal) return;
		
	GetisOrdChoiceDlg dlg(this);
		
	if (dlg.ShowModal() != wxID_OK) return;
		
	if (!dlg.Gi_ClustMap_norm && !dlg.Gi_SigMap_norm &&
		!dlg.GiStar_ClustMap_norm && !dlg.GiStar_SigMap_norm &&
		!dlg.Gi_ClustMap_perm && !dlg.Gi_SigMap_perm &&
		!dlg.GiStar_ClustMap_perm && !dlg.GiStar_SigMap_perm) return;
		
	// m_gVar1 has field we are interested in and m_gX has
	// corresponding values
	wxString field_name = m_gVar1;
	std::vector<double> x(gObservation);
	for (int i=0; i<gObservation; i++) x[i] = m_gX[i];
	
	GStatCoordinator* gc = new GStatCoordinator(gal,
												dlg.row_standardize_weights,
												field_name, x);
	if (!gc || (gc && !gc->IsOk())) {
		// print error message
		delete gc;
		return;
	}
	
	if (dlg.Gi_ClustMap_norm) {
		GetisOrdMapFrame* f =
			new GetisOrdMapFrame(this, project_p, gc,
								 GetisOrdMapFrame::Gi_clus_norm,
								 dlg.row_standardize_weights,
								 "Local G Stats Map", wxDefaultPosition,
								 GeoDaConst::map_default_size,
								 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.Gi_SigMap_norm) {
		GetisOrdMapFrame* f =
			new GetisOrdMapFrame(this, project_p, gc,
								 GetisOrdMapFrame::Gi_sig_norm,
								 dlg.row_standardize_weights,
								 "Local G Stats Map", wxDefaultPosition,
								 GeoDaConst::map_default_size,
								 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.GiStar_ClustMap_norm) {
		GetisOrdMapFrame* f =
			new GetisOrdMapFrame(this, project_p, gc,
								 GetisOrdMapFrame::GiStar_clus_norm,
								 dlg.row_standardize_weights,
								 "Local G Stats Map", wxDefaultPosition,
								 GeoDaConst::map_default_size,
								 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.GiStar_SigMap_norm) {
		GetisOrdMapFrame* f =
			new GetisOrdMapFrame(this, project_p, gc,
								 GetisOrdMapFrame::GiStar_sig_norm,
								 dlg.row_standardize_weights,
								 "Local G Stats Map", wxDefaultPosition,
								 GeoDaConst::map_default_size,
								 wxDEFAULT_FRAME_STYLE);
	}
	
	if (dlg.Gi_ClustMap_perm) {
		GetisOrdMapFrame* f =
		new GetisOrdMapFrame(this, project_p, gc,
							 GetisOrdMapFrame::Gi_clus_perm,
							 dlg.row_standardize_weights,
							 "Local G Stats Map", wxDefaultPosition,
							 GeoDaConst::map_default_size,
							 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.Gi_SigMap_perm) {
		GetisOrdMapFrame* f =
		new GetisOrdMapFrame(this, project_p, gc,
							 GetisOrdMapFrame::Gi_sig_perm,
							 dlg.row_standardize_weights,
							 "Local G Stats Map", wxDefaultPosition,
							 GeoDaConst::map_default_size,
							 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.GiStar_ClustMap_perm) {
		GetisOrdMapFrame* f =
		new GetisOrdMapFrame(this, project_p, gc,
							 GetisOrdMapFrame::GiStar_clus_perm,
							 dlg.row_standardize_weights,
							 "Local G Stats Map", wxDefaultPosition,
							 GeoDaConst::map_default_size,
							 wxDEFAULT_FRAME_STYLE);
	}
	if (dlg.GiStar_SigMap_perm) {
		GetisOrdMapFrame* f =
		new GetisOrdMapFrame(this, project_p, gc,
							 GetisOrdMapFrame::GiStar_sig_perm,
							 dlg.row_standardize_weights,
							 "Local G Stats Map", wxDefaultPosition,
							 GeoDaConst::map_default_size,
							 wxDEFAULT_FRAME_STYLE);
	}	
}


void MyFrame::OnOpenQuantile(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnQuantile(event);
}

void MyFrame::OnQuantile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnQuantile(event);
	}
}

void MyFrame::OnOpenPercentile(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnPercentile(event);
}

void MyFrame::OnPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnPercentile(event);
	}
}

void MyFrame::OnOpenBox15(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnHinge15(event);
}

void MyFrame::OnHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnHinge15(event);
	} else if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnHinge15(event);
	} else if (LisaBoxFrame* f = dynamic_cast<LisaBoxFrame*>(t)) {
		f->OnHinge15(event);
	} else if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnHinge15(event);
	}
}

void MyFrame::OnOpenBox30(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnHinge30(event);
}

void MyFrame::OnHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnHinge30(event);
	} else if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnHinge30(event);
	} else if (LisaBoxFrame* f = dynamic_cast<LisaBoxFrame*>(t)) {
		f->OnHinge30(event);
	} else if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnHinge30(event);
	}
}

void MyFrame::OnOpenStddev(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnStddev(event);
}

void MyFrame::OnStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnStddev(event);
	}
}

void MyFrame::OnOpenNaturalBreak(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnNaturalBreak(event);
}

void MyFrame::OnNaturalBreak(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnNaturalBreak(event);
	}
}

void MyFrame::OnOpenUniqueValue(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnUniqueValue(event);
}

void MyFrame::OnUniqueValue(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnUniqueValue(event);
	}
}

void MyFrame::OnOpenEqualInterval(wxCommandEvent& event)
{
	MapFrame *subframe = new MapFrame(gCompleteFileName, frame,
									  project_p, "Canvas Frame",
									  wxDefaultPosition,
									  GeoDaConst::map_default_size,
									  wxDEFAULT_FRAME_STYLE);
	subframe->OnEqualInterval(event);
}

void MyFrame::OnEqualInterval(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnEqualInterval(event);
	}
}

void MyFrame::OnRawrate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnRawrate(event);
	}
}

void MyFrame::OnExcessrisk(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnExcessrisk(event);
	}
}

void MyFrame::OnBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnBayes(event);
	}
}

void MyFrame::OnSmoother(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSmoother(event);
	}
}

void MyFrame::OnEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnEmpiricalBayes(event);
	}
}

void MyFrame::OnSaveResults(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSaveResults(event);
	}
}

void MyFrame::OnReset(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnReset(event);
	}
}

void MyFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (HistFrame* f = dynamic_cast<HistFrame*>(t)) {
		f->OnHistogramIntervals(event);
	}
}

void MyFrame::OnRan99Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnRan99Per(event);
	}
}

void MyFrame::OnRan199Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnRan199Per(event);
	}
}

void MyFrame::OnRan499Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnRan499Per(event);
	}
}

void MyFrame::OnRan999Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnRan999Per(event);
	}
}

void MyFrame::OnRanOtherPer(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnRanOtherPer(event);
	}
}

void MyFrame::OnEnvelopeSlopes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnEnvelopeSlopes(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnEnvelopeSlopes(event);
	}
}

void MyFrame::OnSaveMoranI(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnSaveMoranI(event);
	} else if (MoranScatterPlotFrame* f =
			   dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnSaveMoranI(event);
	}
}

void MyFrame::OnSigFilter05(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter05(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter05(event);
	}
}

void MyFrame::OnSigFilter01(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter01(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter01(event);
	}
}

void MyFrame::OnSigFilter001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter001(event);
	}
}

void MyFrame::OnSigFilter0001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter0001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter0001(event);
	}
}

void MyFrame::OnAddMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnAddMeanCenters(event);
	}
}

void MyFrame::OnAddCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnAddCentroids(event);
	}
}

void MyFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSaveGetisOrd(event);
	}
}

void MyFrame::OnSaveLisa(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSaveLisa(event);
	}
}

void MyFrame::OnSelectCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCores(event);
	}
}

void MyFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	}
}

void MyFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	}
}

void MyFrame::OnAddNeighborsToSelection(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (!project_p) return;
	if (!project_p->GetWManager() ||
		(project_p->GetWManager() &&
		 !project_p->GetWManager()->GetCurrWeight())) {
		// prompt the user for a weights matrix
		GetGal();
	}
	project_p->AddNeighborsToSelection();
}

void MyFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotFrame* f = dynamic_cast<ScatterPlotFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewStandardizedData(event);
	}
	//} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnViewStandardizedData(event);
	//}	
}

void MyFrame::OnViewOriginalData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotFrame* f = dynamic_cast<ScatterPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewOriginalData(event);
	}
	//} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnViewOriginalData(event);
	//}
}

void MyFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotFrame* f = dynamic_cast<ScatterPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	} else if (MoranGFrame* f = dynamic_cast<MoranGFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	} else if (MoranScatterPlotFrame* f = 
				dynamic_cast<MoranScatterPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	}
	//} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnViewRegressionSelectedExcluded(event);
	//}
}

void MyFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	//if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnViewRegressionSelected(event);
	//}
}

void MyFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	//if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnDisplayStatistics(event);
	//}
}

void MyFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	//if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
	//	f->OnShowAxesThroughOrigin(event);
	//}
}

void MyFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event) )
{
	wxDialog dlg;
	wxXmlResource::Get()->LoadDialog(&dlg, this, "IDD_ABOUTBOX");
	dlg.ShowModal();
}

// 1-MapView. 2-Boxplot, 3-Histogram 4-Scatter Plot
int Conditionable::cViewType = 0;
int Conditionable::cLocator = 0;

// The default constructor is private and should never be called.
Conditionable::Conditionable()
: isConditional(false), conditionFlag(0)
{
}

Conditionable::Conditionable(bool conditional_view)
: isConditional(conditional_view), conditionFlag(0)
{
	//if (!isConditional) return;
	cWhere = cLocator;
	conditionFlag = new bool[gObservation];
	for (int i=0; i<gObservation; i++) conditionFlag[i] = true;
}

Conditionable::~Conditionable()
{
	if (conditionFlag) delete [] conditionFlag; conditionFlag = 0;
}

void Conditionable::UpdateCondition(int *flags)
{
	if(!isConditional) return;
	numCondObs = 0;
	for (int i=0; i<gObservation; i++) {
		if (flags[i] == cWhere) {
			conditionFlag[i] = true;
			numCondObs++;
		} else {
			conditionFlag[i] = false;
		}
	}
}

IMPLEMENT_CLASS(HiddenFrame, TemplateFrame)

IMPLEMENT_CLASS(HiddenCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(HiddenCanvas, TemplateCanvas)
	EVT_PAINT(HiddenCanvas::OnPaint) // new
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground) // new
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent) // new
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent) // new
END_EVENT_TABLE()

HiddenCanvas::HiddenCanvas(wxWindow *parent, const wxPoint& pos,
							 const wxSize& size)
: TemplateCanvas(parent, pos, size)
{
	LOG_MSG("Entering HiddenCanvas::HiddenCanvas");
	highlight_state->registerObserver(this);
	LOG_MSG("Exiting HiddenCanvas::HiddenCanvas");
}


HiddenCanvas::~HiddenCanvas()
{
	LOG_MSG("Entering HiddenCanvas::~HiddenCanvas()");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting HiddenCanvas::~HiddenCanvas()");
}

void HiddenCanvas::OnPaint(wxPaintEvent& event)
{
	// uncomment this if we want the Paint Event to be passed
	// to the Paint Event handler in Template Canvas.
	//event.Skip();  
}

void HiddenCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering HiddenCanvas::update");
	
	std::vector<bool>& hl = highlight_state->GetHighlight();
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int nh_cnt = highlight_state->GetTotalNewlyHighlighted();
	int nuh_cnt = highlight_state->GetTotalNewlyUnhighlighted();
	
	HighlightState::EventType type = highlight_state->GetEventType();
	if (type == HighlightState::delta) {
		LOG(nh_cnt);
		LOG(nuh_cnt);
		if (nh_cnt > 0) {
			LOG_MSG(wxString::Format("ADD_SELECTION with %d elements",
									 nh_cnt));
			gSelection.Reset(true);
			gEvent = ADD_SELECTION;
			for (int i=0; i<nh_cnt; i++) {
				gSelection.Push(nh[i]);
			}
			// Send Update message to Selection class listeners, but 
			// specify not to be updated ourself.
			gSelection.Update();
			MyFrame::theFrame->UpdateWholeView((wxFrame*) GetParent());
			gSelection.Reset(true);
		}
		if (nuh_cnt > 0) {
			LOG_MSG(wxString::Format("DEL_SELECTION with %d elements",
									 nuh_cnt));
			gSelection.Reset(true);
			gEvent = DEL_SELECTION;
			for (int i=0; i<nuh_cnt; i++) {
				gSelection.Push(nuh[i]);
			}
			// Send Update message to Selection class listeners, but
			// specify not to be updated ourself.
			gSelection.Update();
			MyFrame::theFrame->UpdateWholeView((wxFrame*) GetParent());
			gSelection.Reset(true);
		}		
	} else if (type == HighlightState::highlight_all) {
		gSelection.Reset(true);
		gEvent = ADD_SELECTION;
		for (int i=0, iend=gSelection.GetBitmapSize(); i<iend; i++) {
			if (gSelection.selected(i)) gSelection.Push(nh[i]);
		}
		gSelection.Update();
		MyFrame::theFrame->UpdateWholeView((wxFrame*) GetParent());
		gSelection.Reset(true);
	} else if (type == HighlightState::unhighlight_all) {
		gSelection.Reset(true);
		gEvent = NEW_SELECTION;
		gSelection.Update();
		MyFrame::theFrame->UpdateWholeView((wxFrame*) GetParent());
		gSelection.Reset(true);
	} else if (type == HighlightState::invert) {
		gSelection.Reset(true);
		gSelection.Invert();
		gEvent = NEW_SELECTION;
		MyFrame::theFrame->UpdateWholeView(NULL); 
		gSelection.Reset(true);
	}
		
	LOG_MSG("Exiting HiddenCanvas::update");
}


HiddenFrame::HiddenFrame(wxFrame *parent, Project* project,
						 const wxString& title,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering HiddenFrame::HiddenFrame");
	int width, height;
	GetClientSize(&width, &height);
	canvas = new HiddenCanvas(this, wxDefaultPosition, wxSize(width,height));
	template_canvas = canvas;
	template_canvas->template_frame = this;
	Show(false);
	LOG_MSG("Exiting HiddenFrame::HiddenFrame");
}

HiddenFrame::~HiddenFrame()
{
	LOG_MSG("In HiddenFrame::~HiddenFrame()");
	MyFrame::hidden_frame = 0;
}

/** 
 Propogate Update() call to HighlightState.  Note, the Map view is not
 processing the ADD_SELECTION / DEL_SELECTION gEvent properly.  Other
 views such as BoxPlot and Cartogram work fine.
 */
void HiddenFrame::Update()
{
	LOG_MSG("Entering HiddenFrame::Update");
	
	// We need to translate the Selection class selected objects into
	// the HighlightState class selected objects.  
	
	HighlightState* highlight_state = MyFrame::GetProject()->highlight_state;
	std::vector<bool>& hl = highlight_state->GetHighlight();
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int nh_cnt = 0;
	int nuh_cnt = 0;
	bool* bm = gSelection.GetBitmap();

	// for debugging
	//for (int i=0; i<gSelection.GetBitmapSize(); i++) {
	//	int hli = hl[i] ? 1 : 0;
	//	wxString msg = wxString::Format("bm[%d] = %d, hl[%d] = %d",
	//									i,bm[i],i,hli);
	//	LOG_MSG(msg);
	//}
	
	if (gEvent == NEW_SELECTION) {
		LOG_MSG("gEvent == NEW_SELECTION");
		// make everything match what's in the Selection::bitmap array
		for (int i=0; i<hl_size; i++) {
			if (bm[i] && !hl[i]) {
				hl[i] = true;
				nh[nh_cnt++] = i;
			} else if (!bm[i] && hl[i]) {
				hl[i] = false;
				nuh[nuh_cnt++] = i;
			}
		}
	} else if (gEvent == ADD_SELECTION) {
		LOG_MSG("gEvent == ADD_SELECTION");
		// select all items on change stack
		int item = gSelection.Pop();
		while (item != GeoDaConst::EMPTY) {
			if (!hl[item]) {
				hl[item] = true;
				nh[nh_cnt++] = item;
			}
			item = gSelection.Pop();
		}
	} else if (gEvent == DEL_SELECTION) {
		LOG_MSG("gEvent == DEL_SELECTION");
		// unselect all items on the change stack
		int item = gSelection.Pop();
		while (item != GeoDaConst::EMPTY) {
			if (hl[item]) {
				hl[item] = false;
				nuh[nuh_cnt++] = item;
			}
			item = gSelection.Pop();
		}		
	} else {
		LOG_MSG("gEvent == unknown!");
	}
	gSelection.Reset();

	LOG(nh_cnt);
	LOG(nuh_cnt);
	highlight_state->SetTotalNewlyHighlighted(nh_cnt);
	highlight_state->SetTotalNewlyUnhighlighted(nuh_cnt);
	highlight_state->SetEventType(HighlightState::delta);

	// Send update message to HighlightState class Observers, but specify
	// not to receive update message ourself.
	if (nh_cnt > 0 || nuh_cnt > 0) {
		MyFrame::GetProject()->highlight_state->notifyObservers(canvas);
	} else {
		LOG_MSG("No select/highlight changes");
	}
		
	LOG_MSG("Exiting HiddenFrame::Update");
}

