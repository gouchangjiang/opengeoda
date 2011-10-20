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

#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/clipbrd.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/msgdlg.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>

#include "ShapeOperations/shp.h"
#include "ShapeOperations/shp2gwt.h"
#include "ShapeOperations/shp2cnt.h"
#include "ShapeOperations/ShapeFileHdr.h"
#include "ShapeOperations/ShapeFileTypes.h"

#include "OpenGeoDa.h"
#include "DataViewer/DbfGridTableBase.h"
#include "TemplateCanvas.h"

#include "Thiessen/VorDataType.h"
#include "DialogTools/Statistics.h"
#include "DialogTools/MapQuantileDlg.h"
#include "DialogTools/ProgressDlg.h"
#include "DialogTools/RateSmootherDlg.h"
#include "DialogTools/SaveToTableDlg.h"
#include "DialogTools/VariableSettingsDlg.h"
#include "Explore/ConditionalView.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include "GeoDaConst.h"
#include "logger.h"
#include "mapview.h"

extern GeoDaEventType gEvent;
extern int gObservation;
extern inline void convert_rgb(float x, float y, float z, float* result);
extern inline void convert_hsv(float r, float g, float b, float* result);

const int ID_MAP_RESET = 0;
const int ID_MAPANALYSIS_CHOROPLETH_QUANTILE = 1;
const int ID_MAPANALYSIS_CHOROPLETH_PERCENTILE = 2;
const int ID_MAP_HINGE_15 = 3;
const int ID_MAP_HINGE_30 = 4;
const int ID_MAPANALYSIS_CHOROPLETH_STDDEV = 5;
const int ID_MAPANALYSIS_UNIQUE_VALUE = 6;
const int ID_MAPANALYSIS_EQUALINTERVAL = 7;
const int ID_MAPANALYSIS_NATURAL_BREAK = 8;

const int IDM_SMOOTH_RAWRATE = 9;
const int IDM_SMOOTH_EXCESSRISK = 10;
const int IDM_EMPERICAL_BAYES_SMOOTHER = 11;
const int IDM_SPATIAL_RATE_SMOOTHER = 12;
const int IDM_SPATIAL_EMPIRICAL_BAYES = 13;

/* This is a test */
MapSortElement::MapSortElement(double v, int i) {
    value = v;
    recordIndex = i;
}

bool MapSortElement::operator>(MapSortElement& a) {
    return (value > a.value);
}

bool MapSortElement::operator<(MapSortElement& a) {
    return (value < a.value);
}

MapSortElement& MapSortElement::operator=(const MapSortElement& a) {
    value = a.value;
    recordIndex = a.recordIndex;
    return *this;
}

BEGIN_EVENT_TABLE(MapFrame, wxFrame)
	EVT_SIZE(MapFrame::OnSize)
	EVT_MOVE(MapFrame::OnMove)
	EVT_CLOSE(MapFrame::OnClose)
	EVT_ACTIVATE(MapFrame::OnActivate)
	EVT_MENU(XRCID("wxID_CLOSE"), MapFrame::OnMenuClose) 
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MapCanvas, wxScrolledWindow)
	EVT_SIZE(MapCanvas::OnSize)
	EVT_PAINT(MapCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(MapCanvas::OnEvent)
	EVT_MOUSE_CAPTURE_LOST(MapCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()
// ---------------------------------------------------------------------------
// MapCanvas
// ---------------------------------------------------------------------------

// Define a constructor for my canvas
MapCanvas::MapCanvas(DbfGridTableBase* grid_base_s,
					 const wxString& fullPathShapefileName_s,
					 wxWindow *parent,
					 const wxPoint& pos, const wxSize& size,
					 ProgressDlg* p_dlg)
: TemplateCanvas(parent, pos, size),
	fullPathShapefileName(fullPathShapefileName_s), grid_base(grid_base_s)
{
    LOG_MSG("Entering MapCanvas::MapCanvas");
    LOG_MSG("In MapCanvas::MapCanvas");
    myP = (MapFrame*) parent;

	SetBackgroundColour(*wxWHITE);  // default color
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	selectable_fill_color = GeoDaConst::map_default_fill_colour;
	highlight_color = GeoDaConst::highlight_color;
	
    //MMM: For some reason, the following call to SetScrollRate casued a
    //     ScrollEvent to be processed which in turn caused MapCanvas::OnSize
    //     to be called and MapCanvas::CheckSize.  But, CheckSize assumes
    //     MapCanvas is fully initialized, which is not the case at this point!
    //     Try commenting out for now.
    //SetScrollRate( 10, 10 );
    work_mode = MODE_SELECT;
    gObservation = 0;
    wxString ishp = wxEmptyString;
    isPanning = false;
    IsObjSelected = true;

    iShapeFile shx(fullPathShapefileName, "shx");
    char hsx[ 2 * GeoDaConst::ShpHeaderSize ];
    shx.read((char *) & hsx[0], 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hdx(hsx);
    long offset, contents;
    //MMM: Must follow exactly how n is calculated and what it's used for.
    gObservation = (hdx.Length() - GeoDaConst::ShpHeaderSize) / 4;
    std::vector<long> OffsetIx(gObservation);
    if (gObservation < 1) {
        gObservation = 0;
        return;
    }
    for (long rec = 0; rec < gObservation; ++rec) {
        offset = ReadBig(shx);
        contents = ReadBig(shx);
        offset *= 2;
        OffsetIx[rec] = offset;
    }
    shx.close();
    iShapeFile shp1(fullPathShapefileName, "shp");
    char hs1[ 2 * GeoDaConst::ShpHeaderSize ];
    shp1.read(hs1, 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hd1(hs1);
    idx.resize(gObservation);
    n_total = 0;
    n_parts_per_cell.resize(gObservation);
    ppc = new pInt[gObservation];
    colors.resize(MAX_CATEGORY, *wxBLACK);
    c_id.resize(gObservation);

    // At this point, the following arrays all have size gObservation
    //   idx, n_parts_per_cell, ppc, c_id, OffsetIx

    colors.at(0) = selectable_fill_color;
    for (int i = 0; i < gObservation; i++) {
        c_id.at(i) = 0;
    }

    if (ShapeFileTypes::ShapeType(hd1.FileShape()) == ShapeFileTypes::POLYGON) {
        for (long rec = 0; rec < gObservation; rec++) {
            shp1.seekg(OffsetIx[rec] + 12, ios::beg);
            shp1.seekg(OffsetIx[rec] + 12, ios::beg);
            BoundaryShape t;
            t.ReadShape(shp1);
            n_parts_per_cell.at(rec) = t.GetNumParts();
            ppc[rec] = new int[n_parts_per_cell.at(rec) + 1];
            long* part = t.GetParts();
            long k;
            for (k = 0; k < n_parts_per_cell.at(rec); k++) {
                ppc[rec][k] = part[k]; // the index of starting point
            }
            int n_po = t.GetNumPoints(); // -1;
            ppc[rec][k] = n_po;
            n_total += n_po;
        }
    } else if (ShapeFileTypes::ShapeType(hd1.FileShape()) ==
			   ShapeFileTypes::SPOINT) {
        n_total = gObservation;
    }

    shp1.close();
    xs.resize(n_total);
    ys.resize(n_total);
    location.resize(n_total);
    center_location.resize(gObservation);
    Wlocation.resize(n_total);
    Wcenter_location.resize(gObservation);
    cloc_x.resize(gObservation);
    cloc_y.resize(gObservation);
    iShapeFile shp(fullPathShapefileName, "shp");
    char hs[ 2 * GeoDaConst::ShpHeaderSize ];
    shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hd(hs);
    Box pBox;
    xMin = 1e50;
    yMin = 1e50;
    xMax = -1e50;
    yMax = -1e50;

	if (p_dlg) p_dlg->StatusUpdate(0, "Loading Map...");
	int tenth = GenUtils::max(1, gObservation/10);
    long counter = 0;
    if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::POLYGON) {
        m_type = ShapeFileTypes::POLYGON;
        for (long rec = 0; rec < gObservation; ++rec) {
			if (p_dlg && (rec % tenth == 0)) {
				p_dlg->ValueUpdate(rec/ (double) gObservation);
			}
            shp.seekg(OffsetIx[rec] + 12, ios::beg);
#ifdef WORDS_BIGENDIAN
            char r[32], p;
            double m1, m2, n1, n2;
            shp.read((char *) r, sizeof (double) * 4);
            SWAP(r[0], r[7], p);
            SWAP(r[1], r[6], p);
            SWAP(r[2], r[5], p);
            SWAP(r[3], r[4], p);
            memcpy(&m1, &r[0], sizeof (double));
            SWAP(r[8], r[15], p);
            SWAP(r[9], r[14], p);
            SWAP(r[10], r[13], p);
            SWAP(r[11], r[12], p);
            memcpy(&m2, &r[8], sizeof (double));
            SWAP(r[16], r[23], p);
            SWAP(r[17], r[22], p);
            SWAP(r[18], r[21], p);
            SWAP(r[19], r[20], p);
            memcpy(&n1, &r[16], sizeof (double));
            SWAP(r[24], r[31], p);
            SWAP(r[25], r[30], p);
            SWAP(r[26], r[29], p);
            SWAP(r[27], r[28], p);
            memcpy(&n2, &r[24], sizeof (double));
            BasePoint p1 = BasePoint(m1, m2);
            BasePoint p2 = BasePoint(n1, n2);
            pBox = Box(p1, p2);
#else
            shp >> pBox;
#endif
            shp.seekg(OffsetIx[rec] + 12, ios::beg);
            BoundaryShape t;
            t.ReadShape(shp);

            int n_po = t.GetNumPoints(); // -1;

            if (xMin > pBox._min().x) xMin = pBox._min().x;
            if (yMin > pBox._min().y) yMin = pBox._min().y;
            if (xMax < pBox._max().x) xMax = pBox._max().x;
            if (yMax < pBox._max().y) yMax = pBox._max().y;
            BasePoint *Points;
            Points = t.GetPoints();
            idx.at(rec) = counter;
            for (int p = 0; p < n_po; p++) {
                // Copy the original (x,y) vertex data.
                xs.at(counter) = Points[p].x;
                ys.at(counter) = Points[p].y;
                counter++;
            }
        }
        shp.close();
        myBox bbox;

        // Compute the center point of each and every objects including
		// multi polygons
        ComputeXY(wxString::Format("%s", fullPathShapefileName.c_str()),
				  &n_total, cloc_x, cloc_y, &bbox, true);

        // The calculating the relative coordinates of the points from 
        // (xMin, yMin) (0,0) is (xMin, yMin)
        int cnt;
        for (cnt = 0; cnt < n_total; cnt++) {
            xs.at(cnt) = xs.at(cnt) - xMin;
            ys.at(cnt) = ys.at(cnt) - yMin;
        }

        for (cnt = 0; cnt < gObservation; cnt++) {
            cloc_x[cnt] = cloc_x[cnt] - xMin;
            cloc_y[cnt] = cloc_y[cnt] - yMin;
        }

    } else if (ShapeFileTypes::ShapeType(hd.FileShape()) == 
			   ShapeFileTypes::SPOINT) {
        shp.close();
        m_type = ShapeFileTypes::SPOINT;
        myBox bbox;
        ComputeXY(wxString::Format("%s", fullPathShapefileName.c_str()),
				  &n_total, xs, ys, &bbox, true);
        xMin = bbox.p1.x;
        yMin = bbox.p1.y;
        xMax = bbox.p2.x;
        yMax = bbox.p2.y;
        for (counter = 0; counter < gObservation; counter++) {
			if (p_dlg && (counter % tenth == 0)) {
				p_dlg->ValueUpdate(counter/ (double) gObservation);
			}
            cloc_x[counter] = xs.at(counter) - xMin;
            cloc_y[counter] = ys.at(counter) - yMin;
        }
    } else {
        shp.close();
    }
	if (p_dlg) p_dlg->ValueUpdate(100);

    m_nSaveRateValue = 0;

    ThemeID[0] = ID_MAPANALYSIS_CHOROPLETH_PERCENTILE;
    ThemeID[1] = ID_MAPANALYSIS_CHOROPLETH_QUANTILE;
    ThemeID[2] = ID_MAP_HINGE_15;
    ThemeID[3] = ID_MAP_HINGE_30;
    ThemeID[4] = ID_MAPANALYSIS_CHOROPLETH_STDDEV;
    ThemeID[5] = ID_MAPANALYSIS_UNIQUE_VALUE;
    ThemeID[6] = ID_MAPANALYSIS_EQUALINTERVAL;
    ThemeID[7] = ID_MAPANALYSIS_NATURAL_BREAK;

    fieldValue.resize(gObservation +1);

    Hinge = 0;
    key_flag = 0;

    //////////
    sel_mode = SELECTION_RECT;
    //////////

	selected.resize(gObservation); // MMM Selection bitvector
	for (int i=0; i<gObservation; i++) {
		selected[i] = gSelection.selected(i);
	}
	
    isLisaMap = false;
	isGetisOrdMap = false;
    CheckSize();

    LOG_MSG("Exiting MapCanvas::MapCanvas");
}

MapCanvas::~MapCanvas() {
    LOG_MSG("Entering MapCanvas::~MapCanvas");
    myP = 0; // my parent.  Should not need to set this to null
    if (ppc) delete [] ppc; ppc = 0;
    LOG_MSG("Exiting MapCanvas::~MapCanvas");
}

void MapCanvas::PickBrightSet(short ncolor, bool reversed) {

    Color.clear();
    Color.resize(ncolor, *wxBLACK);

    int i;
    float col[3];
    for (i = 0; i < ncolor; i++) {
        if (!reversed) {
            convert_rgb(0.0, 0.0, (float) 0.7 * (ncolor - i) / ncolor + 0.3, col);
            Color.at(i) = wxColour(255 * col[0], 255 * col[1], 255 * col[2]);
        } else {
            convert_rgb(0.0, 0.0, (float) 0.7 * (i + 1) / ncolor + 0.3, col);
            Color.at(i) = wxColour(255 * col[0], 255 * col[1], 255 * col[2]);
        }
    }
}

void MapCanvas::PickColorSet(short coltype, short ncolor, bool reversed)
{
    short colpos[11] = {0, 0, 0, 0, 3, 7, 12, 18, 25, 33, 42};

    wxColour Color1[53] = {//Sequential
        wxColour(255, 247, 188), wxColour(254, 196, 79), wxColour(217, 95, 14),

        wxColour(255, 255, 212), wxColour(254, 217, 142), wxColour(254, 153, 41),
        wxColour(204, 81, 2),

        wxColour(255, 255, 212), wxColour(254, 217, 142), wxColour(254, 153, 41),
        wxColour(217, 95, 14), wxColour(153, 52, 4),

        wxColour(255, 255, 212), wxColour(254, 227, 145), wxColour(254, 196, 79),
        wxColour(254, 153, 41), wxColour(217, 95, 14), wxColour(153, 52, 4),

        wxColour(255, 255, 212), wxColour(254, 227, 145), wxColour(254, 196, 79),
        wxColour(254, 153, 41), wxColour(236, 112, 20), wxColour(204, 81, 2),
        wxColour(140, 51, 5),

        wxColour(255, 255, 229), wxColour(255, 247, 188), wxColour(254, 227, 145),
        wxColour(254, 196, 79), wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 81, 2), wxColour(140, 51, 5),


        wxColour(255, 255, 229), wxColour(255, 247, 188), wxColour(254, 227, 145),
        wxColour(254, 196, 79), wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 81, 2), wxColour(153, 52, 4), wxColour(102, 46, 8),

        wxColour(255, 255, 229), wxColour(255, 247, 188), wxColour(254, 227, 145),
        wxColour(254, 196, 79), wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 81, 2), wxColour(153, 52, 4), wxColour(120, 46, 8),
        wxColour(102, 46, 8)
    };

    wxColour Color2[53] = {// Diverge
        wxColour(145, 191, 219), wxColour(255, 255, 191), wxColour(252, 141, 89),

        wxColour(44, 123, 182), wxColour(171, 217, 233), wxColour(253, 174, 97),
        wxColour(215, 25, 28),

        wxColour(44, 123, 182), wxColour(171, 217, 233), wxColour(255, 255, 191),
        wxColour(253, 174, 97), wxColour(215, 25, 28),

        wxColour(69, 117, 180), wxColour(145, 191, 219), wxColour(224, 243, 248),
        wxColour(254, 224, 144), wxColour(252, 141, 89), wxColour(215, 61, 41),

        wxColour(69, 117, 180), wxColour(145, 191, 219), wxColour(224, 243, 248),
        wxColour(255, 255, 191),
        wxColour(254, 224, 144), wxColour(252, 141, 89), wxColour(215, 61, 41),

        wxColour(69, 117, 180), wxColour(116, 173, 209), wxColour(171, 217, 233),
        wxColour(224, 243, 248), wxColour(254, 224, 144), wxColour(253, 174, 97),
        wxColour(244, 109, 67), wxColour(215, 61, 41),

        wxColour(69, 117, 180), wxColour(116, 173, 209), wxColour(171, 217, 233),
        wxColour(224, 243, 248), wxColour(255, 255, 191), wxColour(254, 224, 144),
        wxColour(253, 174, 97), wxColour(244, 109, 67), wxColour(215, 61, 41),

        wxColour(49, 54, 149), wxColour(69, 117, 180), wxColour(116, 173, 209),
        wxColour(171, 217, 233), wxColour(224, 243, 248), wxColour(254, 224, 144),
        wxColour(253, 174, 97), wxColour(244, 109, 67), wxColour(215, 61, 41),
        wxColour(173, 0, 49)
    };

    wxColour Color3[53] = {// Diverge
        wxColour(145, 191, 219), wxColour(255, 255, 191), wxColour(252, 141, 89),

        wxColour(44, 123, 182), wxColour(171, 217, 233), wxColour(253, 174, 97),
        wxColour(215, 25, 28),

        wxColour(44, 123, 182), wxColour(171, 217, 233), wxColour(255, 255, 191),
        wxColour(253, 174, 97), wxColour(215, 25, 28),

        wxColour(215, 61, 41), wxColour(252, 141, 89), wxColour(254, 224, 139),
        wxColour(230, 245, 152), wxColour(153, 213, 148), wxColour(43, 131, 186),

        wxColour(69, 117, 180), wxColour(145, 191, 219), wxColour(224, 243, 248),
        wxColour(255, 255, 191),
        wxColour(254, 224, 144), wxColour(252, 141, 89), wxColour(215, 61, 41),

        wxColour(69, 117, 180), wxColour(116, 173, 209), wxColour(171, 217, 233),
        wxColour(224, 243, 248), wxColour(254, 224, 144), wxColour(253, 174, 97),
        wxColour(244, 109, 67), wxColour(215, 61, 41),

        wxColour(69, 117, 180), wxColour(116, 173, 209), wxColour(171, 217, 233),
        wxColour(224, 243, 248), wxColour(255, 255, 191), wxColour(254, 224, 144),
        wxColour(253, 174, 97), wxColour(244, 109, 67), wxColour(215, 61, 41),

        wxColour(49, 54, 149), wxColour(69, 117, 180), wxColour(116, 173, 209),
        wxColour(171, 217, 233), wxColour(224, 243, 248), wxColour(254, 224, 144),
        wxColour(253, 174, 97), wxColour(244, 109, 67), wxColour(215, 61, 41),
        wxColour(173, 0, 49)
    };

    Color.clear();
    Color.resize(ncolor, *wxBLACK);

    int i;

    if (!reversed) {
        switch (coltype) {
            case 1:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color1[colpos[ncolor] + i];
                }
                break;
            case 2:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color2[colpos[ncolor] + i];
                }
                break;
            case 3:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color3[colpos[ncolor] + i];
                }
                break;
            default:
                break;
        }
    } else {
        switch (coltype) {
            case 1:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color1[colpos[ncolor] + ncolor - i - 1];
                }
                break;
            case 2:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color2[colpos[ncolor] + ncolor - i - 1];
                }
                break;
            case 3:
                for (i = 0; i < ncolor; i++) {
                    Color.at(i) = Color3[colpos[ncolor] + ncolor - i - 1];
                }
                break;
            default:
                break;
        }
    }
}

