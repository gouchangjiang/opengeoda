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

#include "DialogTools/PCPDlg.h"
#include "DialogTools/FieldNewCalcSheetDlg.h"
#include "DataViewer/DbfGridTableBase.h"
#include "DataViewer/DataViewerAddColDlg.h"
#include "DataViewer/DataViewerDeleteColDlg.h"
#include "DataViewer/DataViewerEditFieldPropertiesDlg.h"
#include "DataViewer/MergeTableDlg.h"
#include "DialogTools/CreateSpTmProjectDlg.h"
#include "DialogTools/ExportCsvDlg.h"
#include "DialogTools/ImportCsvDlg.h"
#include "DialogTools/RangeSelectionDlg.h"
#include "DialogTools/OpenSpaceTimeDlg.h"
#include "DialogTools/TimeChooserDlg.h"
#include "DialogTools/TimeVariantImportDlg.h"
#include "DialogTools/VariableSettingsDlg.h"
#include "ShapeOperations/CsvFileUtils.h"
#include "ShapeOperations/shp.h"
#include "ShapeOperations/shp2cnt.h"
#include "ShapeOperations/ShpFile.h"
#include "ShapeOperations/ShapeUtils.h"
#include "FramesManager.h"
#include "OpenGeoDa.h"
#include "NewTableViewer.h"
#include "TemplateCanvas.h"
#include "Thiessen/VorDataType.h"
#include "DialogTools/ConditionViewDlg.h"
#include "DialogTools/Statistics.h"
#include "DialogTools/SaveSelectionDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionTitleDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "Explore/GetisOrdMapNewView.h"
#include "DialogTools/LisaWhat2OpenDlg.h"
#include "Explore/LisaMapNewView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/CartogramView.h"
#include "Explore/ScatterPlotView.h"
#include "Explore/ScatterNewPlotView.h"
#include "mapview.h"
#include "Explore/MapNewView.h"
//#include "Generic/TestMapView.h"
//#include "Generic/TestTableView.h"
//#include "Generic/TestScrollWinView.h"
#include "Explore/PCPNewView.h"
#include "Explore/HistView.h"
#include "Explore/HistogramView.h"
#include "Explore/BoxNewPlotView.h"
#include "Explore/3DPlotView.h"
#include "Explore/ConditionalView.h"
#include "Regression/DiagnosticReport.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/ProgressDlg.h"
#include "DialogTools/GetisOrdChoiceDlg.h"
#include "Explore/GStatCoordinator.h"
#include "ShapeOperations/WeightsManager.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include "GeoDaConst.h"
#include "logger.h"

// The following is defined in rc/MyAppResouces.cpp.  This file was
// compiled with:
/*
 wxrc dialogs.xrc menus.xrc toolbar.xrc \
   --cpp-code --output=MyAppResources.cpp --function=MyInitXmlResource
*/
// and combines all resouces file into single source file that is linked into
// the application binary.
extern void MyInitXmlResource();

MyFrame* frame = 0;
MyFrame* MyFrame::theFrame = 0;
bool MyFrame::projectOpen = false;

