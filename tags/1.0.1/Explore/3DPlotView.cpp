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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces
#include <wx/splitter.h>
#include <wx/dcbuffer.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "Geom3D.h"
#include "../OpenGeoDa.h"
#include "../DialogTools/3DControlPan.h"
#include "../logger.h"

#include "3DPlotView.h"

extern	Selection			gSelection;
extern	GeoDaEventType			gEvent;

extern int unproject_pixel(int *pixel, double *world, double z);

BEGIN_EVENT_TABLE(C3DPlotCanvas, wxGLCanvas)
    EVT_SIZE(C3DPlotCanvas::OnSize)
    EVT_PAINT(C3DPlotCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(C3DPlotCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(C3DPlotCanvas::OnMouse)
END_EVENT_TABLE()

C3DPlotCanvas::C3DPlotCanvas(DbfGridTableBase* grid_base, wxWindow *parent,
							 wxWindowID id,
							 const wxPoint& pos,
							 const wxSize& size,
							 long style,
							 const int x_col_id,
							 const int y_col_id,
							 const int z_col_id)
: wxGLCanvas(parent, id, pos, size, style), ball(0),
n_obs(grid_base->GetNumberRows())
{
	xs = 0.1;
	xp = 0.0;
    ys = 0.1;
	yp = 0.0;
	zs = 0.1;
	zp = 0.0;
	
	m_bLButton = false;
	m_bRButton = false;

	m_brush = false;

	bb_min[0] = -1;
	bb_min[1] = -1;
	bb_min[2] = -1;

	bb_max[0] = 1;
	bb_max[1] = 1;
	bb_max[2] = 1;

	Vec3f b_min(bb_min);
	Vec3f b_max(bb_max);
	Vec3f ctr((bb_max[0]+bb_min[0])/2.0f,
			  (bb_max[1]+bb_min[1])/2.0f,
			  (bb_max[2]+bb_min[2])/2.0f);
	float radius = norm(b_max - ctr);
	ball = new Arcball();
	ball->bounding_sphere(ctr, radius);

	world_x = new double[n_obs];
	world_y = new double[n_obs];
	world_z = new double[n_obs];

	Raw_x = new double[n_obs];
	Raw_y = new double[n_obs];
	Raw_z = new double[n_obs];

	std::vector<double> vec;
	
	grid_base->col_data[x_col_id]->GetVec(vec);
	for (int i=0; i<n_obs; i++) Raw_x[i] = vec[i];

	grid_base->col_data[y_col_id]->GetVec(vec);
	for (int i=0; i<n_obs; i++) Raw_y[i] = vec[i];

	grid_base->col_data[z_col_id]->GetVec(vec);
	for (int i=0; i<n_obs; i++) Raw_z[i] = vec[i];
	
	xMin = 1e200;
	yMin = 1e200;
	zMin = 1e200;

	xMax = -1e200;
	yMax = -1e200;
	zMax = -1e200;

    for(int i=0; i<n_obs; i++)
	{
		if(Raw_x[i] <xMin)
			xMin = Raw_x[i];
		if(Raw_y[i] <yMin)
			yMin = Raw_y[i];
		if(Raw_z[i] <zMin)
			zMin = Raw_z[i];

		if(Raw_x[i] > xMax)
			xMax = Raw_x[i];
		if(Raw_y[i] > yMax)
			yMax = Raw_y[i];
		if(Raw_z[i] > zMax)
			zMax = Raw_z[i];
	}

	NormalizeData();

	m_x = false;
	m_y = false;
	m_z = false;
	m_d = true;
	b_select = false;

	isInit = false;

	isMe = false;
}

C3DPlotCanvas::~C3DPlotCanvas()
{
	delete [] world_x;
	delete [] world_y;
	delete [] world_z;
	
	delete [] Raw_x;
	delete [] Raw_y;
	delete [] Raw_z;
	
	if (ball) delete ball; ball = 0;
}


void C3DPlotCanvas::OnPaint( wxPaintEvent& event )
{
    /* must always be here */
    wxPaintDC dc(this);

#ifndef __WXMOTIF__
    if (!GetContext()) return;
#endif

    SetCurrent();

    /* initialize OpenGL */
    if (isInit == false) 
    {
        InitGL();
        isInit = true;
    }

	begin_redraw();
    RenderScene();
	end_redraw();

	if(bSelect)
	{
		double world11[3],world12[3], world22[3], world21[3];
		int pixel11[2], pixel12[2], pixel22[2], pixel21[2];

		int small_x= (select_start.x<select_end.x)? select_start.x:select_end.x;
		int large_x= (select_start.x>select_end.x)? select_start.x:select_end.x;
		int small_y= (select_start.y>select_end.y)? select_start.y:select_end.y;
		int large_y= (select_start.y<select_end.y)? select_start.y:select_end.y;

		pixel11[0] = small_x;
		pixel12[0] = small_x;
		pixel21[0] = large_x;
		pixel22[0] = large_x;

		pixel11[1] = small_y;
		pixel21[1] = small_y;
		pixel12[1] = large_y;
		pixel22[1] = large_y;

		unproject_pixel(pixel11, world11, 0.0);	
		unproject_pixel(pixel12, world12, 0.0);
		unproject_pixel(pixel22, world22, 0.0);
		unproject_pixel(pixel21, world21, 0.0);

		glLineWidth(2.0);
		glColor3f(0.75,0.75,0.75);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
			glVertex3dv(world11);
			glVertex3dv(world12);
			glVertex3dv(world12);
			glVertex3dv(world22);
			glVertex3dv(world22);
			glVertex3dv(world21);
			glVertex3dv(world21);
			glVertex3dv(world11);

		glEnd();
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);

	}



    /* flush */
    glFlush();

    /* swap */
    SwapBuffers();
}

void C3DPlotCanvas::OnSize(wxSizeEvent& event)
{
    // this is also necessary to update the context on some platforms
    wxGLCanvas::OnSize(event);
    
    // set GL viewport (not called by wxGLCanvas::OnSize on all platforms...)
    int w, h;
    GetClientSize(&w, &h);


#ifndef __WXMOTIF__
    if (GetContext())
#endif
    {
        //SetCurrent();
        glViewport(0, 0, (GLint) w, (GLint) h);
    }
}

void C3DPlotCanvas::OnEraseBackground(wxEraseEvent& event)
{
    /* Do nothing, to avoid flashing on MSW */
}


void C3DPlotCanvas::OnMouse( wxMouseEvent& event )
{
 

	wxPoint pt( event.GetPosition() );

	wxClientDC dc(this);
	PrepareDC(dc);

	wxPoint point(event.GetLogicalPosition(dc));

	if(event.LeftDown())
	{
//		SetCapture();

	/*	if((nFlags & MK_SHIFT) && this->m_d )
		{
			bSelect = true;
			select_start = point;

			m_dlg->UnsetSelect();
			b_select = false;

		}
		else*/ if( (event.CmdDown()) && this->m_d && this->b_select)
		{
			m_brush = true;

			last[0] = point.x;
			last[1] = point.y;
		}
		else
		{
			m_bLButton = true;

			int where[2];
			where[0] = point.x;
			where[1] = point.y;
			last[0] = point.x;
			last[1] = point.y;

			ball->mouse_down(where,1);

		}

	}

	if(event.LeftUp())
	{
//		ReleaseCapture();

		if(bSelect && this->m_d )
		{

			bSelect =  false;
			SelectByRect();
			gEvent = NEW_SELECTION;
			isMe = true;
			MyFrame::theFrame->UpdateWholeView(NULL);
			gSelection.Reset(true);
			isMe = false;


		}
		else if(m_brush)
		{
			m_brush = false;

			gEvent = NEW_SELECTION;
			isMe = true;
			MyFrame::theFrame->UpdateWholeView(NULL);
			gSelection.Reset(true);
			isMe = false;
		}
		else
		{
			m_bLButton = false;

			int where[2];
			where[0] = point.x;
			where[1] = point.y;

			ball->mouse_up(where,1);
		}

	}
	if(event.Dragging())
	{


		int where[2];
		where[0] = point.x;
		where[1] = point.y;

	/*
		if((nFlags & MK_SHIFT) && bSelect && this->m_d )
		{
			select_end = point;
			SelectByRect();
			InvalidateRect(NULL,FALSE);

		}
		else */ if(m_brush)
		{

			float vp[4];
			glGetFloatv(GL_VIEWPORT, vp);
			float W=vp[2], H=vp[3];

			float diam = 2*(ball->radius);

			ball->apply_transform();

			int pix1[2], pix2[2], pix3[2];
			pix1[0] = (int)(W/2);
			pix1[1] = (int)(H/2);
			pix2[0] = (int)(W/2-1);
			pix2[1] = (int)(H/2);
			pix3[0] = (int)(W/2);
			pix3[1] = (int)(H/2-1);
			double world1[3], world2[3],world3[3];
			unproject_pixel(pix1, world1, 0.0);
			unproject_pixel(pix2, world2, 0.0);
			unproject_pixel(pix3, world3, 0.0);

			ball->unapply_transform();


			Vec3f w1(world1);
			Vec3f w2(world2);
			Vec3f w3(world3);

			Vec3f screen_x = w1-w2;
			unitize(screen_x);
			Vec3f screen_y = w3-w1;
			unitize(screen_y);

			Vec3f XX(1,0,0);
			Vec3f YY(0,1,0);
			Vec3f ZZ(0,0,1);

			xp += diam * (where[0] - last[0]) / W *(XX*screen_x);
			yp += diam * (where[0] - last[0]) / W *(YY*screen_x);
			zp += diam * (where[0] - last[0]) / W *(ZZ*screen_x); 

			xp += diam * (last[1] - where[1]) / H *(XX*screen_y);
			yp += diam * (last[1] - where[1]) / H *(YY*screen_y);
			zp += diam * (last[1] - where[1]) / H *(ZZ*screen_y);


			if(xp < -1.0)
				xp = -1.0;
			if(xp > 1.0)
				xp = 1.0;

			if(yp < -1.0)
				yp = -1.0;
			if(yp > 1.0)
				yp = 1.0;


			if(zp < -1.0)
				zp = -1.0;
			if(zp > 1.0)
				zp = 1.0;


			last[0] = where[0];
			last[1] = where[1];



			pa->control->m_xp->SetValue((int)((xp+1)*10000));
			pa->control->m_yp->SetValue((int)((yp+1)*10000));
			pa->control->m_zp->SetValue((int)((zp+1)*10000));


			this->UpdateSelect();

		}
		else
		{
			bSelect = false;
			if(m_bLButton & m_bRButton)
			{
				ball->mouse_drag(where,last,2);

				last[0] = where[0];
				last[1] = where[1];
			}

			else if(m_bLButton)
			{
				ball->mouse_drag(where,last,1);

				last[0] = where[0];
				last[1] = where[1];
			}
			else if(m_bRButton)
			{
				ball->mouse_drag(where,last,3);

				last[0] = where[0];
				last[1] = where[1];
			}
		}
	}

	if(event.RightDown())
	{
//		SetCapture();
	
		m_bRButton = true;

		int where[2];
		where[0] = point.x;
		where[1] = point.y;
		last[0] = point.x;
		last[1] = point.y;

		ball->mouse_down(where,3);

	}
	if(event.RightUp())
	{
//		ReleaseCapture();


		m_bRButton = false;

		int where[2];
		where[0] = point.x;
		where[1] = point.y;

		ball->mouse_up(where,3);		

	}
	Refresh();
}

void C3DPlotCanvas::InitGL(void)
{
    SetCurrent();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);

    float ambient_light[4] = {1.0, 1.0, 1.0, 1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *)ambient_light);

    const float light0_pos[4] = {0.0f, 0.5f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glEnable(GL_LIGHT0);

    float rgb[4] = {0.912f, 0.717f, 0.505f, 1.0f};
    float r_amb[4]; 
    float r_diff[4];
    float r_spec[4];

	for(int i=0; i<4; i++)
	{
		r_amb[i] = rgb[i]*0.1;
		r_diff[i] = rgb[i]*1.0;
		r_spec[i] = rgb[i]*0.3f;
	}

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, r_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, r_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, r_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void C3DPlotCanvas::UpdateViewsDlg()
{
    gEvent=NEW_SELECTION;

	isMe = true;
	MyFrame::theFrame->UpdateWholeView(NULL);
	gSelection.Reset(true);
	isMe = false;
}

void C3DPlotCanvas::UpdateSelect()
{
	if(b_select) {
		double minx = xp - xs;
		double maxx = xp + xs;
		double miny = yp - ys;
		double maxy = yp + ys;
		double minz = zp - zs;
		double maxz = zp + zs;

		for(int i=0; i<n_obs; i++) {
			if( (world_x[i] >= minx) && (world_x[i] <= maxx) &&
			   (world_y[i] >= miny) && (world_y[i] <= maxy) &&
			   (world_z[i] >= minz) && (world_z[i] <= maxz) )
				gSelection.select(i);
			else 
				gSelection.deselect(i);
		}
	} else {
		LOG_MSG("b_select false while in C3DPlotCanvas::UpdateSelect");
	}
}

void C3DPlotCanvas::SelectByRect()
{

	double world11[3],world12[3], world22[3], world21[3];
	double world113[3], world123[3], world223[3], world213[3];

	int pixel11[2], pixel12[2], pixel22[2], pixel21[2];

    int small_x = (select_start.x < select_end.x)? select_start.x:select_end.x;
    int large_x = (select_start.x > select_end.x)? select_start.x:select_end.x;
    int small_y = (select_start.y < select_end.y)? select_start.y:select_end.y;
    int large_y = (select_start.y > select_end.y)? select_start.y:select_end.y;

	pixel11[0] = small_x;	pixel12[0] = small_x; 
	pixel21[0] = large_x;   pixel22[0] = large_x;

	pixel11[1] = small_y;   pixel21[1] = small_y;
	pixel12[1] = large_y;	pixel22[1] = large_y;


	ball->apply_transform();

	unproject_pixel(pixel11, world11, 0.0);	
	unproject_pixel(pixel12, world12, 0.0);
	unproject_pixel(pixel22, world22, 0.0);
    unproject_pixel(pixel21, world21, 0.0);


	unproject_pixel(pixel11, world113, 1.0);	
	unproject_pixel(pixel12, world123, 1.0);
	unproject_pixel(pixel22, world223, 1.0);
    unproject_pixel(pixel21, world213, 1.0);

	ball->unapply_transform();


	SPlane* plane;
	int i;
	
	bool *inside = new bool[n_obs*4];
	for(i=0; i<n_obs*4; i++)
		inside[i] = false;
	double *world1, *world2, *world3, *world4;
	for(int k=0; k<4; k++)
	{
		switch(k)
		{
		case 0: 
			world1 = world11;
			world2 = world12;
			world3 = world113;
			world4 = world123;
			break;
		case 1:
			world1 = world12;
			world2 = world22;
			world3 = world123;
			world4 = world223;
			break;
		case 2:
			world1 = world22;
			world2 = world21;
			world3 = world223;
			world4 = world213;
			break;
		case 3:
			world1 = world21;
			world2 = world11;
			world3 = world213;
			world4 = world113;
			break;
		default:
			break;
		}

		plane = new SPlane(world1, world2, world3);


		Vec3f a1(world1[0], world1[1], world1[2]);
		Vec3f a2(world2[0], world2[1], world2[2]);
		Vec3f a3(world3[0], world3[1], world3[2]);
		Vec3f a4(world4[0], world4[1], world4[2]);
		Vec3f l1 = a3 - a1;
		Vec3f l2 = a4 - a2;

		for(i=0; i<n_obs; i++)
		{
			Vec3f cor(world_x[i], world_y[i], world_z[i]);
			if(plane->isPositive(cor))
				inside[k*n_obs+i] = true;

		}

        delete plane;
    }

	delete [] inside;

	for(i=0; i<n_obs; i++)
		if(inside[i] && inside[n_obs+i] && inside[2*n_obs+i] && inside[3*n_obs+i])
			gSelection.select(i) ;
		else
			gSelection.deselect(i);

}

void C3DPlotCanvas::NormalizeData()
{
	LOG_MSG("Entering C3DPlotCanvas::NormalizeData");
	double xCtr = (xMin+xMax)/2.0;
	double yCtr = (yMin+yMax)/2.0;
	double zCtr = (zMin+zMax)/2.0;

	double xScale, yScale, zScale;

	if(xMax == xMin)
		xScale = 1.0;
	else
		xScale = 2.0/(xMax-xMin);

	if(yMax == yMin)
		yScale = 1.0;
	else	
		yScale = 2.0/(yMax-yMin);

	if(zMax == zMin)
		zScale = 1.0;
	else			
		zScale = 2.0/(zMax-zMin);

	for(int i=0; i<n_obs; i++) {
		world_x[i] = (Raw_x[i]-xCtr)*xScale;
		world_y[i] = (Raw_y[i]-yCtr)*yScale;
		world_z[i] = (Raw_z[i]-zCtr)*zScale;
		//LOG_MSG(wxString::Format("world_xyz[%d] = (%g, %g, %g)", i,
		//						 world_x[i], world_y[i], world_z[i]));
	}
	LOG_MSG("Exiting C3DPlotCanvas::NormalizeData");
}

void C3DPlotCanvas::RenderScene()
{
	GLUquadric* myQuad = 0;
	myQuad = gluNewQuadric();
	if(m_d)
	{
		glColor3f(1.0, 1.0, 1.0);
		for(int i=0; i<n_obs; i++)
		{
			if(gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], world_y[i], world_z[i]);
			gluSphere(myQuad, 0.03, 5, 5); 	
			glPopMatrix();
		}
		glColor3f(1.0, 1.0, 0.0);
		for(int i=0; i<n_obs; i++)
		{
			if(!gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], world_y[i], world_z[i]);
			gluSphere(myQuad, 0.03, 5, 5); 	
			glPopMatrix();
		}
		
	}

	glDisable(GL_LIGHTING);
	if(m_x)
	{

		glColor3f(0.75,0.75,0.75);
		for(int i=0; i<n_obs; i++)
		{
			if(gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(-1, world_y[i], world_z[i]);
			glRotatef(90, 0.0, 1.0, 0.0);	
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		glColor3f(1.0, 1.0, 0.0);
		for(int i=0; i<n_obs; i++)
		{
			if(!gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(-1, world_y[i], world_z[i]);
			glRotatef(90, 0.0, 1.0, 0.0);
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}
	if(m_y)
	{
		glColor3f(0.75,0.75,0.75);
		for(int i=0; i<n_obs; i++)
		{
			if(gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], -1, world_z[i]);
			glRotatef(90, 1.0, 0.0, 0.0); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		glColor3f(1.0, 1.0, 0.0);
		for(int i=0; i<n_obs; i++)
		{
			if(!gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], -1, world_z[i]);
			glRotatef(90, 1.0, 0.0, 0.0); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}

	if(m_z)
	{

		glColor3f(0.75,0.75,0.75);
		for(int i=0; i<n_obs; i++)
		{
			if(gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], world_y[i], -1); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		glColor3f(1.0, 1.0, 0.0);
		for(int i=0; i<n_obs; i++)
		{
			if(!gSelection.selected(i))
				continue;
			glPushMatrix();
			glTranslatef(world_x[i], world_y[i], -1); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}
	glEnable(GL_LIGHTING);

	if(b_select)
	{
		double minx = xp - xs;
		double maxx = xp + xs;
		double miny = yp - ys;
		double maxy = yp + ys;
		double minz = zp - zs;
		double maxz = zp + zs;
		glDisable(GL_LIGHTING);
		glLineWidth(2.0);
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,maxy,minz);
			glVertex3f(minx,maxy,minz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(minx,maxy,minz);			
		glEnd();
		glEnable(GL_LIGHTING);
	}

	glLineWidth(3.0);

	glColor3f((GLfloat) 0.95,(GLfloat) 0.0,(GLfloat) 0.95);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);

		glVertex3f((GLfloat) 1.2,(GLfloat) -0.9,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.1,(GLfloat) -0.8,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.2,(GLfloat) -0.8,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.1,(GLfloat) -0.9,(GLfloat) -1.0);
    glEnd();

	glColor3f((GLfloat) 0.0,(GLfloat) 0.95,(GLfloat) 0.0);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);

		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.9,(GLfloat) 1.2,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.8,(GLfloat) 1.2,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.1,(GLfloat) -1.0);		
	glEnd();

	glColor3f((GLfloat) 0.0,(GLfloat) 0.95,(GLfloat) 0.95);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);

		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.1);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.1);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.1);		
	glEnd();


	glLineWidth(1.0);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
	glEnd();

	glEnable(GL_LIGHTING);
 
	gluDeleteQuadric(myQuad);
}

