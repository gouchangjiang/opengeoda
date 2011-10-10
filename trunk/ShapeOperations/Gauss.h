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

#ifndef __GEODA_CENTER_GAUSS_H__
#define __GEODA_CENTER_GAUSS_H__

class oGauss  {
  private :
    ofstream	dht, dat;
    int			rows, columns;
    bool		complex;
  public  :
    enum  GaussMetrics {
      kMaxNameLength = 8		// the longest variable name allowed in Gauss
    };
    oGauss(char * nme, const int numRows, const int numColumns, const bool complex= false);
    virtual ~oGauss();
    void write(const double val)  
		{
        dat.write((char *) &val, sizeof(double));
        return;
    };

    void addName(char * nme)  {
       const int	nameLength= strlen(nme);
       if (nameLength < kMaxNameLength)  {
         dht.write(nme, nameLength);
         for (int cnt= nameLength; cnt < kMaxNameLength; ++cnt) dht.put('\0');
       }
	   else  {
         dht.write(nme, kMaxNameLength);		// truncate the name
       };
     return;
	};
};

#endif