// Define the repainting behaviour

void MapCanvas::OnPaint(wxPaintEvent& event)
{
	//LOG_MSG("Entering MapCanvas::OnPaint");
	
	wxAutoBufferedPaintDC dc(this);
	//wxGCDC dc(p_dc);
	//wxGraphicsContext* gc = dc.GetGraphicsContext();
	//if (gc) gc->SetAntialiasMode(wxANTIALIAS_NONE);
	PrepareDC(dc);

	if (!isRedraw) Draw(&dc);
	//LOG_MSG("Exiting MapCanvas::OnPaint");
}

void MapCanvas::OnEvent(wxMouseEvent& event) {
    //LOG_MSG("Entering MapCanvas::OnEvent");
    if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
    if (event.LeftUp() && HasCapture()) ReleaseMouse();

    wxPoint pt(event.GetPosition());
	// check that gSelect1 is valid and update to current mouse
	// position if not valid
	if (gSelect1.x == -1) gSelect1 = pt;

    wxClientDC dc(this);
    PrepareDC(dc);
    wxPoint point(event.GetLogicalPosition(dc));

    if (event.RightUp()) {
        wxMenu* optMenu;
		if (isLisaMap) {
			optMenu = wxXmlResource::Get()->LoadMenu(
										"ID_LISAMAP_VIEW_MENU_CONTEXT");
		} else if (isGetisOrdMap) {
			optMenu = wxXmlResource::Get()->LoadMenu(
										"ID_GETIS_ORD_VIEW_MENU_CONTEXT");
		} else {
			optMenu = wxXmlResource::Get()->LoadMenu(
										"ID_MAP_VIEW_MENU_CONTEXT");
		}
		((MapFrame*) template_frame)->UpdateMenuCheckMarks(optMenu);
        PopupMenu(optMenu, event.GetPosition());
        return;
    }

    if (event.RightIsDown()) {
        // Right mouse drags are not supported, so just return
        // if this is true during the event.
        return;
    }

    /*
    bool debug_info = true;
    if (debug_info && event.LeftUp()) {
        LOG_MSG("MapCanvas state data:");
        for(int i=0; i<n_total; i++) {
            wxString msg;
            msg << "Wlocation[" << i << "] = (";
            msg << Wlocation[i].x << "," << Wlocation[i].y;
            msg << ")";
            LOG_MSG(msg);
        }
        for(int i=0; i<n_total; i++) {
            wxString msg;
            msg << "location[" << i << "] = (";
            msg << location[i].x << "," << location[i].y;
            msg << ")";
            LOG_MSG(msg);
        }
        for(int i=0; i<n_total; i++) {
            wxString msg;
            msg << "(xs[" << i << "],ys[" << i << "]) = (";
            msg << xs.at(i) << "," << ys.at(i);
            msg << ")";
            LOG_MSG(msg);
        }
    }
     */

    if (work_mode == MODE_SELECT) {
		//LOG_MSG("Entering MapCanvas::OnEvent, work_mode == MODE_SELECT");
        switch (sel_mode) {
            case SELECTION_RECT:
                TemplateCanvas::OnEvent(event);
                break;
            default: // CIRCLE and LINE
                if (event.LeftDClick()) {
                    OnLButtonDblClk(event, point);
                    return;
                }
                if (event.LeftDown()) {
                    OnMapLButtonDown(event, point);
                    return;
                }
                if (event.LeftUp()) {
                    OnMapLButtonUp(event, point);
                    return;
                }
                if (event.Leaving()) {
                    OnMouseLeaving(event, point);
                    return;
                }
                if (event.Dragging()) {
                    OnMapMouseMove(event, point);
					return;
				}
                if (event.Moving()) {
                    if (gRegime == BRUSH_SELECT) {
                        OnMapMouseMove(event, point);
					}
					return;
                }
                break;
        }
		//LOG_MSG("Exiting MapCanvas::OnEvent, work_mode == MODE_SELECT");
    } else if (work_mode == MODE_PANNING) {
        if (event.LeftDown()) {
            isPanning = true;
            gSelect1 = pt;
        }
        if (event.LeftUp()) {
            isPanning = false;
        }
        if (event.Dragging()) {
            if (isPanning) {
                int delta_x = gSelect1.x - pt.x;
                int delta_y = gSelect1.y - pt.y;
                int cnt;
                if (abs(delta_x) > 0) {
                    for (cnt = 0; cnt < n_total; cnt++)
                        Wlocation.at(cnt).x = Wlocation.at(cnt).x - delta_x;
                    for (cnt = 0; cnt < gObservation; cnt++) {
                        Wcenter_location.at(cnt).x = 
							Wcenter_location.at(cnt).x - delta_x;
					}
                }
                if (abs(delta_y) > 0) {
                    for (cnt = 0; cnt < n_total; cnt++) {
                        Wlocation.at(cnt).y = Wlocation.at(cnt).y - delta_y;
					}
                    for (cnt = 0; cnt < gObservation; cnt++) {
                        Wcenter_location.at(cnt).y =
							Wcenter_location.at(cnt).y - delta_y;
					}
                }
				Convert_wxPoint2wxRealPoint();
                Refresh(true);
                gSelect1 = pt;
            }
        }
    } else { // work_mode == MODE_ZOOMIN || MODE_ZOOMOUT
        if (event.LeftUp()) {
            EraseSelectionOutline();
			LOG(work_mode);
            wxSize frame_size = GetSize();

            wxSize rec_size(abs(gSelect1.x - gSelect2.x),
                abs(gSelect1.y - gSelect2.y));
            if (rec_size.x < 5)
                return;
            if (rec_size.y < 5)
                return;

            wxRealPoint center((gSelect1.x + gSelect2.x) / 2.0,
                (gSelect1.y + gSelect2.y) / 2.0);
            wxRealPoint fcenter(frame_size.x / 2.0, frame_size.y / 2.0);

            double factor1, factor2, factor;
            if (work_mode == MODE_ZOOMIN) {
                factor1 = (double) frame_size.x / (double) rec_size.x;
                factor2 = (double) frame_size.y / (double) rec_size.y;
                factor = (factor1 > factor2) ? factor2 : factor1;
				LOG(factor1);
				LOG(factor2);
				LOG(factor);
            } else if (work_mode == MODE_ZOOMOUT) {
                factor1 = (double) rec_size.x / (double) frame_size.x;
                factor2 = (double) rec_size.y / (double) frame_size.y;
                factor = (factor1 > factor2) ? factor2 : factor1;
            }

            center.x = center.x * factor;
            center.y = center.y * factor;

            for (int cnt = 0; cnt < n_total; cnt++) {
                Wlocation.at(cnt).x = Wlocation.at(cnt).x * factor;
                Wlocation.at(cnt).y = Wlocation.at(cnt).y * factor;
            }
            for (int cnt = 0; cnt < gObservation; cnt++) {
                Wcenter_location.at(cnt).x = Wcenter_location.at(cnt).x*factor;
                Wcenter_location.at(cnt).y = Wcenter_location.at(cnt).y*factor;
            }

            wxRealPoint po = (fcenter - center);

            for (int cnt = 0; cnt < n_total; cnt++) {
                Wlocation.at(cnt).x = Wlocation.at(cnt).x + po.x;
                Wlocation.at(cnt).y = Wlocation.at(cnt).y + po.y;
            }

            for (int cnt = 0; cnt < gObservation; cnt++) {
                Wcenter_location.at(cnt).x = Wcenter_location.at(cnt).x + po.x;
                Wcenter_location.at(cnt).y = Wcenter_location.at(cnt).y + po.y;
            }
			Convert_wxPoint2wxRealPoint();
            Refresh(true);

        } else if (event.Dragging()) {
			LOG_MSG("In MapCanvas::OnEvent, event.Dragging() == true");
            if (gRegime == BRUSH_SELECT) EraseSelectionOutline();

            wxClientDC dc(this);
            PrepareDC(dc);
            wxPoint point(event.GetLogicalPosition(dc));
            wxPoint m_point;

            wxSize frame_size = GetSize();

            wxSize rec_size(abs(gSelect1.x-point.x), abs(gSelect1.y - point.y));

            float factor_x, factor_y;
            factor_x = (float) rec_size.x / (float) frame_size.x;
            factor_y = (float) rec_size.y / (float) frame_size.y;

            if (factor_x > factor_y) {
                float target_size = factor_y * frame_size.x;

                m_point.x = (gSelect1.x > point.x) ?
                    (int) (gSelect1.x - target_size) :
                    (int) (gSelect1.x + target_size);
                m_point.y = point.y;
            } else {
                float target_size = factor_x * frame_size.y;

                m_point.x = point.x;
                m_point.y = (gSelect1.y > point.y) ?
                    (int) (gSelect1.y - target_size) :
                    (int) (gSelect1.y + target_size);
            }

            gSelection.Reset(true);
            ReSizeSelection(event, m_point);
        } else {
            TemplateCanvas::OnEvent(event);
        }
    }
    //LOG_MSG("Exiting MapCanvas::OnEvent");
}

void MapCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event) {
    // We must release mouse capture when we receive this event
    if (HasCapture()) ReleaseMouse();
}

void MapCanvas::OnMapLButtonDown(wxMouseEvent& event, wxPoint& point) {
    LOG_MSG("Entering MapCanvas::OnMapLButtonDown");
    if (gRegime != BRUSH_SELECT) {
        gSelect1 = point;
        gSelect2 = point;
    }
    gRegime = RECT_SELECT; // start selection of an area.
    MapEraseSelectionOutline();
    LOG_MSG("Exiting MapCanvas::OnMapLButtonDown");
}

void MapCanvas::OnMapLButtonUp(wxMouseEvent& event, wxPoint& point)
{
    gSelect2 = point;
    // If the total movement of the mouse is less than three pixels
	// from the initial gSelect1 position recorded when the
	// left mouse button was first pressed, then we treat the
	// selection as a simple selection rather than a mouse drag.
    if (abs(gSelect1.x - gSelect2.x) + abs(gSelect1.y - gSelect2.y) <= 3) {
        MapEraseSelectionOutline();
        SelectByPoint(event);
        gSelection.Reset(true);
        return;
    }

    if (event.CmdDown()) gRegime = BRUSH_SELECT;

    if (gRegime != BRUSH_SELECT) {
        MapEraseSelectionOutline();
    }
    SelectByType(event);
    gSelection.Reset( true );
 
    canvas_overlay.Reset(); // OVERLAY CODE
}

void MapCanvas::OnMapMouseMove(wxMouseEvent& event, wxPoint& point)
{
    LOG_MSG("Entering MapCanvas::OnMapMouseMove");
    gSelection.Reset(true);
    MapReSizeSelection(event, point);
	LOG_MSG("Exiting MapCanvas::OnMapMouseMove");
}

void MapCanvas::MapReSizeSelection(wxMouseEvent& event, wxPoint& point)
{
    LOG_MSG("Entering MapCanvas::MapReSizeSelection");

    if (gRegime == NO_SELECT) {
		LOG_MSG("MapReSizeSelection, gRegime == NO_SELECT, exiting...");
        return;
    }
 
    if (gRegime == RECT_SELECT) {			// resizing circle or line
        gSelect2 = point;					// update the corner
        MapDrawSelectionOutline();
    } else {								// gRegime == BRUSH_SELECT
        MapEraseSelectionOutline();
        gSelect1.x += (point.x - gSelect2.x);
        gSelect1.y += (point.y - gSelect2.y);
        gSelect2 = point;

        //gSelection.Update(); // This isn't in TemplateCanvas.  Needed?

        int mCnt = SelectByType(event);
        LOG(mCnt);

        if (gEvent == NO_EVENTS && !(event.ShiftDown())) {
			LOG_MSG("gEvent == NO_EVENTS && !(event.ShiftDown())");
            gEvent = NEW_SELECTION;
        }

		LOG_MSG("Calling MapDrawSelectionOutline end of MapReSizeSelection");
        MapDrawSelectionOutline();
    }
    LOG_MSG("Exiting MapCanvas::MapReSizeSelection");
}

void MapCanvas::ClearMapFrame() {
    LOG_MSG("Entering MapCanvas::ClearMapFrame");
	wxClientDC dc(this);
    //wxBufferedDC p_dc(&c_dc);
	//wxGCDC dc(p_dc);
	//wxGraphicsContext* gc = dc.GetGraphicsContext();
	//if (gc) gc->SetAntialiasMode(wxANTIALIAS_NONE);
	PrepareDC(dc);
	
    // OVERLAY CODE
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
    wxDCOverlay overlaydc(canvas_overlay, &dc);
    overlaydc.Clear();
    dc.SetPen(*wxBLUE_PEN);
#else
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(*wxBLACK_PEN);
#endif
    wxCoord radi;
    LOG(sel_mode);
    switch (sel_mode) {
        case SELECTION_LINE:
            dc.DrawLine(gSelect1, gSelect2);
            break;
        case SELECTION_CIRCLE:
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            radi = (int) GenUtils::distance(gSelect1, gSelect2);
            dc.DrawCircle(gSelect1, radi);
            break;
    }
    LOG_MSG("Exiting MapCanvas::ClearMapFrame");
}

void MapCanvas::MapDrawSelectionOutline()
{
	LOG_MSG("In MapCanvas::MapDrawSelectionOutline");
    // cur_sel_outline[0] will store previous value of gSelect1 and
    // cur_sel_outline[1] will store previous value of gSelect2 when
    // sel_mode == SELECTION_LINE || sel_mode == SELECTION_CIRCLE

	wxClientDC dc(this);
    //wxBufferedDC p_dc(&c_dc);
	//wxGCDC dc(p_dc);
	//wxGraphicsContext* gc = dc.GetGraphicsContext();
	//if (gc) gc->SetAntialiasMode(wxANTIALIAS_NONE);
	PrepareDC(dc);	
	
	PrepareDC(dc);
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
		// OVERLAY CODE
		wxDCOverlay overlaydc( canvas_overlay, &dc );
#endif

	if (selection_outline_visible) {
		LOG_MSG("   selection_outline_visible==true, first erasing selection \
				outline");
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
		// OVERLAY CODE
		LOG_MSG("Calling overlaydc.Clear()");
		overlaydc.Clear();
		LOG_MSG("Called overlaydc.Clear()");
#else
		if (!selection_outline_visible) return;
        dc.SetLogicalFunction(wxINVERT);
		dc.SetPen(*wxBLACK_PEN);
        if (sel_mode == SELECTION_LINE) {
            dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
        } else if (sel_mode == SELECTION_CIRCLE) {
            wxCoord radi = (wxCoord) GenUtils::distance(cur_sel_outline[0],
                cur_sel_outline[1]);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawCircle(cur_sel_outline[0], radi);
        }
#endif
		selection_outline_visible = false;
	}

    cur_sel_outline[0] = gSelect1;
    cur_sel_outline[1] = gSelect2;

#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	dc.SetPen(*wxBLUE_PEN);
#else
	dc.SetLogicalFunction(wxINVERT);
	dc.SetPen(*wxBLACK_PEN);
#endif
    if (sel_mode == SELECTION_LINE) {
        dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
    } else if (sel_mode == SELECTION_CIRCLE) {
        wxCoord radi = (wxCoord) GenUtils::distance(cur_sel_outline[0],
            cur_sel_outline[1]);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(cur_sel_outline[0], radi);
    }
	selection_outline_visible = true;
}