void C3DPlotCanvas::apply_camera()
{
    int w, h;
    GetClientSize(&w, &h);

    float aspect = (float)w / (float)h;

    Vec3 d_min(bb_min[0], bb_min[1], bb_min[2]);
    Vec3 d_max(bb_max[0], bb_max[1], bb_max[2]);


    Vec3 up(0, 1, 0);
    double fovy = 60.0;

    Vec3 at = (d_max+d_min)/2.0;

	double radius = sqrt((d_max[0]-at[0])*(d_max[0]-at[0])+(d_max[1]-at[1])*(d_max[1]-at[1])+(d_max[2]-at[2])*(d_max[2]-at[2]));
    double d = 3*radius / tan(fovy * M_PI/180.0);


    Vec3 from = at;

    from[2] += d;

    double znear = d/20;
    double zfar = 10*d;


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(fovy, aspect, znear, zfar);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    gluLookAt(from[0], from[1], from[2],
	      at[0], at[1], at[2],
	      up[0], up[1], up[2]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
}

void C3DPlotCanvas::end_redraw()
{

   ball->unapply_transform();
}

void C3DPlotCanvas::begin_redraw()
{
    apply_camera();

    ball->apply_transform();
}


BEGIN_EVENT_TABLE(C3DPlotFrame, wxFrame)
	EVT_ACTIVATE(C3DPlotFrame::OnActivate)
    EVT_CLOSE(C3DPlotFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), C3DPlotFrame::OnMenuClose)
