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
#include "DialogTools/SaveToTableDlg.h"
#include "DialogTools/VariableSettingsDlg.h"
#include "DialogTools/MovieControlPan.h"
#include "Explore/ConditionalView.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include "GeoDaConst.h"
#include "logger.h"
#include "mapview.h"

extern inline void convert_rgb(float x, float y, float z, float* result);

extern MyFrame *frame;
extern GeoDaEventType gEvent;
extern Selection gSelection;

BEGIN_EVENT_TABLE(MapMovieCanvas, wxScrolledWindow)
	EVT_SIZE(MapMovieCanvas::OnSize)
	EVT_PAINT(MapMovieCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(MapMovieCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(MapMovieCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

MapMovieCanvas::MapMovieCanvas(wxWindow *parent, int num_obs_s,
							   const wxPoint& pos,
							   const wxSize& size,
							   bool conditional_view)
: TemplateCanvas(parent, pos, size), Conditionable(conditional_view, num_obs_s),
num_obs(num_obs_s)
{
	SetBackgroundColour(*wxWHITE);  // default color
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	selectable_fill_color = GeoDaConst::map_movie_default_fill_colour;
	highlight_color = GeoDaConst::map_movie_default_highlight_colour;
	
    isThereMap = false;
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
}

void MapMovieCanvas::TimerCall() {
	LOG_MSG("Entering MapMovieCanvas::TimerCall");
    if ((type == 1) &&
        (current_frame < num_obs - 1) &&
        (current_frame >= 0))
        InvalidatePolygon(current_frame, false);
    current_frame++;

    if (current_frame >= num_obs) {
        current_frame = num_obs - 1;

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
            for (int rec = 0; rec < num_obs; rec++) {
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
            for (int rec = 0; rec < num_obs; rec++) {
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
            double len = RawData[hidx[num_obs - 1]] - RawData[hidx[0]];
            for (int rec = 0; rec < num_obs; rec++) {
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
            double len = RawData[hidx[num_obs - 1]] - RawData[hidx[0]];
            for (int rec = 0; rec < num_obs; rec++) {
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
		for (rec = 0; rec < num_obs; rec++) {
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
		
		for (rec = 0; rec < num_obs; rec++) {
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
	
    if ((current_frame >= 0) && (current_frame < num_obs)) {
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

void MapMovieCanvas::AddMap(const wxString& path,
							double* v1, const wxString& v1_name)
{
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

    hidx = new int[num_obs];
    RawData = new double[num_obs];

    int i = 0;
    while (i < num_obs) {
        hidx[i] = i;
        RawData[i] = v1[i];
        i++;
    }
    IndexSortD(RawData, hidx, 0, num_obs - 1);
    isThereMap = true;
    current_frame = -1;
    CheckSize();
    Refresh();
	LOG_MSG("Exiting MapMovieCanvas::AddMap");
}

void MapMovieCanvas::Remove() {
    if (!isThereMap) return;

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
					  log10((double) num_obs) +
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


MapMovieFrame::MapMovieFrame(wxFrame *parent,
							 double* v1, int num_obs, const wxString& v1_name,
							 Project* project, int type,
							 const wxString& title, const wxPoint& pos,
							 const wxSize& size, const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
    old_style = true;
	my_children.Append(this);

    m_splitter = new wxSplitterWindow(this);
    m_splitterMap = new wxSplitterWindow(m_splitter);

    canvas = new MapMovieCanvas(m_splitterMap, num_obs, wxDefaultPosition);
    template_canvas = canvas;
    template_canvas->template_frame = this;
    canvas->myP = this;
    canvas->type = type;
    control = new MovieControlPan(m_splitter, num_obs);
    control->myB = canvas;

    m_splitter->SplitHorizontally(control, m_splitterMap, 70);

    canvas->AddMap(project->shp_fname.GetFullPath(),
				   v1, v1_name);
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