void MapCanvas::MapEraseSelectionOutline()
{
	LOG_MSG("In MapCanvas::MapEraseSelectionOutline");
#if defined(__WXMAC__) // || defined(__WXGTK__) || defined(__WXMSW__)
	// OVERLAY CODE
	wxClientDC dc(this);
	PrepareDC(dc);
    wxDCOverlay overlaydc( canvas_overlay, &dc );
    LOG_MSG("Calling overlaydc.Clear()");
    overlaydc.Clear();
    LOG_MSG("Called overlaydc.Clear()");
#else
	if (!selection_outline_visible) return;
	
	wxClientDC dc(this);
    //wxBufferedDC p_dc(&c_dc);
	//wxGCDC dc(p_dc);
	//wxGraphicsContext* gc = dc.GetGraphicsContext();
	//if (gc) gc->SetAntialiasMode(wxANTIALIAS_NONE);
	PrepareDC(dc);
	
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(*wxBLACK_PEN);

    // cur_sel_outline[0] has previous value of gSelect1 and
    // cur_sel_outline[1] has previous value of gSelect2 when
    // sel_mode == SELECTION_LINE || sel_mode == SELECTION_CIRCLE
    if (sel_mode == SELECTION_LINE) {
        dc.DrawLine(cur_sel_outline[0], cur_sel_outline[1]);
    } else if (sel_mode == SELECTION_CIRCLE) {
        wxCoord radi = (wxCoord) GenUtils::distance(cur_sel_outline[0],
            cur_sel_outline[1]);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(cur_sel_outline[0], radi);
    }
#endif
	selection_outline_visible = false;
}

void MapCanvas::Selection(wxDC* pDC)
{
    int cnt;
    switch (gEvent) {
        case NEW_SELECTION:
			//MMM Selection bitvector for speedup
            //Draw(pDC);
			// only redraw the polygons that have changed selection status
			for (int i=0; i<gObservation; i++) {
				if (gSelection.selected(i) && !selected[i] ) {
					selected[i] = true;
					DrawSinglePolygon(pDC, i, true);
				} else if (!gSelection.selected(i) && selected[i]) {
					selected[i] = false;
					DrawSinglePolygon(pDC, i, false);
				}
			}
            break;
        case ADD_SELECTION:
            cnt = gSelection.Pop();
            while (cnt != GeoDaConst::EMPTY) {
				selected[cnt] = true;
                DrawSinglePolygon(pDC, cnt, true);
                cnt = gSelection.Pop();
            }
            break;
        case DEL_SELECTION:
			cnt = gSelection.Pop();
            while (cnt != GeoDaConst::EMPTY) {
				selected[cnt] = true;
                DrawSinglePolygon(pDC, cnt, false);
				cnt = gSelection.Pop();
            }
            break;
        default:
            break;
    }
    gSelection.Reset();
}

bool MapCanvas::pointInPolygon(int polySides, wxRealPoint poly[],
    int xx, int yy)
{
    int i;
    int j = 0;
    bool oddNODES = false;
    float x = (float) xx;
    float y = (float) yy;
    for (i = 0; i < polySides; i++) {
        j++;
        if (j == polySides)
            j = 0;
        if (poly[i].y < y && poly[j].y >= y
            || poly[j].y < y && poly[i].y >= y) {
            if (poly[i].x +
                (y - poly[i].y) / (poly[j].y - poly[i].y)
					*(poly[j].x - poly[i].x) < x) {
                oddNODES = !oddNODES;
            }
        }
    }
    return oddNODES;
}

bool MapCanvas::LineToLine(wxRealPoint p1, wxRealPoint p2,
    float a, float b, float c, wxRealPoint pp3, wxRealPoint pp4)
{
    wxPoint p3, p4;
    if (pp3.x <= pp4.x) {
        p3.x = pp3.x;
        p4.x = pp4.x;
    } else {
        p4.x = pp3.x;
        p3.x = pp4.x;
    }
    if (pp3.y <= pp4.y) {
        p3.y = pp3.y;
        p4.y = pp4.y;
    } else {
        p4.y = pp3.y;
        p3.y = pp4.y;
    }

    if (p1.x > p4.x)
        return false;
    if (p1.y > p4.y)
        return false;
    if (p2.x < p3.x)
        return false;

    if (p2.y < p3.y)
        return false;
    if ((a * pp3.x + b * pp3.y + c) * (a * pp4.x + b * pp4.y + c) > 0.0)
        return false;

    return true;
}

int MapCanvas::SelectByType(wxMouseEvent& event)
{
    wxRealPoint p1, p2;
    if (gSelect1.x <= gSelect2.x) {
        p1.x = gSelect1.x;  p2.x = gSelect2.x;
    } else {
        p2.x = gSelect1.x;  p1.x = gSelect2.x;
    }
    if (gSelect1.y <= gSelect2.y) {
        p1.y = gSelect1.y;  p2.y = gSelect2.y;
    } else {
        p2.y = gSelect1.y;  p1.y = gSelect2.y;
    }

	gEvent = (event.ShiftDown()) ? ADD_SELECTION : NEW_SELECTION;

    int mCnt = 0;
    int radi = (int) GenUtils::distance(gSelect1, gSelect2);

    switch (sel_mode) {
        case SELECTION_LINE:
            {
            bool fb;
            float a, b, c;
            a = (float) (gSelect2.y - gSelect1.y);
            b = (float) (gSelect1.x - gSelect2.x);
            c = -a * ((float) gSelect1.x) - b * ((float) gSelect1.y);

            if (m_type == ShapeFileTypes::POLYGON) {
                for (int cnt = 0; cnt < gObservation; ++cnt) {
                    for (int k = 0; k < n_parts_per_cell.at(cnt); k++) {
                        fb = false;
                        for (int p = 0;
                             p < (ppc[cnt][k + 1] - ppc[cnt][k]);
                             p++)
                        {
                            if (LineToLine(p1, p2, a, b, c,
                                Wlocation.at(idx.at(cnt) + ppc[cnt][k] + p),
                                Wlocation.at(idx.at(cnt) + ppc[cnt][k]+(p + 1)
                                    % (ppc[cnt][k + 1] - ppc[cnt][k]))))
                            {
                                gSelection.Push(cnt);
                                ++mCnt;
                                fb = true;
                                break;
                            }
                        }
                        if (fb) break;
                    }
                }
            }
            }
            break;
        case SELECTION_CIRCLE:
            {
            for (int cnt = 0; cnt < gObservation; ++cnt) {
                int radi2 = (int) GenUtils::distance(gSelect1,
                    Wcenter_location.at(cnt));
                if ((radi2 <= radi) &&
                    (gEvent == NEW_SELECTION || !gSelection.selected(cnt)))
                {
                    gSelection.Push(cnt);
                    ++mCnt;
                }
            }
            }
            break;
        default:
            break;
    }

    if (mCnt == 0) {
        gEvent = NO_EVENTS;
    } else {
        gSelection.Update();
        MyFrame::theFrame->UpdateWholeView(NULL);
		gSelection.Reset(true);
    }

    return mCnt;
}

int MapCanvas::SelectByRect(wxMouseEvent& event) {
    wxRealPoint p1, p2;

    if (gSelect1.x <= gSelect2.x) {
        p1.x = gSelect1.x;  p2.x = gSelect2.x;
    } else {
        p2.x = gSelect1.x;  p1.x = gSelect2.x;
    }
    if (gSelect1.y <= gSelect2.y) {
        p1.y = gSelect1.y;  p2.y = gSelect2.y;
    } else {
        p2.y = gSelect1.y;  p1.y = gSelect2.y;
    }

    gEvent = (event.ShiftDown()) ? ADD_SELECTION : NEW_SELECTION;

    int mCnt = 0;
    for (int cnt = 0; cnt < gObservation; ++cnt) {
        if (Wcenter_location.at(cnt).x >= p1.x &&
			Wcenter_location.at(cnt).x <= p2.x &&
            Wcenter_location.at(cnt).y >= p1.y &&
			Wcenter_location.at(cnt).y <= p2.y &&
            (gEvent == NEW_SELECTION || !gSelection.selected(cnt))) {
			//(!gSelection.selected(cnt))) {
            gSelection.Push(cnt);
            ++mCnt;
        }
	}
    if (mCnt == 0) {
        gEvent = NO_EVENTS;
    } else {
        gSelection.Update();
        MyFrame::theFrame->UpdateWholeView(NULL);
		gSelection.Reset(true);
    }

    return mCnt;
}

void MapCanvas::SelectByPoint(wxMouseEvent& event) {
    int Id = GeoDaConst::EMPTY;
    if (m_type == ShapeFileTypes::POLYGON) {
        int cnt, k;
        for (cnt = 0; cnt < gObservation; ++cnt) {
            for (k = 0; k < n_parts_per_cell.at(cnt); k++) {
                if (pointInPolygon((ppc[cnt][k + 1] - ppc[cnt][k]),
                    &(Wlocation.at(idx.at(cnt) + ppc[cnt][k])),
                    gSelect1.x, gSelect1.y)) {
                    Id = cnt;
                    break;
                }
            }
            if (Id != GeoDaConst::EMPTY)
                break;
        }
    }
    if (Id == GeoDaConst::EMPTY) {
        EmptyClick();
		gSelection.Reset(true); // reset to empty
		gEvent = NEW_SELECTION;
        gSelection.Update();
        MyFrame::theFrame->UpdateWholeView(NULL);
		gSelection.Reset(true);
        return;
    };

    bool isSelected = gSelection.selected(Id);
    if (isSelected && event.ShiftDown())
        gEvent = DEL_SELECTION;
    else if (!isSelected && event.ShiftDown())
        gEvent = ADD_SELECTION;
    else
        gEvent = NEW_SELECTION;

    gSelection.Push(Id);
    gSelection.Update();
    MyFrame::theFrame->UpdateWholeView(NULL);
	gSelection.Reset(true);
}


void MapCanvas::DrawPolygons(wxDC* dc) {
    //LOG_MSG("Entering MapCanvas::DrawPolygons");

	bool hide_outlines = !IsSelectableOutlineVisible();
    int k = 0;
    for (int rec = 0; rec < gObservation; rec++) {
        if (m_type == ShapeFileTypes::POLYGON) {
            wxBrush brush;
			wxPen pen(*wxBLACK_PEN);
			if (hide_outlines) {
				pen.SetColour(colors[c_id[rec]]);
			}
			dc->SetPen(pen);
			if (gSelection.selected(rec)) {
				if (hide_outlines) {
					pen.SetColour(highlight_color);
					dc->SetPen(pen);
				}
                brush.SetColour(colors[c_id[rec]]);
                dc->SetBrush(brush);
                for (k = 0; k < n_parts_per_cell[rec]; k++) {
                    dc->DrawPolygon(ppc[rec][k + 1] - ppc[rec][k] - 1,
                        &(location[idx[rec] + ppc[rec][k]]));
                }
                //wxColour highColorAlpha(highlight_color.Red(),
				//                        highlight_color.Green(),
                //						  highlight_color.Blue(), 128);
                //brush.SetColour(highColorAlpha);
                brush.SetColour(highlight_color);
                brush.SetStyle(wxCROSSDIAG_HATCH);
                //wxPen pen = wxPen(highColorAlpha, 5);
                //dc->SetPen(pen);
                dc->SetBrush(brush);
                for (k = 0; k < n_parts_per_cell[rec]; k++) {
                    dc->DrawPolygon(ppc[rec][k + 1] - ppc[rec][k] - 1,
                        &(location[idx[rec] + ppc[rec][k]]));
                }
            } else {
                brush.SetColour(colors[c_id[rec]]);
                dc->SetBrush(brush);
                for (k = 0; k < n_parts_per_cell[rec]; k++) {
                    dc->DrawPolygon(ppc[rec][k + 1] - ppc[rec][k],
                        &(location[idx[rec] + ppc[rec][k]]));
                }
            }
        } else if (m_type == ShapeFileTypes::SPOINT) {
            if (gSelection.selected(rec))
                GenUtils::DrawSmallCirc(dc,
							 center_location.at(rec).x,
							 center_location.at(rec).y,
							 StarSize, highlight_color);
            else
                GenUtils::DrawSmallCirc(dc,
							 center_location.at(rec).x,
							 center_location.at(rec).y,
							 StarSize, colors.at(c_id.at(rec)));
        }
    }

    //LOG_MSG("Exiting MapCanvas::DrawPolygons");
}

void MapCanvas::DrawSinglePolygon(wxDC* dc, int rec, bool highlight) {	
    int k = 0;
    wxBrush brush;
	bool hide_outlines = !IsSelectableOutlineVisible();
    if (m_type == ShapeFileTypes::POLYGON) {
		wxPen pen(*wxBLACK_PEN);
		if (hide_outlines) {
			if (highlight) {
				pen.SetColour(highlight_color);
			} else {
				pen.SetColour(colors[c_id[rec]]);
			}
		}
		dc->SetPen(pen);
        brush.SetColour(colors.at(c_id[rec]));
        dc->SetBrush(brush);
        for (k = 0; k < n_parts_per_cell[rec]; k++)
            dc->DrawPolygon(ppc[rec][k + 1] - ppc[rec][k] - 1,
							&(location[idx[rec] + ppc[rec][k]]));

        if (highlight) {
            brush.SetColour(highlight_color);
            brush.SetStyle(wxCROSSDIAG_HATCH);
            dc->SetBrush(brush);

            for (k = 0; k < n_parts_per_cell[rec]; k++)
                dc->DrawPolygon(ppc[rec][k + 1] - ppc[rec][k] - 1,
								&(location[idx[rec] + ppc[rec][k]]));
        }
    } else {
        GenUtils::DrawSmallCirc(dc,
								center_location[rec].x,
								center_location[rec].y,
								StarSize,
								highlight ?
									highlight_color : colors[c_id[rec]]);
    }
}

void MapCanvas::Draw(wxDC* dc) {
	wxColour color = GetBackgroundColour();
	dc->SetPen(wxPen(color));
	dc->SetBrush(wxBrush(color));
	dc->DrawRectangle(wxPoint(0,0), dc->GetSize());
    DrawPolygons(dc);
}

void MapCanvas::Convert_wxPoint2wxRealPoint() {
    //LOG_MSG("In MapCanvas::Convert_wxPoint2wxRealPoint");
    int cnt;
    for (cnt = 0; cnt < n_total; cnt++) {
        location.at(cnt).x = (int) Wlocation.at(cnt).x;
        location.at(cnt).y = (int) Wlocation.at(cnt).y;
    }
    for (cnt = 0; cnt < gObservation; cnt++) {
        center_location.at(cnt).x = (int) Wcenter_location.at(cnt).x;
        center_location.at(cnt).y = (int) Wcenter_location.at(cnt).y;
    }
}

void MapCanvas::CheckSize() {
    //LOG_MSG("Entering MapCanvas::CheckSize");
    int Left, Right, Top, Bottom, Width, Height;
    wxSize size2 = GetClientSize();
	
    Left = 10;
    Right = 10;
    Top = 10;
    Bottom = 10;
    Width = 40;
    Height = 40;
	
    int w = size2.x;
    int h = size2.y;
	
    double xDensity = 0.0;
    double yDensity = 0.0;
    double dx = xMax - xMin;
    double dy = yMax - yMin;
	
    int res = w - Left - Right - Width;
    if (res < 0) res = 0;
    int rata = res / 16;
    Left += rata;
    Right += rata;
    Width = w - Left - Right;
	
    res = h - Top - Bottom - Height;
    if (res < 0) res = 0;
    rata = res / 16;
    Top += rata;
    Bottom += rata;
    Height = h - Top - Bottom;
	
	
    if (dx > 0.0)
        xDensity = Width / dx;
	
    if (dy > 0.0)
        yDensity = Height / dy;
	
    double aDensity = (xDensity > yDensity) ? yDensity : xDensity;
	
	
    int cnt;
	
    Left = (w - (aDensity * dx)) / 2;
    Top = (h - (aDensity * dy)) / 2;
    Height = h - 2 * Top;
	
    for (cnt = 0; cnt < n_total; cnt++) {
        // scale each point and move away from the border by an appropriate
        // amount.  Also, flip y-coordinate into screen coordinates.
        Wlocation.at(cnt).x = (double) ((xs.at(cnt) * aDensity) + Left);
        Wlocation.at(cnt).y = (double) (Height - (ys.at(cnt) * aDensity) + Top);
    }
	
    for (cnt = 0; cnt < gObservation; cnt++) {
        Wcenter_location.at(cnt).x = (double) (cloc_x[cnt] * aDensity) + Left;
        Wcenter_location.at(cnt).y = (double) (Height - cloc_y[cnt] * aDensity)
		+ Top;
    }
	
    //LOG(Left);
    //LOG(Right);
    //LOG(Top);
    //LOG(Bottom);
    //LOG(Height);
    //LOG(aDensity);
    //LOG(w);
    //LOG(h);
	
    StarSize = (int) (log10((double) Width + (double) Height)
					  - log10((double) gObservation)
					  + ((double) Width + (double) Height)
					  / 256);
    if (StarSize < 0)
        StarSize = 0;
    else if (StarSize > 4)
        StarSize = 4;
	
	Convert_wxPoint2wxRealPoint();
    //LOG_MSG("Exiting MapCanvas::CheckSize");
}

void MapCanvas::OnSize(wxSizeEvent& event)
{
	isRedraw = true;
	if (work_mode == MODE_SELECT) CheckSize();
	Refresh();
	isRedraw = false;
    event.Skip();
}

