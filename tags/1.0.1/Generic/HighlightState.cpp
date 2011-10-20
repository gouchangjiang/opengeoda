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

#include <algorithm>
#include <functional>
#include "HighlightStateObserver.h"
#include "../logger.h"
#include "HighlightState.h"

HighlightState::HighlightState()
{
	LOG_MSG("In HighlightState::HighlightState()");
}

HighlightState::~HighlightState()
{	
	LOG_MSG("In HighlightState::~HighlightState()");
}

void HighlightState::SetSize(int n) {
	total_highlighted = 0;
	highlight.resize(n);
	newly_highlighted.resize(n);
	newly_unhighlighted.resize(n);
	std::vector<bool>::iterator it;
	for ( it=highlight.begin(); it != highlight.end(); it++ ) (*it) = false;
}


/* Observable interface definitions */

void HighlightState::registerObserver(HighlightStateObserver* o)
{
	observers.push_front(o);
}

void HighlightState::removeObserver(HighlightStateObserver* o)
{
	LOG_MSG("Entering HighlightState::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0) {
		LOG_MSG("No more observers left, so deleting self");
		delete this;
	}
	LOG_MSG("Exiting HighlightState::removeObserver");
}

void HighlightState::notifyObservers()
{
	ApplyChanges();
	if (event_type == empty) return;
	//LOG_MSG("In HighlightState::notifyObservers");
	//LOG(observers.size());
	// See section 18.4.4.2 of Stroustrup
	std::for_each(observers.begin(), observers.end(),
			 std::bind2nd(std::mem_fun(&HighlightStateObserver::update),this));
}

void HighlightState::notifyObservers(HighlightStateObserver* caller)
{
	ApplyChanges();
	if (event_type == empty) return;
	std::list<HighlightStateObserver*>::iterator it;
	for (it = observers.begin(); it != observers.end(); it++) {
		if (caller == (*it)) {
			LOG_MSG("HighlightState::notifyObservers: skipping caller");
		} else {
			(*it)->update(this);
		}
	}
}

void HighlightState::ApplyChanges()
{
	switch (event_type) {
		case delta:
		{
			for (int i=0; i<total_newly_highlighted; i++) {
				if (!highlight[newly_highlighted[i]]) {
					total_highlighted++;
					highlight[newly_highlighted[i]] = true;
				}
			}
			for (int i=0; i<total_newly_unghighlighted; i++) {
				if (highlight[newly_unhighlighted[i]]) {
					total_highlighted--;
					highlight[newly_unhighlighted[i]] = false;
				}
			}
		}
			break;
		case unhighlight_all:
		{
			if (total_highlighted == 0) {
				//MMM: figure out why this short-cut isn't always working
				//event_type = empty;
			}
			for (int i=0, iend=highlight.size(); i<iend; i++) {
				highlight[i] = false;
			}
			total_highlighted = 0;
		}
			break;
		case invert:
		{
			for (int i=0, iend=highlight.size(); i<iend; i++) {
				highlight[i] = !highlight[i];
			}
			total_highlighted = highlight.size() - total_highlighted;
		}
			break;
		default:
			break;
	}
}