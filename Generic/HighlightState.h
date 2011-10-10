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

#ifndef __GEODA_CENTER_HIGHLIGHT_STATE_H__
#define __GEODA_CENTER_HIGHLIGHT_STATE_H__

#include <vector>
#include <list>

class HighlightStateObserver;

/**
 An instance of this class models the linked highlight state of all SHP file
 observations in the currently opened SHP file.  This is the means by which
 all of the views in GeoDa are linked.  All children of TemplateCanvas can
 be Observers of the HightlightState Observable class.  To be notified of
 state changes, an Observable registers itself by calling the
 registerObserver(Observer*) method.  The notifyObservers() method notifies
 all registered Observers of state changes.
 
 This class will eventually replace the Selection class found in shp2cnt.h .
 For a time, these classes will communicate with each other to stay in
 sync until all TemplateCanvas children views migrate to the new mouse
 selection / highlight paradigm.
 
 Notes on Selection: There is a single instance of the Selection class
 called gSelection and defined in OpenGeoDa.cpp.  gSelection is initialized
 by a call to Selection::Init() in MyFrame::OnProjectOpen().
 
 Temporary design: Until we completely transition the the new HighlightState
 paradigm, we need to make the HighlightSate and Selection classes work
 together.  We will make a hidden TemplateFrame that has a TemplateCanvas
 child.  TemplateCanvas is an observer of HighlightState and therefore
 it can specialize the update() virutal method to be notified
 of changes to HighlightState.  Subclasses of TemplateFrame automatically
 register themsevelves in the #TemplateFrame::my_children list so that
 when MyFrame::UpdateWholeView() is called, the wxWindow::Update() virtual
 method is called which can be specialized by the child of TemplateFrame
 (itself a subclass of wxFrame, wxTopLevelWindow, wxWindow).  When
 the Update method is called, it's assumed that there were changes to
 the Selection class state.  The new hidden class will pass messages
 between HighlightState and Selection, callin their respective notify
 methods as needed.
 */

class HighlightState {
public:
	// Types for use in Table
	enum EventType {
		empty, // an empty event, observers should not be notified
		delta, // check both newly_highlighted and newly_unhighlighted
		highlight_all, // select everything
		unhighlight_all, // unhighlight everything
		invert // flip highlight state for all observations
	};
	
private:
	/** The list of registered HighlightStateObserver objects. */
	std::list<HighlightStateObserver*> observers;
	/** This array of booleans corresponds to the highlight/not-highlighted
	 of each underlying SHP file observation. */
	std::vector<bool> highlight;
	/** total number of highlight[i] booleans set to true */
	int total_highlighted;
	/** When the highlight vector has changed values, this vector records
	 the observations indicies that have changed from false to true. */
	std::vector<int> newly_highlighted;
	/** We do not resize the newly_highlighted vector, rather it is used
	 more like a stack.  #total_newly_highlighted records the number of
	 valid entries on the newly_highlighted 'stack'. */
	int total_newly_highlighted;
	/** When the highlight vector has changed values, this vector records
	 the observations indicies that have changed from true to false. */
	std::vector<int> newly_unhighlighted;
	/** We do not resize the newly_unhighlighted vector, rather it is used
	 more like a stack.  #total_newly_unhighlighted records the number of
	 valid entries on the #newly_unhighlighted 'stack'. */
	int total_newly_unghighlighted;
	EventType event_type;
	void ApplyChanges(); // called by notifyObservers to update highlight vec
	
public:
	HighlightState();
	virtual ~HighlightState();
	void SetSize(int n);
	std::vector<bool>& GetHighlight() { return highlight; }
	std::vector<int>& GetNewlyHighlighted() { return newly_highlighted; }
	/** To add a single obs to the newly_highlighted list, set pos=0, and
	 val to the obs number to highlight. */
	void SetNewlyHighlighted(int pos, int val) { newly_highlighted[pos] = val; }
	std::vector<int>& GetNewlyUnhighlighted() { return newly_unhighlighted; }
	/** To add a single obs to the newly_unhighlighted list, set pos=0, and
	 val to the obs number to unhighlight. */
	void SetNewlyUnhighlighted(int pos, int val) {
		newly_unhighlighted[pos] = val; }
	int GetHighlightSize() { return highlight.size(); }
	int GetTotalNewlyHighlighted() { return total_newly_highlighted; }
	int GetTotalNewlyUnhighlighted() { return total_newly_unghighlighted; }
	void SetTotalNewlyHighlighted(int n) { total_newly_highlighted = n; }
	void SetTotalNewlyUnhighlighted(int n) { total_newly_unghighlighted = n; }
	bool IsHighlighted(int obs) { return highlight[obs]; }
	EventType GetEventType() { return event_type; }
	void SetEventType( EventType e ) { event_type = e; }
	
	void registerObserver(HighlightStateObserver* o);
	void removeObserver(HighlightStateObserver* o);
	void notifyObservers();
	/** This is a special version of the notifyObservers method that is
	 not specified in the Observable interface.  It is only here temporarily
	 while the HiddenFrame class exists.  When the HiddenFrame class is
	 no longer needed, we will likely remove this method. If a non-null
	 Observer pointer is passed in, then the update method for that Observer
	 is not called. */
	void notifyObservers(HighlightStateObserver* caller);
};

#endif