void MapCanvas::QuickSortFieldValT(std::vector<MapSortElement>& array,
    int low, int high) {
    int i, j;
    SortT(&array[low], &array[(low + high) / 2], &array[high]);
    if ((high - low) > 2) {
        SwapT(&array[low + 1], &array[(low + high) / 2]);
        i = low + 1;
        j = high;
        while (i < j) {
            i++;
            while (array[i] < array[low + 1])
                i++;
            j--;
            while (array[j] > array[low + 1])
                j--;
            SwapT(&array[i], &array[j]);
        }
        SwapT(&array[i], &array[j]);
        SwapT(&array[low + 1], &array[j]);
        QuickSortFieldValT(array, low, j - 1);
        QuickSortFieldValT(array, j + 1, high);

    }
}

void MapCanvas::SortT(MapSortElement *i, MapSortElement *j, MapSortElement *k) {
    if (*i > *j) {
        SwapT(i, j);
        if (*j > *k)
            SwapT(j, k);
        if (*i > *j)
            SwapT(i, j);
    } else {
        if (*j > *k)
            SwapT(j, k);
        if (*i>*j)
            SwapT(i, j);
    }

}

void MapCanvas::SwapT(MapSortElement *x, MapSortElement *y) {
    MapSortElement temp(0, 0);
    temp = *x;
    *x = *y;
    *y = temp;
}

void MapCanvas::ComputeFrequencyUnique(const std::vector<double>& b,
									   int nC, std::vector<long>& f) {
    f.resize(nC + 1);

    for (int i = 0; i < nC; i++) f[i] = 0;

    int k = 0;
    int i = 0;
    while (i < gObservation && k < nC) {
        while ((i < gObservation) && (fieldValue.at(i).value == b[k])) {
            f[k]++;
            i++;
        }
        k++;
    }

    f[nC] = gObservation - i;
}

void MapCanvas::ComputeFrequency(const std::vector<double>& b,
								 int nC, std::vector<long>& f)
{
    f.resize(nC + 1);
    int i = 0;
    for (i = 0; i < nC; i++) f[i] = 0;

    int k = 0;
    i = 0;
    while (i < gObservation && k < nC) {
        while ((i < gObservation) && (fieldValue.at(i).value < b[k])) {
            f[k]++;
            i++;
        }
        k++;
    }

    f[nC] = gObservation - i;
}

void MapCanvas::UserConfigUpdate() {
	// Note, only called for Equal Interval and Natural Break
    int numRecords = gObservation;

    std::vector<long> freq;
    ComputeFrequency(breakVal_reserve, numClasses, freq);

    int ii, k;
    for (ii = 0; ii < numClasses; ii++) {
        myP->m_str[ii] = wxEmptyString;
        myP->m_str[ii] << breakVal_reserve.at(ii - 1)
            << " ~ " << breakVal_reserve.at(ii) << " (" << freq[ii]
            << ")";
    }

    ii = 0;
    k = 0;
    while (ii < numRecords && k < numClasses) {
        while ((ii < numRecords) &&
            (fieldValue.at(ii).value < breakVal_reserve.at(k))) {
            c_id.at(fieldValue.at(ii).recordIndex) = k;
            ii++;
        }
        k++;
    }
}