END_EVENT_TABLE()


C3DPlotFrame::C3DPlotFrame(wxFrame *parent, Project* project,
						   const wxString& title,
						   const wxPoint& pos, const wxSize& size,
						   const long style,
						   const int x_col_id,
						   const int y_col_id,
						   const int z_col_id)
	:TemplateFrame(parent, project, title, pos, size, style)
{
	old_style = true;
	my_children.Append(this);
	m_splitter = new wxSplitterWindow(this);
    
	canvas = new C3DPlotCanvas(project->GetGridBase(), m_splitter, -1,
							   wxDefaultPosition,
							   wxDefaultSize, 0,
							   x_col_id, y_col_id, z_col_id);
	
	DbfGridTableBase* g = project->GetGridBase();
	control = new C3DControlPan(m_splitter, -1, wxDefaultPosition,
								wxDefaultSize, wxCAPTION|wxSYSTEM_MENU,
								g->col_data[x_col_id]->name,
								g->col_data[y_col_id]->name,
								g->col_data[z_col_id]->name);	
	
	canvas->pa = this;
	control->pa = this;

	m_splitter->SplitVertically(control, canvas, 70);

	Show(true);
}

C3DPlotFrame::~C3DPlotFrame()
{
	my_children.DeleteObject(this);
}


void C3DPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In C3DPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("C3DPlotFrame", GetTitle());
	}
}

void C3DPlotFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In C3DPlotFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void C3DPlotFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In C3DPlotFrame::OnMenuClose");
	Close();
}

void C3DPlotFrame::MapMenus()
{
	LOG_MSG("In C3DPlotFrame::MapMenus");
}