// Will go away once everthing is converted to using HighlightState
//extern GeoDaEventType gEvent;
/** The globally defined instance of GeoDaEventType, gEvent. */
GeoDaEventType	gEvent = NO_EVENTS;
Selection gSelection;  // the currently selected objects, an array of bool

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
		// The default is assumed to be Vista / Win 7 family, but can check with
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
	
	if (GeneralWxUtils::isWindows()) {
		// For XP / Vista / Win 7, the user can select to use font sizes of %100, %125 or %150.
		// Therefore, we might need to slighly increase the window size when sizes
		// > %100 are used in the Display options.
		LOG(frame->GetSize().GetHeight());
		LOG(frame->GetClientSize().GetHeight());
		if (frame->GetClientSize().GetHeight() < 22) {
			frame->SetSize(frame->GetSize().GetWidth(),
				frame->GetSize().GetHeight() + (22 - frame->GetClientSize().GetHeight()));
			LOG(frame->GetClientSize().GetHeight());
		}
	}

	// The following code will insert a "Test Map Frame" menu item
	// under the file menu.  Comment this section out when not
	// testing.
	// BEGIN TestMapFrame TEST
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int fm_index = mb->FindMenu("File");
	wxMenu* fm = mb->GetMenu(fm_index);
	//fm->Append(ID_TEST_MAP_FRAME, "Test Map Frame",
	//		   "Open a test frame");
	//GeneralWxUtils::EnableMenuItem(mb, "File",
	//							   ID_TEST_MAP_FRAME, true);
	//fm->Append(ID_TEST_TABLE_FRAME, "Test Table",
	//		   "Open a test table");
	//GeneralWxUtils::EnableMenuItem(mb, "File",
	//							   ID_TEST_TABLE_FRAME, false);
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
	EVT_CHAR_HOOK(MyFrame::OnKeyEvent)
	EVT_MENU(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_TOOL(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_BUTTON(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_MENU(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_TOOL(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_BUTTON(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_MENU(XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),
			 MyFrame::OnOpenSpTmShapefile)
	EVT_MENU(XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),
			 MyFrame::OnOpenSpTmTableOnly)
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

	EVT_MENU(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
	EVT_TOOL(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
	EVT_BUTTON(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
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
	EVT_TOOL(XRCID("ID_OPEN_CHOICES"), MyFrame::OnOpenChoices)

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
	EVT_MENU(XRCID("ID_POINTS_FROM_TABLE"), MyFrame::OnGeneratePointShpFile)
 
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
	EVT_MENU(XRCID("ID_SHOW_TIME_CHOOSER"), MyFrame::OnShowTimeChooser)
	EVT_MENU(XRCID("ID_SPACE_TIME_TOOL"), MyFrame::OnSpaceTimeTool)
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
	EVT_MENU(XRCID("ID_SAVE_PROJECT"),
			 MyFrame::OnSaveProject)
	EVT_MENU(XRCID("ID_SAVE_AS_PROJECT"),
			 MyFrame::OnSaveAsProject)
	EVT_MENU(XRCID("ID_EXPORT_TO_CSV_FILE"),
			 MyFrame::OnExportToCsvFile)
	EVT_MENU(XRCID("ID_ADD_NEIGHBORS_TO_SELECTION"),
			 MyFrame::OnAddNeighborsToSelection)

	EVT_MENU(XRCID("ID_REGRESSION_CLASSIC"), MyFrame::OnRegressionClassic)

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
	EVT_MENU(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_MENU(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_MENU(XRCID("IDM_SCATTER_NEW_PLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_TOOL(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_TOOL(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_BUTTON(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_BUTTON(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_MENU(ID_TEST_MAP_FRAME, MyFrame::OnExploreTestMap)
	EVT_MENU(ID_TEST_TABLE_FRAME, MyFrame::OnExploreTestTable)
	EVT_MENU(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
	EVT_TOOL(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
	EVT_BUTTON(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
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
	EVT_MENU(XRCID("ID_DISPLAY_MEAN_CENTERS"), MyFrame::OnDisplayMeanCenters)
	EVT_MENU(XRCID("ID_DISPLAY_CENTROIDS"), MyFrame::OnDisplayCentroids)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnOpenThemelessMap)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnOpenThemelessMap)
	EVT_MENU(XRCID("ID_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnThemelessMap)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnOpenQuantile)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnOpenQuantile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnQuantile)
	
	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnOpenPercentile)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnOpenPercentile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnPercentile)
	
	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), MyFrame::OnOpenHinge15)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), MyFrame::OnOpenHinge15)
	EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_15"), MyFrame::OnHinge15)
	
	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), MyFrame::OnOpenHinge30)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), MyFrame::OnOpenHinge30)
	EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_30"), MyFrame::OnHinge30)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnOpenStddev)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnOpenStddev)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnStddev)
	
	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnOpenNaturalBreaks)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnOpenNaturalBreaks)
	EVT_MENU(XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnNaturalBreaks)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnOpenUniqueValues)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnOpenUniqueValues)
	EVT_MENU(XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnUniqueValues)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnOpenEqualIntervals)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnOpenEqualIntervals)
	EVT_MENU(XRCID("ID_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnEqualIntervals)
	EVT_MENU(XRCID("ID_SAVE_CATEGORIES"),
			 MyFrame::OnSaveCategories)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), MyFrame::OnOpenRawrate)
	EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), MyFrame::OnOpenRawrate)
	EVT_MENU(XRCID("ID_RATES_SMOOTH_RAWRATE"), MyFrame::OnRawrate)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnOpenExcessrisk)
	EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnOpenExcessrisk)
	EVT_MENU(XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnExcessrisk)

	EVT_TOOL(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnOpenEmpiricalBayes)
	EVT_MENU(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnOpenEmpiricalBayes)
	EVT_MENU(XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnEmpiricalBayes)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnOpenSpatialRate)
	EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnOpenSpatialRate)
	EVT_MENU(XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnSpatialRate)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnOpenSpatialEmpiricalBayes)
	EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnOpenSpatialEmpiricalBayes)
	EVT_MENU(XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnSpatialEmpiricalBayes)

	EVT_MENU(XRCID("ID_MAPANALYSIS_SAVERESULTS"), MyFrame::OnSaveResults)

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
	EVT_MENU(XRCID("ID_SHOW_AXES"), MyFrame::OnShowAxes)

	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR1, MyFrame::OnTimeSyncVariable1)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR2, MyFrame::OnTimeSyncVariable2)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR3, MyFrame::OnTimeSyncVariable3)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR4, MyFrame::OnTimeSyncVariable4)

	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
			 MyFrame::OnFixedScaleVariable1)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR2,
			 MyFrame::OnFixedScaleVariable2)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR3,
			 MyFrame::OnFixedScaleVariable3)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR4,
			 MyFrame::OnFixedScaleVariable4)

	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_1, MyFrame::OnPlotsPerView1)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_2, MyFrame::OnPlotsPerView2)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_3, MyFrame::OnPlotsPerView3)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_4, MyFrame::OnPlotsPerView4)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_5, MyFrame::OnPlotsPerView5)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_6, MyFrame::OnPlotsPerView6)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_7, MyFrame::OnPlotsPerView7)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_8, MyFrame::OnPlotsPerView8)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_9, MyFrame::OnPlotsPerView9)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_10, MyFrame::OnPlotsPerView10)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_OTHER, MyFrame::OnPlotsPerViewOther)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_ALL, MyFrame::OnPlotsPerViewAll)

	EVT_MENU(XRCID("ID_DISPLAY_STATUS_BAR"), MyFrame::OnDisplayStatusBar)

	EVT_MENU(XRCID("wxID_ABOUT"), MyFrame::OnHelpAbout)
END_EVENT_TABLE()


void MyFrame::UpdateToolbarAndMenus()
{
	// This method is called when no particular window is currently active.
	// In this case, the close menu item should be disabled.

	LOG_MSG("In MyFrame::UpdateToolbarAndMenus");
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), "File", wxID_CLOSE,
								   false);

	Project* p = GetProject();
	bool proj_open = (p != 0);
	bool shp_proj = proj_open && !p->IsTableOnlyProject();
	bool table_proj = proj_open && p->IsTableOnlyProject();
	bool time_variant = (proj_open && p->GetGridBase()->IsTimeVariant());

	wxMenuBar* mb = GetMenuBar();

	// Reset the toolbar frame title to default.
	SetTitle("OpenGeoDa");
	
	//MMM: the following two item states are set elsewhere.  This should be
	// unified.
	EnableTool(XRCID("ID_OPEN_CHOICES"), !proj_open);
	EnableTool(XRCID("ID_CLOSE_ALL"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_SHAPE_FILE"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_TABLE_ONLY"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),
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
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_POINTS_FROM_TABLE"), table_proj);
	
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
	EnableTool(XRCID("IDM_BUBBLECHART"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BUBBLECHART"), proj_open);
	
	EnableTool(XRCID("IDM_PCP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_PCP"), proj_open);

	EnableTool(XRCID("IDM_CC"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_CC"), shp_proj);

	EnableTool(XRCID("IDM_3DP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_3DP"), proj_open);

	EnableTool(XRCID("IDM_NEW_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, "Table", proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_TIME_CHOOSER"),
								   time_variant);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SPACE_TIME_TOOL"),
								   proj_open);
	mb->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
				 (time_variant ? "Space-Time Variable Creation Tool" :
				  "Convert to Space-Time Project"));
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_PROJECT"),
			proj_open && project_p->GetGridBase()->ChangedSinceLastSave() &&
			project_p->IsAllowEnableSave());
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_AS_PROJECT"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_TO_CSV_FILE"),
								   proj_open);
	
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
	
	GeneralWxUtils::EnableMenuAll(mb, "Map", shp_proj);
	EnableTool(XRCID("ID_MAP_CHOICES"), shp_proj);
	EnableTool(XRCID("ID_MAPANALYSIS_MAPMOVIE"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CARTOGRAM_MAP"), shp_proj);
	
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

bool MyFrame::IsProjectOpen()
{
	return projectOpen;
}

void MyFrame::SetProjectOpen(bool open)
{
	projectOpen = open;
}

#include "DialogTools/SelectWeightDlg.h"

GalWeight* MyFrame::GetGal()
{
	if (!project_p->GetWManager()->IsDefaultWeight()) {
		SelectWeightDlg dlg(project_p, this);
		if (dlg.ShowModal()!= wxID_OK) return 0;
	}
	GeoDaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeoDaWeight::gal_type) {
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
	tvf->Maximize(false);
	tvf->Raise();
	
	LOG_MSG("Exiting MyFrame::OnOpenNewTable");
}

/** returns false if user wants to abort the operation */
bool MyFrame::OnCloseMap(bool ignore_unsaved_changes)
{
	LOG_MSG("Entering MyFrame::OnCloseMap");
	
	wxString msg;
	if (IsProjectOpen() && !ignore_unsaved_changes &&
		project_p->GetGridBase()->ChangedSinceLastSave()) {
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
	
	TemplateFrame::my_children.Clear();
	
	if (project_p && project_p->regression_dlg) {
		((wxDialog*) project_p->regression_dlg)->Close(true);
		project_p->regression_dlg = 0;
	}
	if (project_p) delete project_p; project_p = 0;
	if (hidden_frame) hidden_frame->Destroy(); hidden_frame = 0;
	
	UpdateToolbarAndMenus();
	EnableTool(XRCID("ID_OPEN_CHOICES"), true);
	EnableTool(XRCID("ID_CLOSE_ALL"), false);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),true);
	
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), false);
	
	return true;
}