void MapCanvas::ChoroplethMapUpdate(int select) {
    LOG_MSG("Entering MapCanvas::ChoroplethMapUpdate");
    DynStatistics DynS(m_gX, gObservation);
    long numRecords;

    std::vector<long> freq;

    if (IDChoropleth != ID_MAP_RESET) {
        if ((IDChoropleth == IDM_SPATIAL_RATE_SMOOTHER) ||
            (IDChoropleth == IDM_EMPERICAL_BAYES_SMOOTHER) ||
            (IDChoropleth == IDM_SPATIAL_EMPIRICAL_BAYES) ||
            (IDChoropleth == IDM_SMOOTH_RAWRATE)) {
            IDChoropleth = ThemeID[select];
        } else {
            if (IDChoropleth != IDM_SMOOTH_EXCESSRISK) {
                m_fieldName = m_gVar1;
            }
        }

        for (int i = 0; i < gObservation; i++) {
            fieldValue.at(i).value = m_gX[i];
            fieldValue.at(i).recordIndex = i;
        }

        DynS.ComputeStats();
        numRecords = gObservation;
        QuickSortFieldValT(fieldValue, 0, gObservation - 1);
    }

    switch (IDChoropleth) {
        case IDM_SMOOTH_EXCESSRISK:
        {
            wxString title = "Excess Risk Map: " + m_fieldName;
            myP->SetTitle(title);
            myP->SetTitle2(m_fieldName);
            myP->numBreaks = 6;

            PickColorSet(2, 6, false);

            wxString str[6];
            str[0] = "< 0.25";
            str[1] = " 0.25 - 0.50";
            str[2] = " 0.50 - 1.00";
            str[3] = " 1.00 - 2.00";
            str[4] = " 2.00 - 4.00";
            str[5] = "> 4.00";
            numClasses = 6;
            breakVal.clear();
            breakVal.resize(numClasses);
            breakVal.at(0) = 0.25;
            breakVal.at(1) = 0.5;
            breakVal.at(2) = 1.0;
            breakVal.at(3) = 2.0;
            breakVal.at(4) = 4.0;
            breakVal.at(5) = 9999;

            ComputeFrequency(breakVal, numClasses, freq);

            int index = 0;
            breakVal_reserve.clear();
            breakVal_reserve.resize(myP->numBreaks + 1);
            breakVal_reserve.at(index) = 0.125;
            for (index = 0; index < 5; index++) {
                breakVal_reserve.at(index + 1) = breakVal.at(index);
            }
            breakVal_reserve.at(6) = DynS.Mean() + 8.0;


            int k = 0, i = 0;
            for (i = 1; i < numClasses; i++) {
                if (breakVal.at(i - 1) == breakVal.at(i) ||
                    freq[i - 1] == 0) k = i;
            }
            for (i = 0; i < k; i++) Color.at(i) = Color.at(k);

            i = 0;
            k = 0;
            while (i < numRecords && k < numClasses) {
                while (fieldValue.at(i).value < breakVal.at(k)
					   && i < numRecords) {
                    c_id.at(fieldValue.at(i).recordIndex) = k;
                    i++;
                }
                k++;
            }

            for (i = 0; i < numClasses; i++) colors.at(i) = Color.at(i);

            for (i = 0; i < numClasses; i++) {
                myP->m_str[i] = wxEmptyString;
                myP->m_str[i] << str[i] << " (" << freq[i] << ")";
            }
            key_flag = IDM_SMOOTH_EXCESSRISK;
        }

            break;

        case ID_MAPANALYSIS_CHOROPLETH_STDDEV:
        {

            myP->SetTitle("Standard Deviation: " + m_fieldName);
            myP->SetTitle2(m_fieldName);
            if (DynS.StDev() > 0) {
                myP->numBreaks = 7;
                PickColorSet(3, 7, false);

                numClasses = 5;
                breakVal.clear();
                breakVal.resize(numClasses + 2);

                double stdVal[5];
                stdVal[0] = DynS.Mean() - (DynS.StDev() * 2.0);
                stdVal[1] = DynS.Mean() - (DynS.StDev() * 1.0);
                stdVal[2] = DynS.Mean();
                stdVal[3] = DynS.Mean() + (DynS.StDev() * 1.0);
                stdVal[4] = DynS.Mean() + (DynS.StDev() * 2.0);

                for (int j = 0; j < numClasses; j++)
                    breakVal.at(j) = stdVal[j];

                ComputeFrequency(breakVal, numClasses, freq);

                breakVal_reserve.clear();
                breakVal_reserve.resize(myP->numBreaks + 1);
                breakVal_reserve.at(0) = DynS.Mean() - (DynS.StDev() * 3.0);
                breakVal_reserve.at(1) = stdVal[0];
                breakVal_reserve.at(2) = stdVal[1];
                breakVal_reserve.at(3) = stdVal[2];
                breakVal_reserve.at(4) = stdVal[2];
                breakVal_reserve.at(5) = stdVal[3];
                breakVal_reserve.at(6) = stdVal[4];
                breakVal_reserve.at(7) = DynS.Mean() + (DynS.StDev() * 3.0);

                myP->m_str[0] = wxEmptyString;
                myP->m_str[0] << "<" << breakVal.at(0) << " ("
                    << freq[0] << ")";

                int index = 0;
                for (index = 1; index < 3; index++) {
                    myP->m_str[index] = wxEmptyString;
                    myP->m_str[index] << breakVal.at(index - 1) << " - "
                        << breakVal.at(index) << " (" << freq[index] << ")";
                }

                myP->m_str[3] = wxEmptyString;
                myP->m_str[3] << "Mean = " << DynS.Mean();
                //freq[3] = 0;

                for (index = 3; index < numClasses; index++) {
                    myP->m_str[index + 1] = wxEmptyString;
                    myP->m_str[index + 1] << breakVal.at(index - 1) << " - "
                        << breakVal.at(index) << " (" << freq[index] << ")";
                }
                myP->m_str[6] = wxEmptyString;
                myP->m_str[6] << "> " << breakVal.at(index - 1) << " ("
                    << freq[index] << ")";
            } else {
                myP->numBreaks = 1;
                PickColorSet(3, 7, false);
                myP->m_str[0] = "SDev = 0 ";
                return;
            }

            int i = 0, k = 0;
            while (i < numRecords) {
                if (fieldValue.at(i).value < breakVal.at(numClasses - 1)) {
                    if (fieldValue.at(i).value < breakVal.at(k)) {
                        while (fieldValue.at(i).value < breakVal.at(k) 
							   && i < numRecords) {
                            if (k > 2) {
								c_id.at(fieldValue.at(i).recordIndex) = k + 1;
							} else {
								c_id.at(fieldValue.at(i).recordIndex) = k;
							}
                            i++;
                        }
                    }
                    k++;
                } else if (fieldValue.at(i).value ==
						   breakVal.at(numClasses - 1)) {
                    c_id.at(i) = numClasses - 1;
                    i++;
                } else {
                    c_id.at(fieldValue.at(i).recordIndex) = numClasses + 1;
                    i++;
                }
            }

            for (i = 0; i < numClasses + 2; i++) colors.at(i) = Color.at(i);

            key_flag = ID_MAPANALYSIS_CHOROPLETH_STDDEV;

        }

            break;

        case ID_MAPANALYSIS_CHOROPLETH_QUANTILE:
        {
            myP->SetTitle("Quantile: " + m_fieldName);
            myP->SetTitle2(m_fieldName);

			int num_values = 1;
			double cur_val = fieldValue[0].value;
			for (int i=0, iend=gObservation; i<iend; i++) {
				if (fieldValue[i].value != cur_val) {
					num_values++;
					cur_val = fieldValue[i].value;
				}
			}
			//LOG(num_values);
			std::vector<double> uniqueVals(num_values);
			cur_val = wxMin(2*fieldValue[0].value-1000,
							0.5*fieldValue[0].value-1000);
			int u_val_ind=0;
			//LOG_MSG("unique values: ");
			for (int i=0, iend=gObservation; i<iend; i++) {
				//LOG_MSG(wxString::Format("fieldValue[%d].value=%f, cur_val=%f",
				//						 i, fieldValue[i].value, cur_val));
	
				if (fieldValue[i].value > cur_val) {
					cur_val = fieldValue[i].value;
					uniqueVals[u_val_ind] = cur_val;
					//LOG_MSG(wxString::Format("uniquVals[%d] = %f",
					//						 u_val_ind, cur_val));
					u_val_ind++;
				}
			}
	
            CMapQuantileDlg m_QuantileDialog(num_values,
											 num_values < gObservation,
											 this);

            if (m_QuantileDialog.ShowModal() != wxID_OK) {
				IDChoropleth = ID_MAP_RESET;
				break;
			}

            numClasses = m_QuantileDialog.m_classes->GetValue();
            myP->numBreaks = numClasses;
            PickColorSet(1, numClasses, false);
			
			int class_sz = gObservation / numClasses;
			int class_sz_rem = gObservation % numClasses;
			std::vector<int> classSz(numClasses, class_sz);
			for (int i=0; i<class_sz_rem; i++) classSz[(numClasses-1)-i]++;
			std::vector<int> lowInd(numClasses);
			std::vector<int> highInd(numClasses);
			std::vector<double> low(numClasses);
			std::vector<double> high(numClasses);			
			int ind = 0;
			for (int i=0; i<numClasses; i++) {
				low[i] = fieldValue[ind].value;
				lowInd[i] = ind;
				ind += classSz[i];
				high[i] = fieldValue[ind-1].value;
				highInd[i] = ind-1;
				LOG_MSG(wxString::Format("lowInd[%d]=%d, highInd[%d]=%d",
										 i, lowInd[i], i, highInd[i]));
				LOG_MSG(wxString::Format("low[%d]=%f, high[%d]=%f, "
										 "classSz[%d]=%d", i, low[i],
										 i, high[i], i, classSz[i])); 
			}

			int example_val = 0;
			int example_cat = 0;
			bool dup_on_break = false;
			for (int i=0; i<numClasses-1; i++) {
				if (high[i] == low[i+1]) {
					example_val = high[i];
					example_cat = i;
					dup_on_break = true;
				}
			}
			
			if (dup_on_break) {
				wxString cat1("");
				wxString cat2("");
				if (example_cat == 0) {
					cat1 = "first"; cat2 = "second";
				} else if (example_cat == 1) {
					cat1 = "second"; cat2 = "third";
				}
				else if (example_cat == 2) {
					cat1 = "third"; cat2 = "fourth";
				}
				else if (example_cat == 3) {
					cat1 = "fourth"; cat2 = "fifth";
				}
				else if (example_cat == 4) {
					cat1 = "fifth"; cat2 = "sixth";
				}
				else if (example_cat == 5) {
					cat1 = "sixth"; cat2 = "seventh";
				}
				else if (example_cat == 6) {
					cat1 = "seventh"; cat2 = "eighth";
				}
				else if (example_cat == 7) {
					cat1 = "eighth"; cat2 = "ninth";
				} else {
					cat1 << example_cat+1 << "th";
					cat2 << example_cat+2 << "th";
				}
				wxString m;
				m << "Warning: A valid Quantile map cannot be created for ";
				m << "field " << m_fieldName <<" with " << numClasses;
				m << " categories since a duplicate value " << example_val;
				m << " would occur in both the " << cat1 << " and " << cat2;
				m << " categories. Try a different number of categories, or ";
				m << " choose a different map type.";
				wxMessageBox(m);
				IDChoropleth = ID_MAP_RESET;
				break;
			}

			// Note: The code to correct duplicates that span multiple
			// categories below does not work when a single value spans
			// two or more categories.  We will leave this code for now, but
			// it will never be executed since the above will check for
			// duplicate values that span categories.
			
			// Check for duplicate values that occur along breaks
			// and shift them into the class that has the largest
			// number of those values already.
			
			dup_on_break = false;
			for (int i=0; i<numClasses-1; i++) {
				float breakVal = high[i];
				if (high[i] == low[i+1]) {
					// duplicate values occur at this break.  Figure out
					// which class has more of the duplicate values currently.
					dup_on_break = true;
					int class1_cnt=0;
					int class2_cnt=0;
					bool done = false;
					int j=highInd[i];
					while (!done && j>=0 &&
						   class1_cnt<=classSz[i])
					{
						if (fieldValue[j].value < breakVal) {
							done = true;
						} else {
							j--;
							class1_cnt++;
						}
					}
					done = false;
					j=lowInd[i+1];
					while (!done && j<gObservation &&
						   class2_cnt<=classSz[i+1])
					{
						if (fieldValue[j].value > breakVal) {
							done = true;
						} else {
							j++;
							class2_cnt++;
						}
					}
					if (class1_cnt >= class2_cnt) {
						// bump duplicates into lower category
						highInd[i] += class2_cnt;
						lowInd[i+1] += class2_cnt;
						low[i+1] = fieldValue[lowInd[i+1]].value;
						classSz[i] += class2_cnt;
						classSz[i+1] -= class2_cnt;
					} else {
						// bump duplicates into upper category
						highInd[i] -= class1_cnt;
						lowInd[i+1] -= class1_cnt;
						high[i] = fieldValue[highInd[i]].value;
						classSz[i+1] += class1_cnt;
						classSz[i] -= class1_cnt;
					}
				}
			}
			if (dup_on_break) {
				LOG_MSG("Duplicates on breaks found, adjustments made:");
				for (int i=0; i<numClasses; i++) {
					LOG_MSG(wxString::Format("lowInd[%d]=%d, highInd[%d]=%d",
											 i, lowInd[i], i, highInd[i]));
					LOG_MSG(wxString::Format("low[%d]=%f, high[%d]=%f, "
											 "classSz[%d]=%d", i, low[i],
											 i, high[i], i, classSz[i])); 
				}
			}
			dup_on_break = false;
			int max_prev_class_sz = class_sz;
			if (class_sz_rem > 0) max_prev_class_sz++;
			for (int i=0; i<numClasses; i++) {
				if (classSz[i] > max_prev_class_sz || classSz[i] < class_sz) {
					dup_on_break = true;
				}
			}
			if (dup_on_break) {
				wxMessageBox("Warning: A duplicate value occured along a category "
							 "break.  Duplicate values have been moved into either "
							 "a lower or an upper category so as to minimize "
							 "category-size imbalance, but while not splitting "
							 "duplicate values accross two or more categories.");
				LOG_MSG("A duplicate adjustment occured that caused a "
						"unbalanced quantile category");
			}
						
			std::vector<int> obsFreq(numClasses,0);
			int cur_class=0;
			for (int i=0; i<gObservation; i++) {
				if (fieldValue[i].value > high[cur_class]) cur_class++;
				obsFreq[cur_class]++;
			}
			
            for (int i=0; i < numClasses; i++) colors.at(i) = Color.at(i);

			int col_num = grid_base->FindColId(m_gVar1);
			int col_type = grid_base->col_data[col_num]->type;
			for (int i=0; i<numClasses; i++) {
				wxString msg;
				if (col_type == GeoDaConst::long64_type) {
					msg = wxString::Format("[%d:%d] (%d)", (int) low[i],
										   (int) high[i], obsFreq[i]);
				} else {
					msg = wxString::Format("[%.4g:%.4g] (%d)",
										   low[i], high[i], obsFreq[i]);
				}
				myP->m_str[i] = msg;
			}

			// assign each record the appropriate color value
			int cur_class_ind = 0;
			for (int i=0, iend=gObservation; i<iend; i++) {
				if (fieldValue.at(i).value > high.at(cur_class_ind)) {
					cur_class_ind++;
				}
				c_id.at(fieldValue.at(i).recordIndex) = cur_class_ind;
			}
			
            key_flag = ID_MAPANALYSIS_CHOROPLETH_QUANTILE;
			
        }
            break;

        case ID_MAP_HINGE_15:
        case ID_MAP_HINGE_30:
        {

            double quantile[4], coef;
            if (IDChoropleth == ID_MAP_HINGE_30) {
                coef = 3;
                key_flag = ID_MAP_HINGE_30;
                myP->SetTitle("Box Map (Hinge=3.0) : " + m_fieldName);
            } else {
                coef = 1.5;
                key_flag = ID_MAP_HINGE_15;
                myP->SetTitle("Box Map (Hinge=1.5) : " + m_fieldName);
            }
            myP->SetTitle2(m_fieldName);

            myP->numBreaks = 6;
            PickColorSet(2, 6, false);


            wxString str[6];
            str[0] = " Lower outlier";
            str[1] = " < 25%";
            str[2] = " 25% - 50%";
            str[3] = " 50% - 75%";
            str[4] = " > 75%";
            str[5] = " Upper outlier";

            numClasses = 5;
            breakVal.clear();
            breakVal.resize(numClasses);

            //for (int i = 0; i < 3; i++) {
            //    if ((gObservation % 4) > 0) {
            //        // quantile[i] = (fieldValue.at(int(gObservation*(i+1)/4)).value +
            //        //   fieldValue.at(int(gObservation*(i+1)/4) + 1).value) / 2;
            //        quantile[i] = (fieldValue.at(int(gObservation * (i + 1) / 4) - 1).value +
            //        fieldValue.at(int(gObservation * (i + 1) / 4)).value) / 2;
            //    } else {
            //        quantile[i] = fieldValue.at(gObservation * (i + 1) / 4 - 1).value;
			//	}
			//}
            //quantile[i] = fieldValue.at(gObservation - 1).value + 1;			
			
			// Median
			if (gObservation % 2 == 1) { // odd
				quantile[1] = fieldValue.at((gObservation+1)/2 - 1).value;
			} else { // even
				quantile[1] = (fieldValue.at(gObservation/2 - 1).value +
							   fieldValue.at(gObservation/2).value)/2.0;
			}
			if (gObservation >= 5) {
				quantile[0] = fieldValue.at((gObservation+1)/4 - 1).value;//Q1
				quantile[2] = fieldValue.at(3*(gObservation+1)/4 - 1).value;//Q3
			} else {
				quantile[0] = fieldValue.at(0).value; // Q1
				quantile[2] = fieldValue.at(gObservation-1).value; // Q3
			}
			
            double IQR = quantile[2] - quantile[0];

            for (int cnt = 0; cnt < 5; cnt++) {
                if (cnt == 0)
                    breakVal.at(0) = (quantile[0] - coef * IQR);
                else if (cnt == 4)
                    breakVal.at(4) = (quantile[2] + coef * IQR);
                else
                    breakVal.at(cnt) = quantile[cnt - 1];
            }

            ComputeFrequency(breakVal, numClasses, freq);
            int index = 0;
            breakVal_reserve.clear();
            breakVal_reserve.resize(myP->numBreaks + 1);
            breakVal_reserve.at(index) = (quantile[0] - 2 * coef * IQR);
            for (index = 0; index < 5; index++) {
                breakVal_reserve.at(index + 1) = breakVal.at(index);
            }
            breakVal_reserve.at(6) = (quantile[2] + 2 * coef * IQR);

            for (int i = 0; i < numClasses + 1; i++) colors.at(i) = Color.at(i);

            int i = 0;
            int k = 0;
            while (i < numRecords) {
                if (fieldValue.at(i).value < breakVal.at(numClasses - 1)) {
                    if (fieldValue.at(i).value < breakVal.at(k)) {
                        while (fieldValue.at(i).value < breakVal.at(k) && i < numRecords) {
                            c_id.at(fieldValue.at(i).recordIndex) = k;
                            i++;
                        }
                    }
                    k++;
                } else if (fieldValue.at(i).value == breakVal.at(numClasses - 1)) {
                    c_id.at(i) = numClasses - 1;
                    i++;
                } else {
                    c_id.at(fieldValue.at(i).recordIndex) = numClasses;
                    i++;
                }
            }

            for (i = 0; i <= numClasses; i++) {
                myP->m_str[i] = wxEmptyString;
                myP->m_str[i] << str[i] << " (" << freq[i] << ")";
            }
        }
            break;


        case ID_MAPANALYSIS_CHOROPLETH_PERCENTILE:
        {
            myP->SetTitle("Percentile: " + m_fieldName);
            myP->SetTitle2(m_fieldName);
            PickColorSet(2, 6, false);

            wxString str[6];
            str[0] = "< 1%";
            str[1] = "  1% - 10%";
            str[2] = " 10% - 50%";
            str[3] = " 50% - 90%";
            str[4] = " 90% - 99%";
            str[5] = ">99%";

            numClasses = 6;
            breakVal.clear();
            breakVal.resize(numClasses);

            myP->numBreaks = 6;

            if (gObservation < 100) {
                breakVal.at(0) = (fieldValue.at(0).value + fieldValue.at(1).value) / 2;
                breakVal.at(4) = (fieldValue.at(numRecords - 2).value + fieldValue.at(numRecords - 1).value) / 2;
                breakVal.at(5) = fieldValue.at(numRecords - 1).value + 1;
            } else {
                breakVal.at(0) = (fieldValue.at(int(numRecords * 0.01) - 1).value + fieldValue.at(int(numRecords * 0.01)).value) / 2.;
                breakVal.at(4) = (fieldValue.at(int(numRecords * 0.99) - 1).value + fieldValue.at(int(numRecords * 0.99)).value) / 2.;
                breakVal.at(5) = fieldValue.at(numRecords - 1).value + 1;
            }
            breakVal.at(1) = (fieldValue.at(int(numRecords * 0.1) - 1).value + fieldValue.at(int(numRecords * 0.1)).value) / 2.;
            breakVal.at(2) = (fieldValue.at(int(numRecords * 0.5) - 1).value + fieldValue.at(int(numRecords * 0.5)).value) / 2.;
            breakVal.at(3) = (fieldValue.at(int(numRecords * 0.9) - 1).value + fieldValue.at(int(numRecords * 0.9)).value) / 2.;


            int index = 0;
            breakVal_reserve.clear();
            breakVal_reserve.resize(myP->numBreaks + 1);
            breakVal_reserve.at(index) = fieldValue.at(0).value;
            for (index = 0; index < 5; index++) {
                breakVal_reserve.at(index + 1) = breakVal.at(index);
            }
            breakVal_reserve.at(6) = fieldValue.at(gObservation - 1).value;

            ComputeFrequency(breakVal, numClasses, freq);

			bool warn_msg_shown_already = false;
            int k = 0, i = 0;
            for (i = 1; i < numClasses; i++) {
                if ((breakVal.at(i - 1) == breakVal.at(i)) ||
                    (freq[i - 1] == 0)) {
                    k = i;
                    freq[i] += freq[i - 1];
                    freq[i - 1] = 0;
					if (!warn_msg_shown_already) {
						wxMessageBox("Warning: Unable to uniquely sort records "
									 "into percentile categories since too many" 
									 " records have same value. "
									 "Records from previous categories are "
									 "bumped into next one. Try using a "
									 "different map.");
						warn_msg_shown_already = true;
					}
                }
            }

            for (i = 0; i < k; i++) {
                Color.at(i) = Color.at(k);
            }

            for (i = 0; i < numClasses; i++)
                colors.at(i) = Color.at(i);

            i = 0;
            k = 0;
            while (i < numRecords && k < numClasses) {
                while (fieldValue.at(i).value < breakVal.at(k) && i < numRecords) {
                    c_id.at(fieldValue.at(i).recordIndex) = k;
                    i++;
                }
                k++;
            }

            wxString xx;
            for (i = 0; i < numClasses; i++) {
                myP->m_str[i] = wxEmptyString;
                myP->m_str[i] << str[i] << " (" << freq[i] << ")";
            }

            key_flag = ID_MAPANALYSIS_CHOROPLETH_PERCENTILE;

        } // end of CASE : percentile
            break;


        case ID_MAPANALYSIS_NATURAL_BREAK:
        {
            myP->SetTitle("Natural Break: " + m_fieldName);
            myP->SetTitle2(m_fieldName);

			int num_values = 1;
			double cur_val = fieldValue[0].value;
			for (int i=0, iend=gObservation; i<iend; i++) {
				if (fieldValue[i].value != cur_val) {
					num_values++;
					cur_val = fieldValue[i].value;
				}
			}

            CMapQuantileDlg m_QuantileDialog(num_values, false, this);
            m_QuantileDialog.SetTitle("Natural Break");

            if (m_QuantileDialog.ShowModal() != wxID_OK) {
				IDChoropleth = ID_MAP_RESET;
				break;
			}

            numClasses = m_QuantileDialog.m_classes->GetValue();

            int index = 0;

            std::vector<double> gaps(numClasses);
            std::vector<int> gaps_index(numClasses);

            int m_intv = gObservation / numClasses;
            for (index = 0; index < numClasses - 1; index++) {
                gaps[index] = 0;
                gaps_index[index] = 0;
            }

            for (index = 0; index < gObservation - 1; index++) {
                double gp = fieldValue.at(index + 1).value - fieldValue.at(index).value;

                int idx;
                for (idx = 0; idx < numClasses - 1; idx++) {
                    if (gp > gaps[idx]) {
                        int kp;
                        for (kp = numClasses - 1; kp > idx; kp--) {
                            gaps[kp] = gaps[kp - 1];
                            gaps_index[kp] = gaps_index[kp - 1];
                        }
                        gaps_index[idx] = index;
                        gaps[idx] = gp;

                        break;
                    }
                }

            }

            myP->numBreaks = numClasses;
            PickColorSet(1, numClasses, false);

            breakVal.clear();
            breakVal.resize(numClasses + 1);
            breakVal_reserve.clear();
            breakVal_reserve.resize(numClasses + 1);


            for (index = 0; index < numClasses - 1; index++) {
                if (fieldValue[gaps_index[index]].value == fieldValue[gaps_index[index] + 1].value)
                    breakVal.at(index) = fieldValue[gaps_index[index]].value;
                else
                    breakVal.at(index) = (fieldValue[gaps_index[index]].value + fieldValue[gaps_index[index] + 1].value) / 2.0;

            }

            breakVal.at(numClasses - 1) = fieldValue.at(gObservation - 1).value;

            std::vector<MapSortElement> bValue(numClasses);

            for (index = 0; index < numClasses - 1; index++) {
                bValue.at(index).value = breakVal.at(index);
                bValue.at(index).recordIndex = index;
            }

            QuickSortFieldValT(bValue, 0, numClasses - 2);

            for (index = 0; index < numClasses - 1; index++) {
                breakVal.at(index) = bValue.at(index).value;
            }

            ComputeFrequency(breakVal, numClasses, freq);

            int i, k;
            breakVal_reserve.at(0) = fieldValue.at(0).value;
            for (i = 0; i < numClasses; i++) {
                breakVal_reserve.at(i + 1) = breakVal.at(i);
                colors.at(i) = Color.at(i);
            }

            //		myP->m_str[0] = wxEmptyString;
            //		myP->m_str[0] << " ~ " << breakVal.at(0) << "("
            //         << freq[0] << ")";

            for (i = 0; i < numClasses; i++) {
                myP->m_str[i] = wxEmptyString;
                myP->m_str[i] << breakVal_reserve.at(i) << " ~ "
                    << breakVal_reserve.at(i + 1) << " ("
                    << freq[i] << ")";
            }

            //		myP->m_str[i] = wxEmptyString;
            //		myP->m_str[i] << breakVal.at(i-1) << " ~ " << "("
            //         << freq[i] << ")";

            i = 0;
            k = 0;
            while (i < numRecords && k < numClasses) {
                while ((i < numRecords) && (fieldValue.at(i).value < breakVal.at(k))) {
                    c_id.at(fieldValue.at(i).recordIndex) = k;
                    i++;
                }
                k++;
            }

            key_flag = ID_MAPANALYSIS_NATURAL_BREAK;

        }
            break;

        case ID_MAPANALYSIS_UNIQUE_VALUE:
        {
            myP->SetTitle("Unique Value: " + m_fieldName);


            int index = 0;
            numClasses = 1;
            for (index = 0; index < gObservation - 1; index++) {
                if (fieldValue.at(index).value != fieldValue.at(index + 1).value)
                    numClasses++;
            }

            if (numClasses > 9) {
                wxMessageBox("Number of unique values can not exceed 10!");
                return;
            }

            myP->numBreaks = numClasses;
            PickColorSet(1, numClasses, false);

            breakVal.clear();
            breakVal.resize(numClasses);


            int idx = 1;
            breakVal.at(0) = fieldValue.at(0).value;
            for (index = 0; index < gObservation - 1; index++) {
                if (fieldValue.at(index).value != fieldValue.at(index + 1).value) {
                    breakVal.at(idx) = fieldValue.at(index + 1).value;
                    idx++;
                }
            }

            ComputeFrequencyUnique(breakVal, numClasses, freq);


            index = 0;
            breakVal_reserve.clear();
            breakVal_reserve.resize(myP->numBreaks + 1);
            for (index = 0; index < myP->numBreaks + 1; index++) {
                breakVal_reserve.at(index) = index;
            }


            int i, k;
            for (i = 0; i < numClasses; i++)
                colors.at(i) = Color.at(i);

            for (i = 0; i < numClasses; i++) {
                myP->m_str[i] = wxEmptyString;
                myP->m_str[i] << breakVal.at(i) << "("
                    << freq[i] << ")";
            }

            i = 0;
            k = 0;
            while (i < numRecords && k < numClasses) {
                while ((i < numRecords) && (fieldValue.at(i).value == breakVal.at(k))) {
                    c_id.at(fieldValue.at(i).recordIndex) = k;
                    i++;
                }
                k++;
            }

            key_flag = ID_MAPANALYSIS_UNIQUE_VALUE;
        }
            break;

        case ID_MAPANALYSIS_EQUALINTERVAL:
        {
            LOG_MSG("Entering case IDChoropleth == ID_MAPANALYSIS_EQUALINTERVAL");
            myP->SetTitle("Equal Interval: " + m_fieldName);
            myP->SetTitle2(m_fieldName);

			int num_values = 1;
			double cur_val = fieldValue[0].value;
			for (int i=0, iend=gObservation; i<iend; i++) {
				if (fieldValue[i].value != cur_val) {
					num_values++;
					cur_val = fieldValue[i].value;
				}
			}
			
            CMapQuantileDlg m_QuantileDialog(num_values, false, this);
            m_QuantileDialog.SetTitle("Equal Interval");

            if (m_QuantileDialog.ShowModal() != wxID_OK) {
				IDChoropleth = ID_MAP_RESET;
				break;
			}

            wxString text;
            text << m_QuantileDialog.m_classes->GetValue();
            if (!text.IsNumber()) {
                wxMessageBox("Please type a NUMBER!");
                return;
            }
            long val;
            text.ToLong(&val);
            numClasses = (int) val;

            myP->numBreaks = numClasses;
            PickColorSet(1, numClasses, false);

            breakVal.clear();
            breakVal.resize(numClasses);
            breakVal_reserve.clear();
            breakVal_reserve.resize(numClasses + 1);

            int index = 0;
            for (index = 0; index < numClasses - 1; index++) {
                breakVal.at(index) =
                    (index + 1)*(fieldValue.at(gObservation - 1).value
                    - fieldValue.at(0).value) / (numClasses)
                    + fieldValue.at(0).value;
            }

            // breakVal.at(index) = fieldValue.at(gObservation-1).value + 1;
            breakVal.at(index) =
                breakVal.at(index - 1) + (fieldValue.at(gObservation - 1).value
                - fieldValue.at(0).value)
                / (numClasses)*1.0001;

            ComputeFrequency(breakVal, numClasses, freq);

            int k = 0, i = 0;

            breakVal_reserve.at(0) = fieldValue.at(0).value;
            for (i = 0; i < numClasses; i++) {
                breakVal_reserve.at(i + 1) = breakVal.at(i);
                colors.at(i) = Color.at(i);
            }

            //		myP->m_str[0] = wxEmptyString;
            //		myP->m_str[0] << " ~ " << breakVal.at(0) << " ("
            //         << freq[0] << ")";

            for (index = 0; index < numClasses; index++) {
                myP->m_str[index] = wxEmptyString;
                myP->m_str[index] << breakVal_reserve.at(index) << " ~ "
                    << breakVal_reserve.at(index + 1)
                    << " (" << freq[index] << ")";
            }

            //		myP->m_str[numClasses-1] = wxEmptyString;
            //		myP->m_str[numClasses-1] <<  breakVal.at(index-1)
            //         << " ~ (" << freq[index] << ")";

            i = 0;
            k = 0;
            while (i < numRecords && k < numClasses) {
                while ((i < numRecords) && (fieldValue.at(i).value < breakVal.at(k))) {
                    c_id.at(fieldValue.at(i).recordIndex) = k;
                    i++;
                }
                k++;
            }

            key_flag = ID_MAPANALYSIS_EQUALINTERVAL;

            LOG_MSG("Exiting case IDChoropleth == ID_MAPANALYSIS_EQUALINTERVAL");
        }
            break;

        default:
            break;

    }

	if (IDChoropleth == ID_MAP_RESET) {
		key_flag = 0;
	    m_nSaveRateValue = 0;
		IDChoropleth = ID_MAP_RESET;
		IDSmoothType = ID_MAP_RESET;
		IDMapType = ID_MAP_RESET;
		numClasses = 1;
		myP->SetTitle("Map - " + GenUtils::GetTheFileTitle(fullPathShapefileName));
		myP->numBreaks = 1;
		colors.at(0) = selectable_fill_color;
		for (int i = 0; i < gObservation; i++) c_id.at(i) = 0;
		myP->m_str[0] = wxEmptyString;
	}
	
    Refresh(true);

    myP->m_left->Refresh(true);
    myP->UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
    //Update the toolbar frame title
    MyFrame::theFrame->SetTitle(myP->GetTitle());

    LOG_MSG("Exiting MapCanvas::ChoroplethMapUpdate");
}

