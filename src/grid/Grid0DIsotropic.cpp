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

#include "global_options.h"
#include "Grid0DIsotropic.h"
#include "Lua.h"
#include "Transport.h"

using namespace std;
namespace pc = physical_constants;

Grid0DIsotropic::Grid0DIsotropic(){
	PRINT_ASSERT(NDIMS,==,0);
	grid_type = "Grid0DIsotropic";
	tetrad_rotation = cartesian;
	xAxes.resize(0);
}

//------------------------------------------------------------
// initialize the zone geometry from model file
//------------------------------------------------------------
void Grid0DIsotropic::read_model_file(Lua* lua)
{
	// number of zones
	rho.set_axes(xAxes);
	T.set_axes(xAxes);
	Ye.set_axes(xAxes);
	H_vis.set_axes(xAxes);
	lapse.set_axes(xAxes);
	rho[0] = lua->scalar<double>("Grid0DIsotropic_rho");
	T[0]   = lua->scalar<double>("Grid0DIsotropic_T")/pc::k_MeV;
	Ye[0]  = lua->scalar<double>("Grid0DIsotropic_Ye");
	lapse[0] = 1.0;
	H_vis[0] = 0;
	PRINT_ASSERT(rho[0],>=,0);
	PRINT_ASSERT(T[0],>=,0);
	PRINT_ASSERT(Ye[0],>=,0);
	PRINT_ASSERT(Ye[0],<=,1.0);
}

void Grid0DIsotropic::write_child_zones(H5::H5File){
	// nothing to write.
}

//------------------------------------------------------------
// Return the zone index containing the position x
//------------------------------------------------------------
int Grid0DIsotropic::zone_index(const Tuple<double,4>&) const
{
	return 0;
}


//------------------------------------------------------------
// return volume of zone z_ind
//------------------------------------------------------------
double  Grid0DIsotropic::zone_lab_3volume(int z_ind) const
{
	PRINT_ASSERT(z_ind,==,0);
	return 1.0;
}


//------------------------------------------------------------
// return length of zone
//------------------------------------------------------------
double  Grid0DIsotropic::zone_min_length(int z_ind) const
{
	PRINT_ASSERT(z_ind,==,0);
	return INFINITY;
}


// ------------------------------------------------------------
// find the coordinates of the zone in geometrical coordinates
// ------------------------------------------------------------
Tuple<double,NDIMS> Grid0DIsotropic::zone_coordinates(int z_ind) const{
	PRINT_ASSERT(z_ind,==,0);
	return Tuple<double,NDIMS>();
}


//-------------------------------------------
// get directional indices from zone index
//-------------------------------------------
Tuple<unsigned,NDIMS> Grid0DIsotropic::zone_directional_indices(int z_ind) const
{
	PRINT_ASSERT(z_ind,==,0);
	return Tuple<unsigned,NDIMS>();
}


//------------------------------------------------------------
// sample a random position within the spherical shell
//------------------------------------------------------------
Tuple<double,4> Grid0DIsotropic::sample_in_zone(int z_ind, ThreadRNG*) const
{
	PRINT_ASSERT(z_ind,==,0);

	// set the double 3-d coordinates
	Tuple<double,4> x;
	x[0] = 0;
	x[1] = 0;
	x[2] = 0;
	return x;
}


//------------------------------------------------------------
// get the velocity vector 
//------------------------------------------------------------
Tuple<double,3> Grid0DIsotropic::interpolate_fluid_velocity(const EinsteinHelper& eh) const
{
	return Tuple<double,3>(0);
}


//------------------------------------------------------------
// Reflect off symmetry plane
//------------------------------------------------------------

void Grid0DIsotropic::symmetry_boundaries(EinsteinHelper*) const{
	// does nothing - no boundary
}

double Grid0DIsotropic::zone_radius(int) const{
	return 0;
}

//-----------------------------
// Dimensions of the grid
//-----------------------------
Tuple<hsize_t,NDIMS> Grid0DIsotropic::dims() const{
	return Tuple<hsize_t,NDIMS>();
}

double Grid0DIsotropic::zone_lorentz_factor(int) const{
	return 1.0;
}
// returning 0 causes the min distance to take over in propagate.cpp::which_event
double Grid0DIsotropic::d_boundary(const EinsteinHelper&) const{
	return 0;
}
double Grid0DIsotropic::d_randomwalk(const EinsteinHelper&) const{
	return INFINITY;
}

Tuple<double,4> Grid0DIsotropic::dk_dlambda(const EinsteinHelper& eh) const{ // default Minkowski
	return Tuple<double,4>(NaN);
}
Tuple<double,3> Grid0DIsotropic::interpolate_shift(const EinsteinHelper&) const{ // default Minkowski
	return Tuple<double,3>(NaN);
}
Tuple<double,6> Grid0DIsotropic::interpolate_3metric(const EinsteinHelper&) const{ // default Minkowski
	return Tuple<double,6>(NaN);
}

void Grid0DIsotropic::grid_coordinates(const Tuple<double,4>&, double coords[NDIMS]) const{
	coords[0] = 0;
}
