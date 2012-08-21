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

#include "ShapeFile.h"
#include "../GenUtils.h"

iShapeFile::iShapeFile(const wxString& fname, const wxString& ext)
	: std::ifstream(), record(0)
{
	open(GenUtils::swapExtension(fname, ext).mb_str(),
		std::ios::binary | std::ios::in);
}

iShapeFile::~iShapeFile()  {
	close();
}

long int iShapeFile::Recl(const long& shape)
{
	long RecordNumber, RecordLength, ShapeType;
	long dummy1, dummy2;
	++record;
	
	(*this) >> dummy1 >> dummy2 >> ShapeType;
	
#ifdef WORDS_BIGENDIAN
	RecordNumber = dummy1;
	RecordLength = dummy2;
#else
	RecordNumber = GenUtils::Reverse(dummy1);
	RecordLength = GenUtils::Reverse(dummy2);
#endif
	
	return RecordLength;
}

oShapeFile::oShapeFile(const wxString& fname, const wxString& ext)
	: std::ofstream()  
{
    open(GenUtils::swapExtension(fname, ext).mb_str(),
		std::ios::binary | std::ios::out);
	if (fail()) {
	}
}

oShapeFile::~oShapeFile()  
{
	close();
}