void MyFrame::OnOpenShapefile(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(false, false);
}

void MyFrame::OnOpenTableOnly(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(true, false);
}

void MyFrame::OnOpenSpTmShapefile(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(false, true);
}

void MyFrame::OnOpenSpTmTableOnly(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(true, true);
}

void MyFrame::OpenProject(bool table_only, bool space_time)
{
	LOG_MSG("Entering MyFrame::OpenProject");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseMap()");
		if (!OnCloseMap()) return;
	}
	
	DbfGridTableBase* grid_base = 0;
	bool is_csv_file = false;
	if (!space_time) {
		wxFileName ifn;
		if (!table_only) {
			wxFileDialog dlg(this, "Choose a Shapefile to open", "", "",
							 "Shapefiles (*.shp)|*.shp");
	
			if (dlg.ShowModal() != wxID_OK) return;
		
			// dlg.GetPath returns the selected filename with complete path.
			ifn = wxFileName(dlg.GetPath());
			// ifn.GetFullPath returns the filename with path and extension.
		
			if (IsLineShapeFile(ifn.GetFullPath())) {
				wxMessageBox("Error: OpenGeoDa does not support Shapefiles "
							 "with line data at this time.  Please choose a "
							 "Shapefile with either point or polygon data.");
				return;
			}
		
			bool shx_found;
			bool dbf_found;
			if (!GenUtils::ExistsShpShxDbf(ifn, 0, &shx_found, &dbf_found)) {
				wxString msg;
				msg << "Error: " <<  ifn.GetName() << ".shp, ";
				msg << ifn.GetName() << ".shx, and " << ifn.GetName();
				msg << ".dbf were not found together in the same file ";
				msg << "directory. Could not find ";
				if (!shx_found && dbf_found) {
					msg << ifn.GetName() << ".shx.";
				} else if (shx_found && !dbf_found) {
					msg << ifn.GetName() << ".dbf.";
				} else {
					msg << ifn.GetName() << ".shx and " << ifn.GetName();
					msg << ".dbf.";
				}
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		} else {
			//wxFileDialog dlg(this, "Choose a Table DBF file to open", "", "",
			//				 "DBF files (*.dbf)|*.dbf");
			wxFileDialog dlg(this, "Choose a Table file to open", "", "",
							 "DBF or CSV files (*.dbf;*.csv)|*.dbf;*.csv");			
			if (dlg.ShowModal() != wxID_OK) return;
			
			// dlg.GetPath returns the selected filename with complete path.
			ifn = wxFileName(dlg.GetPath());
			is_csv_file = (ifn.GetExt().CmpNoCase("dbf") != 0);
		}		
		
		if (!is_csv_file) {
			DbfFileReader dbf_reader(ifn.GetPathWithSep()+ifn.GetName()+".dbf");
			if (!dbf_reader.isDbfReadSuccess()) {
				wxString msg("There was a problem reading in the DBF file.");
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		
			project_p = new Project(dbf_reader.getNumRecords());
			grid_base = new DbfGridTableBase(dbf_reader,
											 project_p->highlight_state);
			if (!table_only) {
				project_p->Init(grid_base, ifn);
			} else {
				project_p->Init(grid_base);
			}
		} else {
			std::string csv_fname(ifn.GetFullPath().ToStdString());
			std_str_array_type string_table;
			wxString err_msg;
			bool r;
			int num_rows;
			int num_cols;
			std::vector<std::string> first_row;
			r = GeoDa::GetCsvStats(csv_fname, num_rows, num_cols, first_row,
								   err_msg);
			if (!r) {
				wxString msg("There was a problem reading in the CSV: ");
				msg << err_msg;
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			
			// need to ask user if first_row contains valid field names
			
			ImportCsvDlg dlg(this, first_row);
			if (dlg.ShowModal() != wxID_OK) return;
			
			bool first_row_field_names = dlg.contains_var_names;
			if (first_row_field_names) num_rows--;
			if (!first_row_field_names) first_row.clear();
			
			r = GeoDa::FillStringTableFromCsv(csv_fname, string_table,
											  first_row_field_names, err_msg);
			if (!r) {
				wxString msg("There was a problem reading in the CSV: ");
				msg << err_msg;
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			
			project_p = new Project(num_rows);
			grid_base = new DbfGridTableBase(string_table,
											 first_row,
											 csv_fname,
											 project_p->highlight_state);
			project_p->Init(grid_base);
		}
	} else {
		OpenSpaceTimeDlg dlg(table_only);
		if (dlg.ShowModal() != wxID_OK) return;
		
		DbfFileReader dbf_sp(dlg.time_invariant_dbf_name.GetFullPath());
		DbfFileReader dbf_tm(dlg.time_variant_dbf_name.GetFullPath());
		
		project_p = new Project(dbf_sp.getNumRecords());
		grid_base = new DbfGridTableBase(dbf_sp, dbf_tm,
										 dlg.sp_table_space_col,
										 dlg.tm_table_space_col,
										 dlg.tm_table_time_col,
										 project_p->highlight_state);
		if (!table_only) {
			wxFileName shp_fname = dlg.time_invariant_dbf_name;
			shp_fname.SetExt("shp");
			project_p->Init(grid_base, shp_fname);
		} else {
			project_p->Init(grid_base);
		}
	}
		
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
	if (table_only) tvf->Show(true);
		
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
		
	// This will go away as soon as everything starts using
	// the new Highlight State
	gSelection.Init(grid_base->GetNumberRows()+1); // Initialize the bitmap
	gSelection.Update();
	
	if (!table_only) {
		MapNewFrame* nf = new MapNewFrame(frame, project_p,
										  ThemeUtilities::no_theme,
										  MapNewCanvas::no_smoothing,
										  wxDefaultPosition,
										  GeoDaConst::map_default_size);
		nf->UpdateTitle();
	}
	
	EnableTool(XRCID("ID_OPEN_CHOICES"), false);
	EnableTool(XRCID("ID_CLOSE_ALL"), true);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"), false);	
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), true);
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_SHOW_TIME_CHOOSER"), space_time);
	mb->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
				 (space_time ? "Space-Time Variable Creation Tool" :
				  "Convert to Space-Time Project"));
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_NEW_TABLE_MERGE_TABLE_DATA"),
								   !space_time);
	
	// HidenFrame can go away as soon as everything is convernted to
	// using HighlightState
	if (hidden_frame) hidden_frame->Destroy(); hidden_frame = 0;
	hidden_frame = new HiddenFrame(frame, project_p,
								   "Hidden Frame",
								   wxDefaultPosition,
								   GeoDaConst::map_default_size,
								   wxDEFAULT_FRAME_STYLE);

	LOG_MSG("Exiting MyFrame::OpenProject");
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
	
	if (IsProjectOpen()) {
		wxMessageDialog msgDlg(this, msg, title,
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	
		// Show the message dialog, and if it returns wxID_YES...
		if (msgDlg.ShowModal() != wxID_YES) return;
	}
	OnCloseMap(true);
	Destroy();

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
	t->OnSelectWithRect(event);
}

void MyFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithCircle(event);
}

void MyFrame::OnSelectWithLine(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithLine(event);
}

void MyFrame::OnSelectionMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectionMode(event);
}

void MyFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFitToWindowMode(event);
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
	// MMM: Not implemented in new style framework
}

void MyFrame::OnZoomOut(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	// MMM: Not implemented in new style framework
}


void MyFrame::OnPanMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPanMode(event);
}

void MyFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called MyFrame::OnPrintCanvasState");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnPrintCanvasState(event);
	
	// Add this menu item to the XRC file to see this debugging option:
	//<object class="wxMenuItem" name="ID_PRINT_CANVAS_STATE">
	//  <label>Print Canvas State to Log File</label>
    //</object>
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
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
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
	t->OnSelectableFillColor(event);	
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
	t->OnSelectableOutlineVisible(event);
}

void MyFrame::OnHighlightColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnHighlightColor(event);
}

void MyFrame::OnSetDefaultVariableSettings(wxCommandEvent& WXUNUSED(event) )
{	
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::quadvariate, true);
	VS.ShowModal();
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

void MyFrame::OnKeyEvent(wxKeyEvent& event)
{
	Project* project = GetProject();
	if (event.GetModifiers() == wxMOD_CMD &&
		project && project->GetGridBase() &&
		project->GetGridBase()->IsTimeVariant() &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		DbfGridTableBase* grid_base = project->GetGridBase();
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		LOG(del);
		grid_base->curr_time_step = grid_base->curr_time_step + del;
		if (grid_base->curr_time_step < 0) {
			grid_base->curr_time_step = grid_base->time_steps-1;
		} else if (grid_base->curr_time_step >= grid_base->time_steps) {
			grid_base->curr_time_step = 0;
		}
		if (project->GetFramesManager()) {
			project->GetFramesManager()->notifyObservers();
		}
		return;
	}
	event.Skip();
}