void MapCanvas::OnSaveRates(wxString title) {
    if (m_nSaveRateValue > 0) {
		std::vector<SaveToTableEntry> data(1);
		
		std::vector<double> dt(gObservation);
        for (int i=0; i<gObservation; i++) {
            dt[fieldValue.at(i).recordIndex] = fieldValue.at(i).value;
        }
		data[0].type = GeoDaConst::double_type;
		data[0].d_val = &dt;
		data[0].label = "Rate";
		
		if (m_nSaveRateValue == 1) {
			data[0].field_default = "R_RAWRATE";
		} else if (m_nSaveRateValue == 2) {
			data[0].field_default = "R_EXCESS";
		} else if (m_nSaveRateValue == 3) {
			data[0].field_default = "R_EBS";
		} else if (m_nSaveRateValue == 4) {
			data[0].field_default = "R_SPATRATE";
		} else { // m_nSaveRateValue == 5
			data[0].field_default = "R_SPATEBS";
		}
		
        SaveToTableDlg dlg(grid_base, this, data,
						   "Save Rates - " + title,
						   wxDefaultPosition, wxSize(400,400));
        if (dlg.ShowModal() == wxID_OK) m_nSaveRateValue = 0;
    } else {
        wxMessageBox("There are no smoothed rates to be saved.");
	}
}

// ---------------------------------------------------------------------------
// MapLegend
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MapLegend, TemplateLegend)
EVT_MOUSE_EVENTS(MapLegend::OnEvent)
END_EVENT_TABLE()

MapLegend::MapLegend(wxWindow *parent, const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, pos, size) {
    myP = (MapFrame*) parent;
    count_color = 0;
    d_rect = 20;
    px = 10;
    py = 40;
    m_w = 15;
    m_l = 20;
}

MapLegend::~MapLegend() {
    LOG_MSG("In MapLegend::~MapLegend");
}

#include "DialogTools/UserConfigDlg.h"

void MapLegend::OnEvent(wxMouseEvent& event) {
    if (event.RightUp()) {
        LOG_MSG("MapLegend::OnEvent, event.RightUp() == true");
        wxMenu* optMenu =
            wxXmlResource::Get()->LoadMenu("ID_MAP_VIEW_MENU_LEGEND");
		((MapFrame*) template_frame)->UpdateMenuCheckMarks(optMenu);
        PopupMenu(optMenu, event.GetPosition());
        return;
    }

    if (event.LeftDown())
        //	if(event.LeftDClick())
    {
        LOG_MSG("MapLegend::OnEvent, event.LeftDown() == true");
        wxPoint pt(event.GetPosition());
        int x, y;
        CalcUnscrolledPosition(pt.x, pt.y, &x, &y);

        int i;

        int numRect, cur_y = py;
        numRect = myP->numBreaks;


        for (i = 0; i < numRect; i++) {

            if ((x > px) && (x < px + m_l) && (y > cur_y - 3) &&
                (y < cur_y + m_w)) {
                
                wxColour col = myP->canvas->colors.at(i);

                wxColourData data;
                data.SetColour(col);
                data.SetChooseFull(true);
                int ki;
                for (ki = 0; ki < 16; ki++) {
                    wxColour colour(ki * 16, ki * 16, ki * 16);
                    data.SetCustomColour(i, colour);
                }

                wxColourDialog dialog(this, &data);
                dialog.SetTitle("Choose the mapping colour");
                if (dialog.ShowModal() == wxID_OK) {
                    wxColourData retData = dialog.GetColourData();
                    wxColour co = retData.GetColour();
                    myP->canvas->colors.at(i) = co;
                    myP->canvas->Refresh();
                    Refresh();
                }

                return;
            }
            else if ((i == 0) || (i == numRect - 1))
                ;
            else if ((x > px + m_l) && (y > cur_y - 3) && (y < cur_y + m_w)) {
                //  Do user config stuff.

                if ((myP->canvas->key_flag == ID_MAPANALYSIS_EQUALINTERVAL)
                    || (myP->canvas->key_flag == ID_MAPANALYSIS_NATURAL_BREAK)) {
                    CUserConfigDlg dlg(this);
                    wxString str;
                    str << wxEmptyString;
                    str << "Category " << i + 1;
                    dlg.m_label->SetLabel(str);

                    str = wxEmptyString;
                    str << myP->canvas->breakVal_reserve.at(i);
                    dlg.s_int = str;
                    str = wxEmptyString;
                    str << myP->canvas->breakVal_reserve.at(i + 1);
                    dlg.s_int2 = str;

                    if (dlg.ShowModal() == wxID_OK) {

                        //  Do some value changes

                        double val1, val2;
                        (dlg.m_min->GetValue()).ToDouble(&val1);
                        (dlg.m_max->GetValue()).ToDouble(&val2);


                        if (val1 >= val2) {
                            wxMessageBox("Wrong values!");
                            return;
                        }


                        if (val1 < myP->canvas->breakVal_reserve.at(i - 1)) {
                            wxMessageBox("Wrong values!");
                            return;
                        }

                        if (val2 > myP->canvas->breakVal_reserve.at(i + 2)) {
                            wxMessageBox("Wrong values!");
                            return;
                        }

                        myP->canvas->breakVal_reserve.at(i) = val1;
                        myP->canvas->breakVal_reserve.at(i + 1) = val2;

                        myP->canvas->UserConfigUpdate();
                        myP->SetTitle("User Configure: " + myP->GetTitle2());
                        myP->canvas->Refresh();
                        Refresh();
                    }
                }
                return;
            }

            cur_y += d_rect;
        }
    }
}

void MapLegend::OnDraw(wxDC& dc) {
    //LOG_MSG("Entering MapLegend::OnDraw");
    wxFont m_font(*wxSMALL_FONT);
    m_font.SetPointSize(10);
    dc.SetFont(m_font);

    wxString stt = myP->GetTitle();
    stt = stt.AfterLast('\\');
    stt = stt.AfterLast('/');
    dc.DrawText(stt, 5, 15);

    int numRect, cur_y = py;
    numRect = myP->numBreaks;

    dc.SetPen(*wxBLACK_PEN);
    wxBrush brush;

    int i;
    if (myP->canvas->key_flag != ID_MAPANALYSIS_CHOROPLETH_STDDEV) {
        for (i = 0; i < numRect; i++) {
            brush.SetColour(myP->canvas->colors.at(i));
            dc.SetBrush(brush);

            dc.DrawText(myP->m_str[i], (px + m_l + 10), cur_y - (m_w / 2));
            dc.DrawRectangle(px, cur_y - 3, m_l, m_w);
            cur_y += d_rect;
        }
    } else {
        for (i = 0; i < numRect; i++) {
            brush.SetColour(myP->canvas->colors.at(i));
            dc.SetBrush(brush);

            dc.DrawText(myP->m_str[i], (px + m_l + 10), cur_y - (m_w / 2));
            if (i != 3) dc.DrawRectangle(px, cur_y - 3, m_l, m_w);

            cur_y += d_rect;
        }
    }
	
    //LOG_MSG("Exiting MapLegend::OnDraw");
}


// ---------------------------------------------------------------------------
// MapFrame
// ---------------------------------------------------------------------------

MapFrame::MapFrame(const wxString& fullPathShapefileName,
				   wxFrame *parent, Project* project,
				   const wxString& title, const wxPoint& pos,
				   const wxSize& size, const long style, ProgressDlg* p_dlg)
: TemplateFrame(parent, project, title, pos, size, style)
{
    LOG_MSG("Entering MapFrame::MapFrame");
    old_style = true;
	my_children.Append(this);
    SetSizeHints(100, 100);

    numBreaks = 0;
    numBreaks2 = 0;
    m_splitter = new wxSplitterWindow(this);

    // This how the map is created
    // m_left is an handle for the legend
    m_left = new MapLegend(m_splitter, wxPoint(0, 0), wxSize(0, 0));
    // m_right is an handle for the map
    m_right = new MapCanvas(project->GetGridBase(),
							project->GetMainDir()+project->GetMainName()+".shp",
							m_splitter,
							wxPoint(0, 0), wxSize(0, 0), p_dlg);
    ((MapLegend*) m_left)->myP = this;
    ((MapCanvas*) m_right)->myP = this;
    canvas = (MapCanvas*) m_right;
    template_legend = (TemplateLegend*) m_left;
    template_canvas = canvas;
    template_canvas->template_frame = this;
	template_legend->template_frame = this;
    m_splitter->SplitVertically(m_left, m_right, 100);
    numBreaks = 1;
    m_str[0] = wxEmptyString;
    SetTitle("Map - " + project->GetMainName());
    Show(true);
    LOG_MSG("Exiting MapFrame::MapFrame");
}

MapFrame::~MapFrame() {
    LOG_MSG("Entering MapFrame::~MapFrame");
	my_children.DeleteObject(this);
    LOG_MSG("Exiting MapFrame::~MapFrame");
}

void MapFrame::Update() {
	wxClientDC dc(canvas);
    //wxBufferedDC p_dc(&c_dc);
	//wxGCDC dc(p_dc);
	//wxGraphicsContext* gc = dc.GetGraphicsContext();
	//if (gc) gc->SetAntialiasMode(wxANTIALIAS_NONE);
    canvas->PrepareDC(dc);
    canvas->Selection(&dc);
}

void MapFrame::UpdateMenuBar() {
    // This method disables all menu items that are not
    // in the specified menus.
    wxMenuBar* mb = GetMenuBar();
    if (!mb) return;
    wxMenu* menu = NULL;
    wxString menuText = wxEmptyString;
    int menuCnt = mb->GetMenuCount();
    for (int i = 0; i < menuCnt; i++) {
        menu = mb->GetMenu(i);
        menuText = mb->GetMenuLabelText(i);
        if ((menuText != "File") &&
            (menuText != "Edit") &&
            (menuText != "Tools") &&
            (menuText != "Map") &&
            (menuText != "Explore") &&
            (menuText != "Space") &&
            (menuText != "Methods") &&
            (menuText != "Help")) {
            GeneralWxUtils::EnableMenuAll(mb, menuText, false);
        }
    }

    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_OPEN_NEW_MAP"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_NEW_MAP_WINDOW"), true);

    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_IN"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_OUT"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_PAN_MODE"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTABLE_FILL_COLOR"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_HIGHLIGHT_COLOR"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_CANVAS_BACKGROUND_COLOR"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_MAP_ADDMEANCENTERS"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_MAP_ADDCENTROIDS"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_CANVAS_IMAGE_AS"), true);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_SELECTED_TO_COLUMN"), true);
}

void MapFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(true);
}

void MapFrame::OnActivate(wxActivateEvent& event) {
	canvas->gSelect1.x = -1; // invalidate first selection point
    if (event.GetActive()) {
        RegisterAsActive("MapFrame", GetTitle());
    }
    if (event.GetActive() && canvas) canvas->SetFocus();
}

void MapFrame::UpdateMenuBarCheckMarks(wxMenuBar* menuBar) {
    LOG_MSG("Entering MapFrame::UpdateMenuBarCheckMarks");
    if (menuBar == NULL) {
        LOG_MSG("MapFrame::UpdateMenuBarCheckMarks: menuBar == NULL");
        return;
    }
    int menu;
    menu = menuBar->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MapFrame::UpdateMenuBarCheckMarks: Options menu not found");
	} else {
        UpdateMenuCheckMarks(menuBar->GetMenu(menu));
	}
    menu = menuBar->FindMenu("Map");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MapFrame::UpdateMenuBarCheckMarks: Map menu not found");
	} else {
        UpdateMenuCheckMarks(menuBar->GetMenu(menu));
	}
    LOG_MSG("Exiting MapFrame::UpdateMenuBarCheckMarks");
}

void MapFrame::UpdateMenuCheckMarks(wxMenu* menu) {
    LOG_MSG("Entering MapFrame::UpdateMenuCheckMarks");

    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
        canvas->IDMapType == ID_MAPANALYSIS_CHOROPLETH_QUANTILE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
        canvas->IDMapType == ID_MAPANALYSIS_CHOROPLETH_PERCENTILE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAP_HINGE_15"),
        canvas->IDMapType == ID_MAP_HINGE_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAP_HINGE_30"),
        canvas->IDMapType == ID_MAP_HINGE_30);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
        canvas->IDMapType == ID_MAPANALYSIS_CHOROPLETH_STDDEV);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUE"),
        canvas->IDMapType == ID_MAPANALYSIS_UNIQUE_VALUE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_EQUALINTERVAL"),
        canvas->IDMapType == ID_MAPANALYSIS_EQUALINTERVAL);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_NATURAL_BREAK"),
        canvas->IDMapType == ID_MAPANALYSIS_NATURAL_BREAK);

    GeneralWxUtils::CheckMenuItem(menu, XRCID("IDM_SMOOTH_RAWRATE"),
        canvas->IDSmoothType == IDM_SMOOTH_RAWRATE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("IDM_SMOOTH_EXCESSRISK"),
        canvas->IDSmoothType == IDM_SMOOTH_EXCESSRISK);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("IDM_EMPERICAL_BAYES_SMOOTHER"),
        canvas->IDSmoothType == IDM_EMPERICAL_BAYES_SMOOTHER);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("IDM_SPATIAL_RATE_SMOOTHER"),
        canvas->IDSmoothType == IDM_SPATIAL_RATE_SMOOTHER);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("IDM_SPATIAL_EMPIRICAL_BAYES"),
        canvas->IDSmoothType == IDM_SPATIAL_EMPIRICAL_BAYES);

	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
		canvas->IsSelectableOutlineVisible() );
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTION_MODE"),
        canvas->work_mode == MODE_SELECT);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_RECT"),
        canvas->work_mode == MODE_SELECT &&
        canvas->sel_mode == SELECTION_RECT);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_LINE"),
        canvas->work_mode == MODE_SELECT &&
        canvas->sel_mode == SELECTION_LINE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_CIRCLE"),
        canvas->work_mode == MODE_SELECT &&
        canvas->sel_mode == SELECTION_CIRCLE);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_ZOOM_IN"),
        canvas->work_mode == MODE_ZOOMIN);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_ZOOM_OUT"),
        canvas->work_mode == MODE_ZOOMOUT);
    // Full Extent zoom is not really a mode, so this will not be checkmarked.
    // MMM: Perhaps we should make it a mode.  It is in the new style canvas
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_FIT_TO_WINDOW_MODE"), false);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_PAN_MODE"),
        canvas->work_mode == MODE_PANNING);

    if (canvas->isLisaMap || canvas->isGetisOrdMap) {
        GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
            canvas->MapFrameSignificanceFilter == 1);
        GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
            canvas->MapFrameSignificanceFilter == 2);
        GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
            canvas->MapFrameSignificanceFilter == 3);
        GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
            canvas->MapFrameSignificanceFilter == 4);
    }
    LOG_MSG("Exiting MapFrame::UpdateMenuCheckMarks");
}


