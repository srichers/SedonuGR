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

#ifndef _GRID_1D_SPHERE_H
#define _GRID_1D_SPHERE_H 1

#include "Grid.h"

using std::vector;

//*******************************************
// 1-Dimensional Spherical geometry
//*******************************************
class Grid1DSphere: public Grid
{

protected:
	// store location of the outer edge of the zone.
	int reflect_outer;

	// ds^2 = -alpha dt^2 + X^2 dr^2 + dOmega^2
	ScalarMultiDArray<double,NDIMS> X, vr;

public:

	Grid1DSphere();
	virtual ~Grid1DSphere() {}

	void read_model_file(Lua* lua);
	void read_custom_model(Lua* lua);
	void read_nagakura_model(Lua* lua);
	void read_nagakura2_model(Lua* lua);

	// required functions
	int  zone_index               (const Tuple<double,4>& x                                             ) const;
	double zone_coord_volume       (int z_ind                                   ) const;
	double zone_lab_3volume       (int z_ind                                               ) const;
	double zone_min_length        (int z_ind                                               ) const;
	Tuple<double,NDIMS> zone_coordinates(int z_ind                              ) const;
	Tuple<size_t,NDIMS> zone_directional_indices (int z_ind) const;
	Tuple<double,4> sample_in_zone (int z_ind, ThreadRNG* rangen                ) const;
	Tuple<double,3> interpolate_fluid_velocity(const EinsteinHelper& eh               ) const;
	void symmetry_boundaries      (EinsteinHelper *eh                                            ) const;
	double zone_lorentz_factor    (int z_ind                                               ) const;
	double zone_radius            (int z_ind) const;
	Tuple<hsize_t,NDIMS> dims() const;
	hsize_t dimensionality() const {return 1;};
	double d_boundary(const EinsteinHelper& eh) const;
	double d_randomwalk(const EinsteinHelper& eh) const;
	void write_child_zones(H5::H5File file);
	void read_child_zones(H5::H5File file);

	// GR functions
	Christoffel interpolate_Christoffel(const EinsteinHelper& eh) const; // Gamma^alhpa_mu_nu
	Tuple<double,3> interpolate_shift(const EinsteinHelper& eh) const;
	Tuple<double,6> interpolate_3metric(const EinsteinHelper& eh) const;
	void grid_coordinates(const Tuple<double,4>& xup, double coords[NDIMS]) const;
};


#endif