void MyFrame::OnToolsWeightsOpen(wxCommandEvent& WXUNUSED(event) )
{
	SelectWeightDlg dlg(project_p, this);
	if (dlg.ShowModal()!= wxID_OK) return;
	GeoDaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeoDaWeight::gal_type) {
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
	WeightCharacterDlg dlg(this, project_p->GetGridBase());
	if(dlg.ShowModal() == wxID_OK) {
		HistFrame *subframe = new HistFrame(frame,
											0, project_p->GetNumRecords(), "",
											project_p,
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

void MyFrame::OnOpenChoices(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnOpenChoices");
	wxMenu* popupMenu = 0;
	popupMenu = wxXmlResource::Get()->LoadMenu("ID_OPEN_CHOICES_MENU");
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnOpenChoices");
}


#include "Thiessen/Thiessen.h"
#include "DialogTools/ThiessenPolygonDlg.h"
void MyFrame::OnShapePointsToPolygons(wxCommandEvent& WXUNUSED(event) )
{
	ThiessenPolygonDlg dlg(false, true, this);
	dlg.ShowModal();

}
void MyFrame::OnShapePolygonsToCentroids(wxCommandEvent& WXUNUSED(event) )
{
	ThiessenPolygonDlg dlg(false, false, this);
	dlg.ShowModal();
}
void MyFrame::OnShapePolygonsToMeanCenters(wxCommandEvent& WXUNUSED(event))
{
	ThiessenPolygonDlg dlg(true, false, this);
	dlg.ShowModal();
}


#include "DialogTools/DBF2SHPDlg.h"
void MyFrame::OnShapePointsFromDBF(wxCommandEvent& WXUNUSED(event) )
{
	DBF2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/ASC2SHPDlg.h"
void MyFrame::OnShapePointsFromASCII(wxCommandEvent& WXUNUSED(event) )
{
	ASC2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/CreateGridDlg.h"
void MyFrame::OnShapePolygonsFromGrid(wxCommandEvent& WXUNUSED(event) )
{
	CreateGridDlg dlg(this);
	dlg.ShowModal();
}


#include "DialogTools/Bnd2ShpDlg.h"
void MyFrame::OnShapePolygonsFromBoundary(wxCommandEvent& WXUNUSED(event) )
{
	Bnd2ShpDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/SHP2ASCDlg.h" 
void MyFrame::OnShapeToBoundary(wxCommandEvent& WXUNUSED(event) )
{
	SHP2ASCDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/Dbf2GaussDlg.h" 
void MyFrame::OnExportDataToASCII(wxCommandEvent& WXUNUSED(event) )
{
	Dbf2GaussDlg dlg(0,this);
	dlg.ShowModal();
}


#include "DialogTools/AddCentroidsDlg.h"
void MyFrame::OnToolsDataExportMeanCenters(wxCommandEvent& WXUNUSED(event) )
{
	AddCentroidsDlg dlg(true, this);
	dlg.ShowModal();
}

void MyFrame::OnToolsDataExportCentroids(wxCommandEvent& WXUNUSED(event) )
{
	AddCentroidsDlg dlg(false, this);
	dlg.ShowModal();
}

void MyFrame::OnShowTimeChooser(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetGridBase()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		if (TimeChooserDlg* w = dynamic_cast<TimeChooserDlg*>(*it)) {
			LOG_MSG("TimeChooserDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new TimeChooserDlg");
	TimeChooserDlg* dlg = new TimeChooserDlg(0,
											 project_p->GetFramesManager(),
											 project_p->GetGridBase());
	dlg->Show(true);
}

void MyFrame::OnSpaceTimeTool(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	if (project_p->GetGridBase()->IsTimeVariant()) {
		if (!project_p || !project_p->GetGridBase()) return;
		TimeVariantImportDlg dlg(GetProject()->GetGridBase(), this);
		if (dlg.ShowModal() != wxID_OK) return;	
	} else {
		wxString dup_name;
		if (GetProject()->GetGridBase()->IsDuplicateColNames(dup_name)) {
			wxString msg;
			msg << "Two or more fields share the name " << dup_name;
			msg << " in the Table. This is technically not permitted in the";
			msg << " DBF standard. Please use Table > Edit ";
			msg << " Variable Properties to remove all duplicate";
			msg << " field names then use Table > Save to";
			msg << " save changes before proceeding.";
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		CreateSpTmProjectDlg dlg(this, GetProject());
		if (dlg.ShowModal() == wxID_OK) {
			OnSpaceTimeTool(event);
		}
		UpdateToolbarAndMenus();
	}
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
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
						project_p->GetGridBase()->ChangedSinceLastSave() &&
						project_p->IsAllowEnableSave());
}

void MyFrame::OnFieldCalculation(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	FieldNewCalcSheetDlg dlg(project_p, this,
							 wxID_ANY, "Variable Calculation",
							 wxDefaultPosition,
							 wxSize(700, 500) );
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnAddCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerAddColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnDeleteCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerDeleteColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnEditFieldProperties(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerEditFieldPropertiesDlg dlg(project_p->GetGridBase(),
										 wxDefaultPosition,
										 wxSize(600, 400));
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnMergeTableData(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	MergeTableDlg dlg(project_p->GetGridBase(), wxDefaultPosition);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnSaveProject(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	if (project_p->GetGridBase()->IsTimeVariant()) {
		SaveTableSpaceTime();
	} else {
		SaveTableSpace();
	}
}

void MyFrame::OnSaveAsProject(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	// if Table Only, then will offer to save as DBF
	// if also Shapefile, then will ask for shapefile
	// name, but we will have to see if we can create
	// .shx, .dbf and .shp and ask for overwrite permission
	// for each if needed.
	
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	bool table_only = project_p->IsTableOnlyProject();
	bool space_time = grid_base->IsTimeVariant();
	
	wxString file_dlg_title = (table_only ? "DBF Name to Save As" :
							   "Shapefile Name to Save As");
	wxString file_dlg_type = (table_only ? "DBF files (*.dbf)|*.dbf" :
							  "SHP files (*.shp)|*.shp");
	wxFileDialog dlg(this, file_dlg_title, wxEmptyString, wxEmptyString,
					 file_dlg_type, wxFD_SAVE);
	
	if (dlg.ShowModal() != wxID_OK) return;
	wxFileName new_fname = dlg.GetPath();
	wxString new_main_dir = new_fname.GetPathWithSep();
	wxString new_main_name = new_fname.GetName();
	
	wxString new_sp_dbf = new_main_dir + new_main_name + ".dbf";
	wxString new_tm_dbf = new_main_dir + new_main_name + "_time.dbf";
	wxString new_shp = new_main_dir + new_main_name + ".shp";
	wxString new_shx = new_main_dir + new_main_name + ".shx";
	
	wxString cur_sp_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	wxString cur_tm_dbf;
	if (space_time) {
		cur_tm_dbf = grid_base->GetTimeDbfFileName().GetFullPath();
	}
	wxString cur_shp = project_p->GetMainDir()+project_p->GetMainName()+".shp";
	wxString cur_shx = project_p->GetMainDir()+project_p->GetMainName()+".shx";
	
	// Prompt for overwrite permissions
	// in each case, if new file name equals current file name, don't
	// need to ask permission.
	std::vector<wxString> overwrite_list;
	if ((new_sp_dbf.CmpNoCase(cur_sp_dbf) != 0) && wxFileExists(new_sp_dbf)) {
		overwrite_list.push_back(new_sp_dbf);
	}
	if (space_time &&
		(new_tm_dbf.CmpNoCase(cur_tm_dbf) != 0) && wxFileExists(new_tm_dbf)) {
		overwrite_list.push_back(new_tm_dbf);
	}
	if (!table_only && (new_shp.CmpNoCase(cur_shp) != 0)
		&& wxFileExists(new_shp)) {
		overwrite_list.push_back(new_shp);
	}
	if (!table_only && (new_shx.CmpNoCase(cur_shx) != 0)
		&& wxFileExists(new_shx)) {
		overwrite_list.push_back(new_shx);
	}
	
	// Prompt for overwrite permission
	if (overwrite_list.size() > 0) {
		wxString msg((overwrite_list.size() > 1) ? "Files " : "File ");
		for (int i=0; i<overwrite_list.size(); i++) {
			msg << overwrite_list[i];
			if (i < overwrite_list.size()-1) msg << ", ";
		}
		msg << " already exist";
		msg << ((overwrite_list.size() > 1) ? "." : "s.");
		msg << " Ok to overwrite?";
		wxMessageDialog dlg (this, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}

	if (!project_p->IsShpFileNeedsFirstSave()) {
		// Copy shp and shx files as needed
		if (!wxFileExists(new_shp)) {
			if (!wxCopyFile(cur_shp, new_shp, true)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name << ".shp";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		if (!wxFileExists(new_shx)) {
			if (!wxCopyFile(cur_shx, new_shx, true)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name << ".shx";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
	} else {
		std::string err_msg;
		std::string shx_fname(new_shx.ToStdString());
		bool r = Shapefile::writePointIndexFile(shx_fname,
												project_p->index_data, err_msg);
		if (!r) {
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		std::string shp_fname(new_shp.ToStdString());
		r = Shapefile::writePointMainFile(shp_fname,
										  project_p->main_data, err_msg);
		if (!r) {
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		project_p->SetShapeFileNeedsFirstSave(false);
	}
	
	if (!table_only) project_p->shp_fname = wxFileName(new_shp);
	grid_base->dbf_file_name = wxFileName(new_sp_dbf);
	grid_base->dbf_file_name_no_ext = grid_base->dbf_file_name.GetName();
	if (space_time) {
		grid_base->dbf_tm_file_name = wxFileName(new_tm_dbf);
		grid_base->dbf_tm_file_name_no_ext =
			grid_base->dbf_tm_file_name.GetName();
	}
	
	OnSaveProject(event);
	
	// Must notify all frames that project name has changed so that they
	// can update titles and legend as needed.  At the moment, no
	// views contain the name of the Project, so no update needed.
}

void MyFrame::OnExportToCsvFile(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	ExportCsvDlg dlg(this, project_p);
	dlg.ShowModal();
}

bool MyFrame::SaveTableSpace()
{
	if (!project_p || !project_p->GetGridBase()) return false;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	DbfFileHeader backup_header = grid_base->orig_header;

	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxString curr_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	
	wxString err_msg;
	bool success = grid_base->WriteToDbf(curr_dbf, err_msg);
	if (!success) {
		grid_base->orig_header = backup_header;
		wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
		return false;
	}
	project->SetAllowEnableSave(true);
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
								   false);
	LOG_MSG("Table saved successfully");
	return true;
}

bool MyFrame::SaveTableSpaceTime()
{
	if (!project_p || !project_p->GetGridBase()) return false;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	DbfFileHeader backup_sp_header = grid_base->orig_header;
	DbfFileHeader backup_tm_header = grid_base->orig_header_tm;
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	grid_base->orig_header_tm.year = timeinfo->tm_year+1900;
	grid_base->orig_header_tm.month = timeinfo->tm_mon+1;
	grid_base->orig_header_tm.day = timeinfo->tm_mday;
	
	wxString curr_sp_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	wxString curr_tm_dbf = grid_base->GetTimeDbfFileName().GetFullPath();
	
	wxString err_msg;
	bool success = grid_base->WriteToSpaceTimeDbf(curr_sp_dbf,
												  curr_tm_dbf, err_msg);
	if (!success) {
		grid_base->orig_header = backup_sp_header;
		grid_base->orig_header_tm = backup_tm_header;
		wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
		return false;
	}
	project->SetAllowEnableSave(true);
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
					false);
	LOG_MSG("Space-Time tables saved successfully");
	return true;
}

void MyFrame::OnGeneratePointShpFile(wxCommandEvent& event)
{
	if (!GetProject() || !GetProject()->GetGridBase()) return;
	Project* p = GetProject();
	DbfGridTableBase* grid_base = p->GetGridBase();
	VariableSettingsDlg VS(GetProject(), VariableSettingsDlg::bivariate, false,
						   "New Map Coordinates");
	if (VS.ShowModal() != wxID_OK) return;
	
	std::vector<double> x;
	std::vector<double> y;
	grid_base->col_data[VS.col_ids[0]]->GetVec(x, VS.var_info[0].time);
	grid_base->col_data[VS.col_ids[1]]->GetVec(y, VS.var_info[1].time);
	ShapeUtils::populatePointShpFile(x, y, p->index_data, p->main_data);
	p->CreateShapefileFromPoints(x, y);
	LOG(p->main_data.records.size());
	UpdateToolbarAndMenus();
	MapNewFrame* nf = new MapNewFrame(this, p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
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
	RegressionTitleDlg dlg(this, regFileName);
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
	RegressionReportDlg *regReportDlg = new RegressionReportDlg(this, dump);
	regReportDlg->Show(true);
	regReportDlg->m_textbox->SetSelection(0, 0);
}

void MyFrame::OnShowCartogramMap(wxCommandEvent& WXUNUSED(event) )
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxString title = "Cartogram - " + VS.v1_name_with_time;
	CartogramFrame *subframe =
		new CartogramFrame(frame, VS.v1_single_time, project_p->GetNumRecords(),
						   VS.v1_name_with_time,
						   project_p, title, wxDefaultPosition,
						   GeoDaConst::map_default_size,
						   wxDEFAULT_FRAME_STYLE);
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
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxString title = "Map Movie - " + VS.v1_name_with_time;
	MapMovieFrame *subframe =
	new MapMovieFrame(frame,
					  VS.v1_single_time, project_p->GetNumRecords(),
					  VS.v1_name_with_time,
					  project_p, 1, title,
					  wxDefaultPosition,
					  wxSize(750, 450),
					  wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnMapMovieCumul(wxCommandEvent& WXUNUSED(event) )
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxString title = "Map Movie - " + VS.v1_name_with_time;
	MapMovieFrame *subframe =
		new MapMovieFrame(frame, VS.v1_single_time,
						  project_p->GetNumRecords(),
						  VS.v1_name_with_time,
						  project_p, 2, title,
						  wxDefaultPosition,
						  wxSize(750, 450), wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnExploreHist(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreHist");
	
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	
	HistogramFrame* f = new HistogramFrame(frame, project_p, VS.var_info,
										   VS.col_ids, "Histogram",
										   wxDefaultPosition,
										   GeoDaConst::hist_default_size);
	
	/*
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true);
	if (VS.ShowModal() != wxID_OK) return;
	
	HistFrame *subframe = new HistFrame(frame, VS.v1_single_time,
										project_p->GetNumRecords(),
										VS.v1_name_with_time, project_p,
										"Histogram - " + VS.v1_name_with_time,
										wxDefaultPosition,
										GeoDaConst::hist_default_size,
										wxDEFAULT_FRAME_STYLE);
	 */
	LOG_MSG("Exiting MyFrame::OnExploreHist");
}

void MyFrame::OnExploreScatterplot(wxCommandEvent& event)
{
	OnExploreScatterNewPlot(event);
}

void MyFrame::OnExploreScatterNewPlot(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
							"Scatter Plot Variables",
							"Independent Var X", "Dependent Var Y");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Scatter Plot");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(frame, project_p, dlg.var_info, dlg.col_ids,
							false, title, wxDefaultPosition,
							GeoDaConst::scatterplot_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnExploreBubbleChart(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate, false,
							"Bubble Chart Variables",
							"X-Axis", "Y-Axis",
							"Bubble Size", "Standard Deviation Color");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Bubble Chart");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(frame, project_p, dlg.var_info, dlg.col_ids,
							true, title, wxDefaultPosition,
							GeoDaConst::bubble_chart_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnExploreTestMap(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreTestMap");
	
	//MapNewFrame* subframe = new MapNewFrame(frame, project_p,
	//										MapNewCanvas::no_theme,
	//										MapNewCanvas::no_smoothing,
	//										wxDefaultPosition,
	//										GeoDaConst::map_default_size);
	//subframe->UpdateTitle();
	
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

void MyFrame::OnExploreNewBox(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxSize size = GeoDaConst::boxplot_default_size;
	int w = size.GetWidth();
	if (VS.var_info[0].is_time_variant) size.SetWidth((w*3)/2);
	else size.SetWidth(w/2);
	BoxNewPlotFrame *sf = new BoxNewPlotFrame(frame, GetProject(),
											  VS.var_info, VS.col_ids,
											  "Box Plot", wxDefaultPosition,
											  size);
}

void MyFrame::OnExplorePCP(wxCommandEvent& WXUNUSED(event))
{
	PCPDlg dlg(project_p->GetGridBase(),this);
	if (dlg.ShowModal() != wxID_OK) return;
	PCPNewFrame* s = new PCPNewFrame(this, GetProject(), dlg.var_info,
									 dlg.col_ids);
}

void MyFrame::OnExplore3DP(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
							"3D Scatter Plot Variables", "X", "Y", "Z");
	if (dlg.ShowModal() != wxID_OK) return;
	
	C3DPlotFrame *subframe =
		new C3DPlotFrame(frame, project_p, dlg.var_info, dlg.col_ids,
						 "3D Plot", wxDefaultPosition,
						 GeoDaConst::three_d_default_size,
						 wxDEFAULT_FRAME_STYLE,
						 dlg.v1_single_time, dlg.v1_name_with_time,
						 dlg.v2_single_time, dlg.v2_name_with_time,
						 dlg.v3_single_time, dlg.v3_name_with_time);
	
}

void MyFrame::OnExploreCC(wxCommandEvent& WXUNUSED(event))
{
	ConditionViewDlg dlg(this);
	if (dlg.ShowModal() != wxID_OK) return;
	
	VariableSettingsDlg::VarType v_type = VariableSettingsDlg::trivariate;
	wxString t;
	wxString v3_title("Variable");
	wxString v4_title;
	if (Conditionable::cViewType == 1) {
		t << "Conditional Map Variables";
	} else if (Conditionable::cViewType == 2) {
		t << "Conditional Boxplot Variables";
	} else if (Conditionable::cViewType == 3) {
		t << "Conditional Histogram Variables";
	} else if (Conditionable::cViewType == 4) {
		t << "Conditional Scatter Plot Variables";
		v_type = VariableSettingsDlg::quadvariate;
		v3_title = "Independent Var (x-axis)";
		v4_title = "Dependent Var (y-axis)";
	} else {
		return;
	}

	VariableSettingsDlg dlg2(project_p, v_type, true,
							 t, "X", "Y", v3_title, v4_title);	
	if (dlg2.ShowModal() != wxID_OK) return;
	
	wxString title;
	title << "X:" << dlg2.v1_name_with_time << ", Y:";
	title << dlg2.v2_name_with_time << ", ";
	if (Conditionable::cViewType != 4) {
		title << "Var:" << dlg2.v3_name_with_time;
	} else {
		title << "Independent:" << dlg2.v3_name_with_time;
		title <<", Dependent:" << dlg2.v4_name_with_time;
	}
	
	ConditionalViewFrame *subframe =
		new ConditionalViewFrame(frame, project_p,
								 dlg2.v1_single_time, dlg2.v1_name_with_time,
								 dlg2.v2_single_time, dlg2.v2_name_with_time,
								 dlg2.v3_single_time, dlg2.v3_name_with_time,
								 dlg2.v4_single_time, dlg2.v4_name_with_time,
								 title, wxDefaultPosition,
								 GeoDaConst::cond_view_default_size,
								 wxDEFAULT_FRAME_STYLE);
	subframe->SetSize(wxDefaultCoord, wxDefaultCoord, 800, 600);
}

void MyFrame::OnToolOpenNewTable(wxCommandEvent& WXUNUSED(event))
{
	OnOpenNewTable();
}

void MyFrame::OnOpenMSPL(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::univariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(frame, project_p, lc);
}

void MyFrame::OnOpenGMoran(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::bivariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(frame, project_p, lc);
}

void MyFrame::OnOpenMoranEB(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false,
						   "Empirical Bayes Rate Standardization Variables",
						   "Event Variable", "Base Variable");
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
										VS.var_info, VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
										false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(frame, project_p, lc);
}

void MyFrame::OnOpenUniLisa(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;

	GalWeight* gal = GetGal();
	if (!gal) return;
		
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap && !LWO.m_Moran) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::univariate,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  true, false, false);
	}
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  false, false, false,
												  wxDefaultPosition);
	}
}

void MyFrame::OnOpenMultiLisa(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false);
	if (VS.ShowModal() != wxID_OK) return;

	GalWeight* gal = GetGal();
	if (!gal) return;

	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap &&!LWO.m_Moran) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::bivariate,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  true, true, false);
	}
	
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  false, true, false);
	}
}

void MyFrame::OnOpenLisaEB(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, 9, 0);
	if (VS.ShowModal() != wxID_OK) return;
	
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  true, false, true);
	}
	
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(frame, project_p, lc,
												  false, false, true);
	}	
}

void MyFrame::OnOpenGetisOrd(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	if (!GetGal()) return;
		
	GetisOrdChoiceDlg dlg(this);
		
	if (dlg.ShowModal() != wxID_OK) return;
		
	if (!dlg.Gi_ClustMap_norm && !dlg.Gi_SigMap_norm &&
		!dlg.GiStar_ClustMap_norm && !dlg.GiStar_SigMap_norm &&
		!dlg.Gi_ClustMap_perm && !dlg.Gi_SigMap_perm &&
		!dlg.GiStar_ClustMap_perm && !dlg.GiStar_SigMap_perm) return;

	GStatCoordinator* gc = new GStatCoordinator(GetGal(),
												project_p->GetGridBase(),
												VS.var_info, VS.col_ids,
												dlg.row_standardize_weights);
	if (!gc || !gc->IsOk()) {
		// print error message
		delete gc;
		return;
	}
	
	if (dlg.Gi_ClustMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_clus_norm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_SigMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_sig_norm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_ClustMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_clus_norm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_SigMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_sig_norm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_ClustMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_clus_perm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_SigMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_sig_perm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_ClustMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_clus_perm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_SigMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_sig_perm, dlg.row_standardize_weights);
	}	
}

void MyFrame::OnOpenThemelessMap(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnThemelessMap(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnThemeless(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnThemeless(event);
	}
}

void MyFrame::OnOpenQuantile(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::quantile,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnQuantile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnQuantile(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnQuantile(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnQuantile(event);
	}
}

void MyFrame::OnOpenPercentile(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::percentile,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnPercentile(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnPercentile(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnPercentile(event);
	}
}

void MyFrame::OnOpenHinge15(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::hinge_15,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnHinge15(event);
	} else if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnHinge15(event);
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnHinge15(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge15(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnHinge15(event);
	}
}

void MyFrame::OnOpenHinge30(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::hinge_30,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (CartogramFrame* f = dynamic_cast<CartogramFrame*>(t)) {
		f->OnHinge30(event);
	} else if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnHinge30(event);
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnHinge30(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge30(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnHinge30(event);
	}
}

void MyFrame::OnOpenStddev(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::stddev,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnStdDevMap(event);
	}
}

void MyFrame::OnOpenNaturalBreaks(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::natural_breaks,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnNaturalBreaks(event);
	}
}

void MyFrame::OnOpenEqualIntervals(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::equal_intervals,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();	
}

void MyFrame::OnEqualIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnEqualIntervals(event);
	}
}

void MyFrame::OnOpenUniqueValues(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::unique_values,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnUniqueValues(event);
	}
}

void MyFrame::OnSaveCategories(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnSaveCategories(event);
	}
}