void MapFrame::MapMenus() {
    LOG_MSG("In MapFrame::MapMenus");
    wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
        true);
    // Map Options Menus
	wxMenu* optMenu;
	if (canvas->isLisaMap) {
		optMenu =
			wxXmlResource::Get()->LoadMenu("ID_LISAMAP_VIEW_MENU_OPTIONS");
	} else if (canvas->isGetisOrdMap) {
		optMenu =
			wxXmlResource::Get()->LoadMenu("ID_GETIS_ORD_VIEW_MENU_OPTIONS");
	} else {
		optMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_VIEW_MENU_OPTIONS");
	}
    GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
    UpdateMenuCheckMarks(optMenu);
    UpdateOptionMenuItems();

    // Enable all items in Map menu and map related toolbar items.
    GeneralWxUtils::EnableMenuAll(mb, "Map", true);
	MyFrame::theFrame->EnableTool(XRCID("ID_MAP_CHOICES"), true);
    //MyFrame::theFrame->EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"), true);
    //MyFrame::theFrame->EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"), true);
    //MyFrame::theFrame->EnableTool(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), true);
    //MyFrame::theFrame->EnableTool(XRCID("ID_MAP_HINGE_15"), true);
    //MyFrame::theFrame->EnableTool(XRCID("ID_MAP_HINGE_30"), true);
	// Formerly in toolbar.xrc
	//<object class="separator"/>
    //<object class="tool" name="ID_MAPANALYSIS_CHOROPLETH_QUANTILE">
	//  <bitmap>ToolBarBitmaps_25.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_25_disabled.png</bitmap2>
	//  <tooltip>Quantile Map</tooltip>
    //</object>
    //<object class="tool" name="ID_MAPANALYSIS_CHOROPLETH_PERCENTILE">
	//  <bitmap>ToolBarBitmaps_26.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_26_disabled.png</bitmap2>
	//  <tooltip>Percentile Map</tooltip>
    //</object>
    //<object class="tool" name="ID_MAPANALYSIS_CHOROPLETH_STDDEV">
	//  <bitmap>ToolBarBitmaps_27.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_27_disabled.png</bitmap2>
	//  <tooltip>Standard Deviation Map</tooltip>
    //</object>
    //<object class="tool" name="ID_MAP_HINGE_15">
	//  <bitmap>ToolBarBitmaps_28.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_28_disabled.png</bitmap2>
	//  <tooltip>Box Map (Hinge = 1.5)</tooltip>
    //</object>
    //<object class="tool" name="ID_MAP_HINGE_30">
	//  <bitmap>ToolBarBitmaps_29.png</bitmap>
	//  <bitmap2>ToolBarBitmaps_29_disabled.png</bitmap2>
	//  <tooltip>Box Map (Hinge = 3.0)</tooltip>
    //</object>	
}

void MapFrame::OnMove(wxMoveEvent& event) {
    event.Skip();
}

void MapFrame::OnSize(wxSizeEvent& event) {
    //LOG_MSG("Entering MapFrame::OnSize");
    // VZ: under MSW the size event carries the client size (quite
    //     unexpectedly) *except* for the very first one which has the full
    //     size... what should it really be? TODO: check under wxGTK
    // wxSize size1 = event.GetSize();
    // wxSize size2 = GetSize();
    // wxSize size3 = GetClientSize();
    event.Skip();
    //LOG_MSG("Exiting MapFrame::OnSize");
}

void MapFrame::OnClose(wxCloseEvent& event) {
    LOG_MSG("Entering/Exiting MapFrame::OnClose, Destroy() called");
    DeregisterAsActive();
    Destroy();
}

void MapFrame::OnMenuClose(wxCommandEvent& event) {
    LOG_MSG("Entering/Exiting MapFrame::OnMenuClose, Close() called");
    LOG_MSG("In MapFrame::OnMenuClose");
    Close();
}

void MapFrame::OnQuantile(wxCommandEvent& event) {
    LOG_MSG("Entering MapFrame::OnQuantile");

    VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());

    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_CHOROPLETH_QUANTILE;
    canvas->IDMapType = ID_MAPANALYSIS_CHOROPLETH_QUANTILE;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
    LOG_MSG("Exiting MapFrame::OnQuantile");
}

void MapFrame::OnPercentile(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnPercentile");

	VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());

    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_CHOROPLETH_PERCENTILE;
    canvas->IDMapType = ID_MAPANALYSIS_CHOROPLETH_PERCENTILE;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}

void MapFrame::OnHinge15(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnHinge15");

	VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());

    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->Hinge = 0;
    canvas->IDChoropleth = ID_MAP_HINGE_15;
    canvas->IDMapType = ID_MAP_HINGE_15;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}

void MapFrame::OnHinge30(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnHinge30");

	VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());

    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->Hinge = 1;
    canvas->IDChoropleth = ID_MAP_HINGE_30;
    canvas->IDMapType = ID_MAP_HINGE_30;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}

void MapFrame::OnStddev(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnStddev");

    VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());
	
    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_CHOROPLETH_STDDEV;
    canvas->IDMapType = ID_MAPANALYSIS_CHOROPLETH_STDDEV;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}

void MapFrame::OnNaturalBreak(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnNaturalBreak");

	VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());
	
    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_NATURAL_BREAK;
    canvas->IDMapType = ID_MAPANALYSIS_NATURAL_BREAK;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}

void MapFrame::OnUniqueValue(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnUniqueValue");

    VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());
	
    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_UNIQUE_VALUE;
    canvas->IDMapType = ID_MAPANALYSIS_UNIQUE_VALUE;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
    Raise();
}

void MapFrame::OnEqualInterval(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnEqualInterval");

    VariableSettingsDlg VS(true, gCompleteFileName, gObservation,
						   m_gVar1, m_gVar2, m_gX, m_gY,
						   m_VarDefault,
						   project->GetGridBase());
	
    if (VS.ShowModal() != wxID_OK) return;

    m_VarDefault = VS.m_CheckDefault;
    m_gVar1 = VS.m_Var1;
    m_gVar2 = VS.m_Var2;

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAPANALYSIS_EQUALINTERVAL;
    canvas->IDMapType = ID_MAPANALYSIS_EQUALINTERVAL;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    Raise();
}


