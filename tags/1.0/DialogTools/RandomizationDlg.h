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

#ifndef __GEODA_CENTER_RANDOMIZATION_DLG_H__
#define __GEODA_CENTER_RANDOMIZATION_DLG_H__

#include "../ShapeOperations/Randik.h"

class CRandomizationDlg: public wxDialog
{    
    DECLARE_CLASS( CRandomizationDlg )
    DECLARE_EVENT_TABLE()

public:
 	CRandomizationDlg( int type, wxWindow* win,
					  const int NumPermutations, const double Slope,
					  wxWindow* parent, wxWindowID id = -1,
					  const wxString& caption = "Randomization",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& my_size = wxDefaultSize,
					  long style = wxCAPTION|wxSYSTEM_MENU);
	virtual ~CRandomizationDlg();
    void CreateControls();
	void Init();

    void OnPaint( wxPaintEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    static bool ShowToolTips();
    void CheckSize(const int width, const int height);
    void Paint(wxDC *dc);
	void Draw(wxDC* dc);
	void DrawRectangle(wxDC* dc, int left, int top, int right, int bottom,
					   const wxColour color);
	
    void SinglePermute();
	void RunPermutations(); 
	void UpdateStatistics();
	
    int	    Width, Height, Left, Right, Top, Bottom;
    const int Permutations;
    double* MoranI; // vector of Moran's I for every permutation experiment

	const double start, stop;
    double  range;
    int	    bins, minBin, maxBin;
    int	    binX, thresholdBin;
    int*    freq;
	
	const double Moran;
	double  MMean;
	double  MSdev;
	int     totFrequency;
	int		signFrequency;
	double  pseudo_p_val;
	double  expected_val;

	int type;
    MoranScatterPlotCanvas* MoranPtr;
    MoranGCanvas* MoranGPtr;
	
    Randik  rng;
	bool    experiment_run_once;

private:
	CRandomizationDlg() : start(-1), stop(1), Moran(0), Permutations(0) {}
};

#endif