void MyFrame::OnOpenRawrate(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::raw_rate,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnRawrate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnRawrate(event);
	}
}

void MyFrame::OnOpenExcessrisk(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::excess_risk_theme,
									  MapNewCanvas::excess_risk,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnExcessrisk(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnExcessRisk(event);
	}
}

void MyFrame::OnOpenEmpiricalBayes(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::empirical_bayes,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnEmpiricalBayes(event);
	}
}

void MyFrame::OnOpenSpatialRate(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::spatial_rate,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnSpatialRate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSpatialRate(event);
	}
}

void MyFrame::OnOpenSpatialEmpiricalBayes(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(frame, project_p,
									  ThemeUtilities::no_theme,
									  MapNewCanvas::spatial_empirical_bayes,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}


void MyFrame::OnSpatialEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSpatialEmpiricalBayes(event);
	}
}

void MyFrame::OnSaveResults(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveRates(event);
	}
}

void MyFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (HistFrame* f = dynamic_cast<HistFrame*>(t)) {
		f->OnHistogramIntervals(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnHistogramIntervals(event);
	}
}

void MyFrame::OnRan99Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan99Per(event);
	}
}

void MyFrame::OnRan199Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan199Per(event);
	}
}

void MyFrame::OnRan499Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (LisaScatterPlotFrame* f
			   = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan499Per(event);
	}
}