void MapFrame::OnRawrate(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnRawrate");

	RateSmootherDlg RS(project->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 4);
	if (RS.ShowModal() != wxID_OK) return;

    m_gVar1 = RS.m_Var1;
    m_gVar2 = RS.m_Var2;
    m_VarDefault = (bool) RS.m_CheckDefault;

    canvas->m_nSaveRateValue = 1;
    canvas->IDChoropleth = IDM_SMOOTH_RAWRATE;
    canvas->IDSmoothType = IDM_SMOOTH_RAWRATE;
    canvas->IDMapType = canvas->ThemeID[RS.m_theme];

    canvas->m_fieldName = "Raw Rate " + m_gVar1 + " over " + m_gVar2;
    canvas->ChoroplethMapUpdate(RS.m_theme);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnExcessrisk(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnExcessrisk");

	RateSmootherDlg RS(project->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 5);
	if (RS.ShowModal() != wxID_OK) return;
	
    m_gVar1 = RS.m_Var1;
    m_gVar2 = RS.m_Var2;
    m_VarDefault = (bool) RS.m_CheckDefault;

    canvas->m_nSaveRateValue = 2;
    canvas->IDChoropleth = IDM_SMOOTH_EXCESSRISK;
    canvas->IDSmoothType = IDM_SMOOTH_EXCESSRISK;
    canvas->IDMapType = ID_MAP_RESET;
    LOG(RS.m_theme);
    canvas->m_fieldName = m_gVar1 + " over " + m_gVar2;
    canvas->ChoroplethMapUpdate(RS.m_theme);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnBayes(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnBayes");

	RateSmootherDlg RS(project->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 2);
	if (RS.ShowModal() != wxID_OK) return;

    m_gVar1 = RS.m_Var1;
    m_gVar2 = RS.m_Var2;
    m_VarDefault = (bool) RS.m_CheckDefault;

    canvas->m_nSaveRateValue = 3;
    canvas->IDChoropleth = IDM_EMPERICAL_BAYES_SMOOTHER;
    canvas->IDSmoothType = IDM_EMPERICAL_BAYES_SMOOTHER;
    canvas->IDMapType = canvas->ThemeID[RS.m_theme];
    canvas->m_fieldName = "EBS-Smoothed " + m_gVar1 + " over " + m_gVar2;
    canvas->ChoroplethMapUpdate(RS.m_theme);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnSmoother(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnSmoother");

	GalWeight* gal = MyFrame::theFrame->GetGal();
    if (!gal) return;

	RateSmootherDlg RS(project->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 1, gal->gal);
	if (RS.ShowModal() != wxID_OK) return;

    m_gVar1 = RS.m_Var1;
    m_gVar2 = RS.m_Var2;

    m_VarDefault = (bool) RS.m_CheckDefault;

    canvas->m_fieldName = "SRS-Smoothed " + m_gVar1 + " over " + m_gVar2;

    canvas->m_nSaveRateValue = 4;
    canvas->IDChoropleth = IDM_SPATIAL_RATE_SMOOTHER;
    canvas->IDSmoothType = IDM_SPATIAL_RATE_SMOOTHER;
    canvas->IDMapType = canvas->ThemeID[RS.m_theme];
    canvas->ChoroplethMapUpdate(RS.m_theme);
	UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnEmpiricalBayes(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnEmpiricalBayes");

	GalWeight* gal = MyFrame::theFrame->GetGal();
    if (!gal) return;

	RateSmootherDlg RS(project->GetGridBase(),
					   m_gVar1, m_gVar2, m_gX,
					   m_VarDefault, 3, gal->gal);
	if (RS.ShowModal() != wxID_OK) return;
	
    m_gVar1 = RS.m_Var1;
    m_gVar2 = RS.m_Var2;
    m_VarDefault = (bool) RS.m_CheckDefault;

    canvas->m_nSaveRateValue = 5;
    canvas->IDChoropleth = IDM_SPATIAL_RATE_SMOOTHER;
    canvas->IDSmoothType = IDM_SPATIAL_EMPIRICAL_BAYES;
    canvas->IDMapType = canvas->ThemeID[RS.m_theme];

    canvas->m_fieldName = "SEBS-Smoothed " + m_gVar1 + " over " + m_gVar2;
    canvas->ChoroplethMapUpdate(RS.m_theme);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnSaveResults(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnSaveResults");

    canvas->OnSaveRates(GetTitle());
}

void MapFrame::OnReset(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnReset");

    canvas->m_nSaveRateValue = 0;
    canvas->IDChoropleth = ID_MAP_RESET;
    canvas->IDSmoothType = ID_MAP_RESET;
    canvas->IDMapType = ID_MAP_RESET;
    canvas->ChoroplethMapUpdate(canvas->IDChoropleth);
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectableOutlineVisible(wxCommandEvent& WXUNUSED(event)) {
	LOG_MSG("In MapFrame::OnMapSelectableOutlineVisible");
	canvas->SetSelectableOutlineVisible(!canvas->IsSelectableOutlineVisible());
	UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectionMode(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapSelectionMode");
    canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
    canvas->work_mode = MODE_SELECT;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectWithRect(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapSelectWithRect");
    canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
    canvas->work_mode = MODE_SELECT;
    canvas->sel_mode = SELECTION_RECT;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectWithLine(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapSelectWithLine");
    canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
    canvas->work_mode = MODE_SELECT;
    canvas->sel_mode = SELECTION_LINE;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectWithCircle(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapSelectWithCircle");
    canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
    canvas->work_mode = MODE_SELECT;
    canvas->sel_mode = SELECTION_CIRCLE;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapZoomIn(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnMapZoomIn");
    canvas->SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
    canvas->work_mode = MODE_ZOOMIN;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapZoomOut(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnMapZoomOut");
    canvas->SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
    canvas->work_mode = MODE_ZOOMOUT;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapFitToWindowMode(wxCommandEvent& event) {
    LOG_MSG("In MapFrame::OnMapFitToWindowMode");
    canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
    wxSize size = canvas->GetClientSize();
    canvas->CheckSize();
    canvas->Refresh();
    canvas->work_mode = MODE_SELECT;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapPanMode(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapPanMode");
    canvas->SetCursor(wxCursor(wxCURSOR_HAND));
    canvas->work_mode = MODE_PANNING;
    UpdateMenuBarCheckMarks(MyFrame::theFrame->GetMenuBar());
}

void MapFrame::OnMapSelectableFillColor(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapSelectableFillColor");

	wxColour new_color;
	if (GetColorFromUser(this, canvas->selectable_fill_color, new_color)) {
		canvas->selectable_fill_color = new_color;
		if (numBreaks == 1) canvas->colors.at(0) = new_color;
        canvas->Refresh();
	}
}

void MapFrame::OnMapHighlightColor(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnMapHighlightColor");

	wxColour new_color;
	if (GetColorFromUser(this, canvas->highlight_color, new_color)) {
		canvas->highlight_color = new_color;
        canvas->Refresh();
	}
}

void MapFrame::OnAddMeanCenters(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnAddMeanCenters");
	wxString fn = project->GetMainDir() + project->GetMainName() + ".shp";

	long nPoints;
	myBox* B = new myBox;
	vector<double> x(gObservation);
	vector<double> y(gObservation);
	if (!ComputeXY(fn, &nPoints, x, y, B, true)) return;
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].label = "X-Coordinates";
	data[0].field_default = "XMCTR";
	data[0].type = GeoDaConst::double_type;

	data[1].d_val = &y;
	data[1].label = "Y-Coordinates";
	data[1].field_default = "YMCTR";
	data[1].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data,
					   "Add Mean Centers to Table",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
	if (B) delete B; B = 0;
}

void MapFrame::OnAddCentroids(wxCommandEvent& WXUNUSED(event)) {
    LOG_MSG("In MapFrame::OnAddCentroids");
	wxString fn = project->GetMainDir() + project->GetMainName() + ".shp";

	long nPoints;
	myBox* B = new myBox;
	vector<double> x(gObservation);
	vector<double> y(gObservation);
	if (!ComputeXY(fn, &nPoints, x, y, B, false)) return;
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].label = "X-Coordinates";
	data[0].field_default = "XCTRD";
	data[0].type = GeoDaConst::double_type;
	
	data[1].d_val = &y;
	data[1].label = "Y-Coordinates";
	data[1].field_default = "YCTRD";
	data[1].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data,
					   "Add Centroids to Table",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
	if (B) delete B; B = 0;
}

BEGIN_EVENT_TABLE(MapMovieCanvas, wxScrolledWindow)
	EVT_SIZE(MapMovieCanvas::OnSize)
	EVT_PAINT(MapMovieCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(MapMovieCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(MapMovieCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

MapMovieCanvas::MapMovieCanvas(wxWindow *parent, const wxPoint& pos,
							   const wxSize& size,
							   bool conditional_view)
: TemplateCanvas(parent, pos, size), Conditionable(conditional_view)
{
	SetBackgroundColour(*wxWHITE);  // default color
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	selectable_fill_color = GeoDaConst::map_movie_default_fill_colour;
	highlight_color = GeoDaConst::map_movie_default_highlight_colour;
	
    isThereMap = false;
    obs = 0;
    idx.clear();
    xs.clear();
    ys.clear();
    location.clear();
    RawData = NULL;
    hidx = NULL;
    ppc = NULL;
	
}

MapMovieCanvas::~MapMovieCanvas() {
    Remove();
}

void MapMovieCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event) {
    // We must release mouse capture when we receive this event
    if (HasCapture()) ReleaseMouse();
}

void MapMovieCanvas::OnMouseEvent(wxMouseEvent& event) {
	//if (event.RightUp()) {
    //    wxMenu* optMenu =
	//		wxXmlResource::Get()->LoadMenu("ID_MAP_MOVIE_VIEW_MENU_OPTIONS");
	//	((MapMovieFrame* template_frame)->UpdateMenuCheckMarks(optMenu);
    //    PopupMenu(optMenu, event.GetPosition());
    //    return;
    //}
}

void MapMovieCanvas::TimerCall() {
	LOG_MSG("Entering MapMovieCanvas::TimerCall");
    if ((type == 1) &&
        (current_frame < gObservation - 1) &&
        (current_frame >= 0))
        InvalidatePolygon(current_frame, false);
    current_frame++;

    if (current_frame >= gObservation) {
        current_frame = gObservation - 1;

        wxCommandEvent event;
        myP->control->OnCButton2Click(event);
    } else {
        InvalidatePolygon(current_frame, true);
    }
	LOG(current_frame);
	LOG_MSG("Exiting MapMovieCanvas::TimerCall");

}

void MapMovieCanvas::OnPaint(wxPaintEvent& event)
{	
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);
	dc.Clear();

    if (!isThereMap) return;

    wxColour color = selectable_fill_color;
    wxBrush brush(selectable_fill_color);
	
	bool hide_outlines = !IsSelectableOutlineVisible();
	
	dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(brush);
	
    if (!isConditional) {
        if (m_type == ShapeFileTypes::POLYGON) {
            for (int rec = 0; rec < obs; rec++) {
                int cid = hidx[rec];
                if (((type == 2) && (rec <= current_frame)) ||
					((type == 1) && (rec == current_frame))) {
                    brush.SetColour(highlight_color);
				} else {
                    brush.SetColour(selectable_fill_color);
				}
				dc.SetBrush(brush);
				wxPen pen(hide_outlines ? brush.GetColour() : *wxBLACK_PEN);
				dc.SetPen(pen);
                for (int k = 0; k < n_parts_per_cell[cid]; k++) {
                    dc.DrawPolygon(ppc[cid][k + 1] - ppc[cid][k],
								   &(location[idx[cid] + ppc[cid][k]]));
                }
            }
        } else if (m_type == ShapeFileTypes::SPOINT) {
            for (int rec = 0; rec < obs; rec++) {
                int cid = hidx[rec];

                if (((type == 2) && (rec <= current_frame))
					|| ((type == 1) && (rec == current_frame)))
                    GenUtils::DrawSmallCirc(&dc, location.at(cid).x,
											location.at(cid).y,
											StarSize, highlight_color);
                else
                    GenUtils::DrawSmallCirc(&dc, location.at(cid).x,
											location.at(cid).y,
											StarSize, selectable_fill_color);
            }
        }
    } else {
        if (m_type == ShapeFileTypes::POLYGON) {
            double len = RawData[hidx[gObservation - 1]] - RawData[hidx[0]];
            for (int rec = 0; rec < gObservation; rec++) {
                if (conditionFlag[rec]) {
                    float col[3];

                    convert_rgb(((int) (220.0 - 220.0 *
										(RawData[rec] - RawData[hidx[0]])/len))
								% 360, (float) 0.7, (float) 0.7, col);
                    wxColour color((int) (col[0]*255),
								   (int) (col[1]*255), (int) (col[2]*255));

                    wxBrush brush;
                    brush.SetColour(color);
                    dc.SetBrush(brush);
					wxPen pen(hide_outlines ? brush.GetColour() : *wxBLACK_PEN);
					dc.SetPen(pen);

                    for (int k = 0; k < n_parts_per_cell.at(rec); k++) {
                        dc.DrawPolygon(ppc[rec][k + 1] - ppc[rec][k],
									   &(location.at(idx.at(rec)
													 + ppc[rec][k])));
                    }
                }
			}
        } else if (m_type == ShapeFileTypes::SPOINT) {
            double len = RawData[hidx[gObservation - 1]] - RawData[hidx[0]];
            for (int rec = 0; rec < gObservation; rec++) {
                if (conditionFlag[rec]) {
                    float col[3];

                    convert_rgb(((int) (220.0 - 220.0 *
										(RawData[rec] - RawData[hidx[0]])/len))
								% 360, (float) 0.7, (float) 0.7, col);
                    wxColour color((int) (col[0]*255), (int) (col[1]*255),
								   (int) (col[2]*255));

                    wxBrush brush;
                    brush.SetColour(color);
                    dc.SetBrush(brush);

                    GenUtils::DrawSmallCirc(&dc, location.at(rec).x,
											location.at(rec).y, StarSize,color);
                }
			}
        }
	
		wxColor color(wxColour(230, 230, 230)); 
		wxBrush brush;
		brush.SetColour(wxColour(230, 230, 230));
		dc.SetBrush(brush);
		wxPen pen(hide_outlines ? brush.GetColour() : *wxBLACK_PEN);
		dc.SetPen(pen);
		int rec = 0;
		for (rec = 0; rec < gObservation; rec++) {
			if (!conditionFlag[rec]) {
				
				if (m_type == ShapeFileTypes::POLYGON) {
					for (int k = 0; k < n_parts_per_cell.at(rec); k++) {
						dc.DrawPolygon(ppc[rec][k + 1] - ppc[rec][k],
										 &(location.at(idx.at(rec)
													   + ppc[rec][k])));
					}
				} else if (m_type == ShapeFileTypes::SPOINT) {
					GenUtils::DrawSmallCirc(&dc, location.at(rec).x,
											location.at(rec).y, StarSize,
											color);
				}
			}
		}
		
		color = GeoDaConst::map_default_highlight_colour;
		wxBrush brush2;
		if (m_type == ShapeFileTypes::POLYGON) {
			brush2.SetColour(color);
			brush2.SetStyle(wxCROSSDIAG_HATCH);
			dc.SetBrush(brush2);
			wxPen pen(hide_outlines ? brush2.GetColour() : *wxBLACK_PEN);
			dc.SetPen(pen);
		}
		
		for (rec = 0; rec < gObservation; rec++) {
			if ((conditionFlag[rec]) && (gSelection.selected(rec))) {
				if (m_type == ShapeFileTypes::POLYGON) {
					
					for (int k = 0; k < n_parts_per_cell.at(rec); k++) {
						dc.DrawPolygon(ppc[rec][k + 1] - ppc[rec][k],
										 &(location.at(idx.at(rec)
													   + ppc[rec][k])));
					}
					
				} else if (m_type == ShapeFileTypes::SPOINT) {
					GenUtils::DrawSmallCirc(&dc,
											location.at(rec).x,
											location.at(rec).y, StarSize,
											color);
				}
			}
		}
		// Display number of observations in conditional plot cell
		int w, h;
		GetClientSize(&w, &h);
		dc.SetFont(*GeoDaConst::small_font);
		dc.DrawText(wxString::Format("(%d)", numCondObs),
					w-30, 4);
    }
}

void MapMovieCanvas::InvalidatePolygon(int id, bool s) {
    wxClientDC dc(this);
    PrepareDC(dc);
	
	bool hide_outlines = !IsSelectableOutlineVisible();
    wxColour color(s ? highlight_color : selectable_fill_color);
    dc.SetBrush(wxBrush(color));
	
    if ((current_frame >= 0) && (current_frame < gObservation)) {
        int cid = hidx[id];
        if (m_type == ShapeFileTypes::POLYGON) {
			wxPen pen(*wxBLACK_PEN);
			if (hide_outlines) {
				pen.SetColour(color);
			}
			dc.SetPen(pen);
            for (int k = 0; k < n_parts_per_cell.at(cid); k++) {
                dc.DrawPolygon(ppc[cid][k + 1] - ppc[cid][k],
							   &(location.at(idx.at(cid) + ppc[cid][k])));
            }
        } else if (m_type == ShapeFileTypes::SPOINT) {
            GenUtils::DrawSmallCirc(&dc, location.at(cid).x,
									location.at(cid).y, StarSize, color);
        }
    }
	
    bool isSelected = gSelection.selected(hidx[id]);
    if (s) {
        gEvent = ADD_SELECTION;
        gSelection.Push(hidx[id]);
        gSelection.Update();
        MyFrame::theFrame->UpdateWholeView(NULL);
        gSelection.Reset(true);
    } else if (isSelected) {
        gEvent = DEL_SELECTION;
        gSelection.Push(hidx[id]);
        gSelection.Update();
        MyFrame::theFrame->UpdateWholeView(NULL);
        gSelection.Reset(true);
    }
	
}

void MapMovieCanvas::AddMap(wxString& path) {
	LOG_MSG("Entering MapMovieCanvas::AddMap");
    if (isThereMap)
        Remove();

    iShapeFile shx(path, "shx");
	LOG(GenUtils::swapExtension(path,"shx"));
    char hsx[ 2 * GeoDaConst::ShpHeaderSize ];
    shx.read((char *) & hsx[0], 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hdx(hsx);
    long offset, contents, *OffsetIx;
    long n = (hdx.Length() - GeoDaConst::ShpHeaderSize) / 4;
    OffsetIx = new long [ n ];


    if (n < 1 || OffsetIx == NULL)
        return;

    for (long rec = 0; rec < n; ++rec) {
        offset = ReadBig(shx);
        contents = ReadBig(shx);
        offset *= 2;
        OffsetIx[rec] = offset;
    };

    shx.close();

    iShapeFile shp1(path, "shp");
    char hs1[ 2 * GeoDaConst::ShpHeaderSize ];
    shp1.read(hs1, 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hd1(hs1);

    idx.resize(n);
    n_total = 0;

    n_parts_per_cell.resize(n);
    ppc = new pInt[n];

    gObservation = n;
    obs = gObservation;

    if (ShapeFileTypes::ShapeType(hd1.FileShape()) == ShapeFileTypes::POLYGON) {
        for (long rec = 0; rec < n; ++rec) {
            shp1.seekg(OffsetIx[rec] + 12, ios::beg);
            shp1.seekg(OffsetIx[rec] + 12, ios::beg);
            BoundaryShape t;
            t.ReadShape(shp1);

            n_parts_per_cell.at(rec) = t.GetNumParts();
            ppc[rec] = new int[n_parts_per_cell.at(rec) + 1];

            long k = 0, *part = t.GetParts();
            for (k = 0; k < n_parts_per_cell.at(rec); k++) {
                ppc[rec][k] = part[k];
            }

            int n_po = t.GetNumPoints() - 1;

            ppc[rec][k] = n_po;
            n_total += n_po;
        }
    } else if (ShapeFileTypes::ShapeType(hd1.FileShape()) ==
			   ShapeFileTypes::SPOINT) {
        n_total = n;
    }


    shp1.close();

    xs.resize(n_total);
    ys.resize(n_total);

    location.resize(n_total);

    iShapeFile shp(path, "shp");
    char hs[ 2 * GeoDaConst::ShpHeaderSize ];
    shp.read(hs, 2 * GeoDaConst::ShpHeaderSize);
    ShapeFileHdr hd(hs);

    Box pBox;
    xMin = 1e200;
    yMin = 1e200;
    xMax = -1e200;
    yMax = -1e200;

    long counter = 0;
    if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::POLYGON) {
        m_type = ShapeFileTypes::POLYGON;
        for (long rec = 0; rec < n; ++rec) {
            shp.seekg(OffsetIx[rec] + 12, ios::beg);
#ifdef WORDS_BIGENDIAN
            char r[32], p;
            double m1, m2, n1, n2;
            shp.read((char *) r, sizeof (double) * 4);
            SWAP(r[0], r[7], p);
            SWAP(r[1], r[6], p);
            SWAP(r[2], r[5], p);
            SWAP(r[3], r[4], p);
            memcpy(&m1, &r[0], sizeof (double));
            SWAP(r[8], r[15], p);
            SWAP(r[9], r[14], p);
            SWAP(r[10], r[13], p);
            SWAP(r[11], r[12], p);
            memcpy(&m2, &r[8], sizeof (double));
            SWAP(r[16], r[23], p);
            SWAP(r[17], r[22], p);
            SWAP(r[18], r[21], p);
            SWAP(r[19], r[20], p);
            memcpy(&n1, &r[16], sizeof (double));
            SWAP(r[24], r[31], p);
            SWAP(r[25], r[30], p);
            SWAP(r[26], r[29], p);
            SWAP(r[27], r[28], p);
            memcpy(&n2, &r[24], sizeof (double));
            BasePoint p1 = BasePoint(m1, m2);
            BasePoint p2 = BasePoint(n1, n2);
            pBox = Box(p1, p2);
#else
            shp >> pBox;
#endif
            shp.seekg(OffsetIx[rec] + 12, ios::beg);
            BoundaryShape t;
            t.ReadShape(shp);
            int n_po = t.GetNumPoints() - 1;
            if (xMin > pBox._min().x)
                xMin = pBox._min().x;
            if (yMin > pBox._min().y)
                yMin = pBox._min().y;
            if (xMax < pBox._max().x)
                xMax = pBox._max().x;
            if (yMax < pBox._max().y)
                yMax = pBox._max().y;
            BasePoint *Points;
            Points = t.GetPoints();
            idx.at(rec) = counter;
            for (int p = 0; p < n_po; p++) {
                xs.at(counter) = Points[p].x;
                ys.at(counter) = Points[p].y;
                counter++;
            }
        }
        shp.close();
    } else if (ShapeFileTypes::ShapeType(hd.FileShape()) ==
			   ShapeFileTypes::SPOINT) {
        shp.close();
        m_type = ShapeFileTypes::SPOINT;
        myBox bbox;
        ComputeXY(path, &n_total, xs, ys, &bbox, true);
        xMin = bbox.p1.x;
        yMin = bbox.p1.y;
        xMax = bbox.p2.x;
        yMax = bbox.p2.y;

    } else {
        shp.close();
    }

    hidx = new int[gObservation];
    RawData = new double[gObservation];

    int i = 0;
    while (i < gObservation) {
        hidx[i] = i;
        RawData[i] = m_gX[i];
        i++;
    }
    IndexSortD(RawData, hidx, 0, gObservation - 1);
    isThereMap = true;
    current_frame = -1;
    CheckSize();
    Refresh();
	LOG_MSG("Exiting MapMovieCanvas::AddMap");
}

void MapMovieCanvas::Remove() {
    if (!isThereMap) return;

    obs = 0;

    idx.clear();
    xs.clear();
    ys.clear();
    location.clear();
    if (RawData) delete [] RawData;
    RawData = NULL;
    if (hidx) delete [] hidx;
    hidx = NULL;
    if (ppc) delete [] ppc;
    ppc = NULL;
    n_parts_per_cell.clear();
    myP = NULL;

    isThereMap = false;

    Refresh();
}

void MapMovieCanvas::CheckSize() {
    if (!isThereMap) return;
	int Left, Right, Top, Bottom, Width, Height;
    wxSize size2 = GetClientSize();
    int w = size2.x;
    int h = size2.y;

    Left = 7;
    Right = 7;
    Top = 7;
    Bottom = 7;

	Width = w - (Left + Right);
    Height = h - (Top + Bottom);
	
    double xDensity = 0.0;
    double yDensity = 0.0;
    double dx = xMax - xMin;
    double dy = yMax - yMin;

	xDensity = (dx != 0) ? Width / dx : 1;
    yDensity = (dy != 0) ? Height / dy : 1;

    int offset_x;
    int offset_y;
    if (xDensity >= yDensity) {
        xDensity = yDensity;
        offset_x = (w - (int) (dx * xDensity)) / 2;
        offset_y = Top;
    } else {
        yDensity = xDensity;
        offset_x = Left;
        offset_y = (h - (int) (dy * yDensity)) / 2;
    }

    for (int cnt = 0; cnt < n_total; cnt++) {
        location.at(cnt).x = (int) ((xs.at(cnt) - xMin) * xDensity + offset_x);
        location.at(cnt).y = (int) (h - ((ys.at(cnt) - yMin) * yDensity
									+ offset_y));
    }

    StarSize = (int) (log10((double) Width + (double) Height) -
					  log10((double) obs) +
					  ((double) Width + (double) Height) / 256);
    if (StarSize < 0)
		StarSize = 0;
    else if (StarSize > 4)
        StarSize = 4;
}

void MapMovieCanvas::OnSize(wxSizeEvent& event) {
	LOG_MSG("Entering MapMovieCanvas::OnSize");
    CheckSize();
    Refresh();
    event.Skip();
	LOG_MSG("Exiting MapMovieCanvas::OnSize");
}

void MapMovieCanvas::UpdateCondition(int* flags) {
    Conditionable::UpdateCondition(flags);
    Refresh(true);
}

void MapMovieCanvas::Selection(wxDC* dc)
{
    if (!isConditional) return;

    int cnt;
    wxBrush brush;
    wxColour color;
    switch (gEvent) {
        case NEW_SELECTION:
            OnDraw(*dc);
            break;

        case ADD_SELECTION:

            color = highlight_color;
            brush.SetColour(color);
            dc->SetBrush(brush);

            cnt = gSelection.Pop();
            while (cnt != GeoDaConst::EMPTY) {
                if (conditionFlag[cnt]) {
                    if (m_type == ShapeFileTypes::POLYGON) {
                        for (int k = 0; k < n_parts_per_cell.at(cnt); k++) {
                            dc->DrawPolygon(ppc[cnt][k + 1] - ppc[cnt][k],
											&(location.at(idx.at(cnt) +
														  ppc[cnt][k])));
                        }

                    } else if (m_type == ShapeFileTypes::SPOINT) {
                        GenUtils::DrawSmallCirc(dc, location.at(cnt).x,
												location.at(cnt).y, StarSize,
												color);
                    }


                }
                cnt = gSelection.Pop();
            };

            break;

        case DEL_SELECTION:

            color = selectable_fill_color;
            brush.SetColour(color);
            dc->SetBrush(brush);

            while ((cnt = gSelection.Pop()) != GeoDaConst::EMPTY) {
                if (conditionFlag[cnt]) {
                    if (m_type == ShapeFileTypes::POLYGON) {

                        for (int k = 0; k < n_parts_per_cell.at(cnt); k++) {
                            dc->DrawPolygon(ppc[cnt][k + 1] - ppc[cnt][k],
											&(location.at(idx.at(cnt) +
														  ppc[cnt][k])));
                        }

                    } else if (m_type == ShapeFileTypes::SPOINT) {
                        GenUtils::DrawSmallCirc(dc, location.at(cnt).x,
												location.at(cnt).y, StarSize,
												color);
                    }

                }
            }

            break;

        default:
            break;
    }

    gSelection.Reset();

}

BEGIN_EVENT_TABLE(MapMovieFrame, wxFrame)
EVT_ACTIVATE(MapMovieFrame::OnActivate)
EVT_CLOSE(MapMovieFrame::OnClose)
EVT_MENU(XRCID("wxID_CLOSE"), MapMovieFrame::OnMenuClose)
END_EVENT_TABLE()


MapMovieFrame::MapMovieFrame(wxFrame *parent, Project* project, int type,
							 const wxString& title, const wxPoint& pos,
							 const wxSize& size, const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
    old_style = true;
	my_children.Append(this);

    m_splitter = new wxSplitterWindow(this);
    m_splitterMap = new wxSplitterWindow(m_splitter);

    canvas = new MapMovieCanvas(m_splitterMap, wxDefaultPosition);
    template_canvas = canvas;
    template_canvas->template_frame = this;
    canvas->myP = this;
    canvas->type = type;
    control = new CMovieControlPan(m_splitter);
    control->myB = canvas;

    m_splitter->SplitHorizontally(control, m_splitterMap, 70);

    canvas->AddMap(gCompleteFileName);
	m_splitterMap->Initialize(canvas);

    timer = new MapMovieTimer();
    timer->myP = this;

    Show(true);
}

MapMovieFrame::~MapMovieFrame()
{
    timer->Stop();
	my_children.DeleteObject(this);
}

void MapMovieTimer::Notify() {
    myP->canvas->TimerCall();
}

void MapMovieFrame::OnActivate(wxActivateEvent& event) {
    LOG_MSG("In MapMovieFrame::OnActivate");
    if (event.GetActive()) {
		RegisterAsActive("MapMovieFrame");
	}
}

void MapMovieFrame::OnClose(wxCloseEvent& event) {
    LOG_MSG("In MapMovieFrame::OnClose");
    DeregisterAsActive();
    Destroy();
}

void MapMovieFrame::OnMenuClose(wxCommandEvent& event) {
    LOG_MSG("In MapMovieFrame::OnMenuClose");
    Close();
}

void MapMovieFrame::MapMenus() {
    LOG_MSG("In MapMovieFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   true);
    // Map Options Menus
    wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_MOVIE_VIEW_MENU_OPTIONS");
    GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateMenuCheckMarks(optMenu);
}

void MapMovieFrame::UpdateMenuCheckMarks(wxMenu* menu) {
    LOG_MSG("Entering MapMovieFrame::UpdateMenuCheckMarks");
		
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
								  canvas->IsSelectableOutlineVisible() );
	
    LOG_MSG("Exiting MapMovieFrame::UpdateMenuCheckMarks");
}

