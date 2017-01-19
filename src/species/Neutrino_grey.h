/*
//  Copyright (c) 2015, California Institute of Technology and the Regents
//  of the University of California, based on research sponsored by the
//  United States Department of Energy. All rights reserved.
//
//  This file is part of Sedonu.
//
//  Sedonu is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Neither the name of the California Institute of Technology (Caltech)
//  nor the University of California nor the names of its contributors 
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  Sedonu is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Sedonu.  If not, see <http://www.gnu.org/licenses/>.
//
*/

#ifndef _NEUTRINOS_GREY_H
#define _NEUTRINOS_GREY_H

#include "Neutrino.h"

class Neutrino_grey: public Neutrino
{

protected:

	// grey opacity and absorption fraction
	double grey_opac; //(cm^2/g)
	double grey_abs_frac;       //unitless

public:

	Neutrino_grey();

	void myInit(Lua* lua);
	void set_eas(int zone_index);
	void get_opacity(const double com_nu, const int z_ind, double* abs_opac, double* scat_opac) const;
};

#endif