void MyFrame::OnRan999Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan999Per(event);
	}
}

void MyFrame::OnRanOtherPer(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRanOtherPer(event);
	}
}

void MyFrame::OnSaveMoranI(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSaveMoranI(event);
	}
}

void MyFrame::OnSigFilter05(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter05(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter05(event);
	}
}

void MyFrame::OnSigFilter01(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter01(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter01(event);
	}
}

void MyFrame::OnSigFilter001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter001(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter001(event);
	}
}

void MyFrame::OnSigFilter0001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter0001(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter0001(event);
	}
}

void MyFrame::OnAddMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->GetProject()->AddMeanCenters();
	}
}

void MyFrame::OnAddCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->GetProject()->AddCentroids();
	}
}

void MyFrame::OnDisplayMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnDisplayMeanCenters();
	}
}

void MyFrame::OnDisplayCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnDisplayCentroids();
	}
}

void MyFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSaveGetisOrd(event);
	}
}

void MyFrame::OnSaveLisa(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSaveLisa(event);
	}
}

void MyFrame::OnSelectCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectCores(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSelectCores(event);
	}
}

void MyFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	}
}

void MyFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
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
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewStandardizedData(event);
	}	
}

void MyFrame::OnViewOriginalData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotFrame* f = dynamic_cast<ScatterPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	}
}

void MyFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotFrame* f = dynamic_cast<ScatterPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	}
}

void MyFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelected(event);
	}
}

void MyFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnDisplayStatistics(event);
	}
}

void MyFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnShowAxesThroughOrigin(event);
	}
}

void MyFrame::OnShowAxes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnShowAxes(event);
	}
}

void MyFrame::OnTimeSyncVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnTimeSyncVariable(var_index);
}

void MyFrame::OnTimeSyncVariable1(wxCommandEvent& event)
{
	OnTimeSyncVariable(0);
}

void MyFrame::OnTimeSyncVariable2(wxCommandEvent& event)
{
	OnTimeSyncVariable(1);
}

void MyFrame::OnTimeSyncVariable3(wxCommandEvent& event)
{
	OnTimeSyncVariable(2);
}

void MyFrame::OnTimeSyncVariable4(wxCommandEvent& event)
{
	OnTimeSyncVariable(3);
}

void MyFrame::OnFixedScaleVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFixedScaleVariable(var_index);
}

void MyFrame::OnFixedScaleVariable1(wxCommandEvent& event)
{
	OnFixedScaleVariable(0);
}

void MyFrame::OnFixedScaleVariable2(wxCommandEvent& event)
{
	OnFixedScaleVariable(1);
}

void MyFrame::OnFixedScaleVariable3(wxCommandEvent& event)
{
	OnFixedScaleVariable(2);
}

void MyFrame::OnFixedScaleVariable4(wxCommandEvent& event)
{
	OnFixedScaleVariable(3);
}

void MyFrame::OnPlotsPerView(int plots_per_view)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerView(plots_per_view);
}

void MyFrame::OnPlotsPerView1(wxCommandEvent& event)
{
	OnPlotsPerView(1);
}

void MyFrame::OnPlotsPerView2(wxCommandEvent& event)
{
	OnPlotsPerView(2);
}

void MyFrame::OnPlotsPerView3(wxCommandEvent& event)
{
	OnPlotsPerView(3);
}

void MyFrame::OnPlotsPerView4(wxCommandEvent& event)
{
	OnPlotsPerView(4);
}

void MyFrame::OnPlotsPerView5(wxCommandEvent& event)
{
	OnPlotsPerView(5);
}

void MyFrame::OnPlotsPerView6(wxCommandEvent& event)
{
	OnPlotsPerView(6);
}

void MyFrame::OnPlotsPerView7(wxCommandEvent& event)
{
	OnPlotsPerView(7);
}

void MyFrame::OnPlotsPerView8(wxCommandEvent& event)
{
	OnPlotsPerView(8);
}

void MyFrame::OnPlotsPerView9(wxCommandEvent& event)
{
	OnPlotsPerView(9);
}

void MyFrame::OnPlotsPerView10(wxCommandEvent& event)
{
	OnPlotsPerView(10);
}

void MyFrame::OnPlotsPerViewOther(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewOther();
}

void MyFrame::OnPlotsPerViewAll(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewAll();
}

void MyFrame::OnDisplayStatusBar(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnDisplayStatusBar(event);
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

Conditionable::Conditionable(bool conditional_view, int num_obs_s)
: isConditional(conditional_view), conditionFlag(0), num_obs(num_obs_s)
{
	//if (!isConditional) return;
	cWhere = cLocator;
	conditionFlag = new bool[num_obs];
	for (int i=0; i<num_obs; i++) conditionFlag[i] = true;
}

Conditionable::~Conditionable()
{
	if (conditionFlag) delete [] conditionFlag; conditionFlag = 0;
}

void Conditionable::UpdateCondition(int *flags)
{
	if(!isConditional) return;
	numCondObs = 0;
	for (int i=0; i<num_obs; i++) {
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
	my_children.Append(this);
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
	my_children.DeleteObject(this);
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
	if (nh_cnt == 0 && nuh_cnt == highlight_state->GetTotalHighlighted()) {
		highlight_state->SetEventType(HighlightState::unhighlight_all);
	} else {
		highlight_state->SetEventType(HighlightState::delta);
	}
	// Send update message to HighlightState class Observers, but specify
	// not to receive update message ourself.
	if (nh_cnt > 0 || nuh_cnt > 0) {
		MyFrame::GetProject()->highlight_state->notifyObservers(canvas);
	} else {
		LOG_MSG("No select/highlight changes");
	}
		
	LOG_MSG("Exiting HiddenFrame::Update");
}

