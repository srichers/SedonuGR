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

#ifndef _THREAD_RNG_H
#define _THREAD_RNG_H

#include <gsl/gsl_rng.h>
#include <vector>

class ThreadRNG
{

protected:

	// OPTIMIZE - to prevent false sharing, make sure all the RNG's are on different cache lines
	// vector of generators
	std::vector<gsl_rng*> generators;

public:

	void   init();
	double uniform();
	double uniform(const double min, const double max);
	int    uniform_discrete(const int    min, const int    max);
};

#endif
