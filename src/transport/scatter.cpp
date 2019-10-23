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

#include "Species.h"
#include "Transport.h"
#include "Grid.h"
#include "global_options.h"
using namespace std;
namespace pc = physical_constants;


// decide whether to kill a particle
void Transport::window(EinsteinHelper *eh) const{
	PRINT_ASSERT(eh->N,>=,0);
	PRINT_ASSERT(eh->fate,!=,rouletted);

	// Roulette if too low energy
	if(eh->N == 0) eh->fate = rouletted;
	else while(eh->N/eh->N0 < min_packet_weight and eh->fate==moving){
		if(rangen.uniform() < 0.5) eh->fate = rouletted;
		else eh->N *= 2.0;
	}

	if(eh->fate == moving){
		PRINT_ASSERT(eh->N/eh->N0,>=,min_packet_weight);
		PRINT_ASSERT(eh->N,<,INFINITY);
		PRINT_ASSERT(eh->N,>,0);
	}
}

// choose which type of scattering event to do
void Transport::scatter(EinsteinHelper *eh) const{
	PRINT_ASSERT(eh->scatopac,>,0);

	// store the old direction
	double Nold = eh->N;
	Tuple<double,4> kup_tet_old;
	for(size_t i=0; i<4; i++) kup_tet_old[i] = eh->kup_tet[i];
	PRINT_ASSERT(kup_tet_old[3],==,eh->kup_tet[3]);
	
	// sample outgoing energy and set the post-scattered state
	if(use_scattering_kernels) sample_scattering_final_state(eh,kup_tet_old);
	else{
		// sample new direction
		Tuple<double,4> kup_tet;
		isotropic_kup_tet(eh->nu(),kup_tet,&rangen);
		eh->set_kup_tet(kup_tet);
	}

	for(size_t i=0; i<4; i++){
		grid->fourforce_abs[eh->z_ind][i] += (kup_tet_old[i]*Nold - eh->kup_tet[i]*eh->N);
	}
}



//-------------------------------------------------------
// Initialize the CDF that determines particle dwell time
// result is D*t/(R^2)
//-------------------------------------------------------
void Transport::init_randomwalk_cdf(Lua* lua){
	int sumN = lua->scalar<int>("randomwalk_sumN");
	int npoints = lua->scalar<int>("randomwalk_npoints");
	randomwalk_max_x = lua->scalar<double>("randomwalk_max_x");
	double interpolation_order = lua->scalar<double>("randomwalk_interpolation_order");

	randomwalk_diffusion_time.resize(npoints);
	randomwalk_diffusion_time.interpolation_order = interpolation_order;
	randomwalk_xaxis = Axis(0,randomwalk_max_x,npoints);

	#pragma omp parallel for
	for(int i=1; i<=npoints; i++){
		double sum = 0;
		double x = randomwalk_xaxis.top[i];

		for(int n=1; n<=sumN; n++){
			double tmp = 2.0 * exp(-x * (n*pc::pi)*(n*pc::pi));
			if(n%2 == 0) tmp *= -1;
			sum += tmp;
	    }

		randomwalk_diffusion_time.set(i,1.0-sum);
	}
	randomwalk_diffusion_time.normalize();
}

//----------------------
// Do a random walk step
//----------------------
void Transport::random_walk(EinsteinHelper *eh) const{
	PRINT_ASSERT(eh->scatopac,>,0);
	PRINT_ASSERT(eh->absopac,>=,0);
	PRINT_ASSERT(eh->N,>=,0);
	PRINT_ASSERT(eh->nu(),>=,0);

	// save old values
	const Tuple<double,4> kup_tet_old = eh->kup_tet;
	const double Nold = eh->N;
	const int z_ind_old = eh->z_ind;
	size_t dir_ind_old[NDIMS+1];
	for(size_t i=0; i<NDIMS+1; i++) dir_ind_old[i] = eh->dir_ind[i];

	const double Rcom = eh->ds_com;
	const double D = pc::c / (3.*eh->scatopac);

	// sample the distance travelled during the random walk
	double path_length_com = pc::c * Rcom*Rcom / D * randomwalk_diffusion_time.invert(rangen.uniform(),&randomwalk_xaxis,-1);
	path_length_com = max(path_length_com,Rcom);

	// move along with the fluid
	double dtau = path_length_com / pc::c;
	for(size_t i=0; i<4; i++) eh->xup[i] += dtau * eh->u[i];

	// determine the average and final neutrino numbers
	double Naverage = eh->N, Nfinal = eh->N;
	if(eh->absopac > 0){
		double opt_depth = eh->absopac * path_length_com;
		Nfinal = eh->N * exp(-opt_depth);
		Naverage = (eh->N - Nfinal) / (opt_depth);
	}
	PRINT_ASSERT(Naverage,<=,Nold);
	PRINT_ASSERT(Nfinal,<=,Naverage);

	// select a random direction
	Tuple<double,4> kup_tet;
	isotropic_kup_tet(eh->nu(),kup_tet,&rangen);
	eh->set_kup_tet(kup_tet);
	eh->ds_com = Rcom;
	eh->N = Naverage;
	move(eh,false);
	eh->N = Nfinal;


	// select a random outward direction. Use delta=2 to make pdf=costheta
	Tuple<double,4> kup_tet_final = 0.0;
	kup_tet_final[3] = kup_tet[3];
	Tuple<double,3> direction;
	do{
		isotropic_direction(direction,&rangen);
	} while(reject_direction(Metric::dot_Minkowski<3>(direction,kup_tet)/kup_tet[3], 2.) );
	for(size_t i=0; i<3; i++) kup_tet_final[i] = direction[i] * kup_tet_final[3];
	eh->set_kup_tet(kup_tet_final);

	// contribute energy isotropically
	double Eiso;
	Eiso = pc::h*eh->nu() * Naverage * (path_length_com - Rcom);
	grid->distribution[eh->s]->add_isotropic(dir_ind_old, Eiso);
	grid->l_abs[z_ind_old] += (Nold - Nfinal) * species_list[eh->s]->lepton_number;
	for(size_t i=0; i<4; i++){
		grid->fourforce_abs[z_ind_old][i] += (kup_tet_old[i]*Nold - eh->kup_tet[i]*eh->N);
	}

}

void Transport::sample_scattering_final_state(EinsteinHelper *eh, const Tuple<double,4>& kup_tet_old) const{
	PRINT_ASSERT(use_scattering_kernels,>,0);
	PRINT_ASSERT(eh->scatopac,>,0);
	PRINT_ASSERT(grid->scattering_delta[eh->s].size(),>,0);
	PRINT_ASSERT(kup_tet_old[3],==,eh->kup_tet[3]);

	// rejection sampling to get outgoing frequency bin.
	size_t igout;
	double P;
	double part_opac_sum = 0.0;	
	size_t global_index = grid->scat_opac[eh->s].direct_index(eh->dir_ind);	
	for(igout=0; igout<grid->nu_grid_axis.size(); igout++){
		part_opac_sum += grid->partial_scat_opac[eh->s][igout][global_index];
	}
	PRINT_ASSERT(part_opac_sum,>=,0);
	if(part_opac_sum==0) return;
	do{
		igout = rangen.uniform_discrete(0, grid->nu_grid_axis.size()-1);
		P = grid->partial_scat_opac[eh->s][igout][global_index] / part_opac_sum;
		PRINT_ASSERT(P-1.0,<=,TINY);
		PRINT_ASSERT(P,>=,0.0);
		P = min(1.0,P);
	} while(rangen.uniform() > P);

	// if scattering to same group don't change energy. Otherwise, distribute uniformily
	double outnu;
	if(igout != eh->dir_ind[NDIMS])
		outnu = rangen.uniform(grid->nu_grid_axis.bottom(igout), grid->nu_grid_axis.top[igout]);
	else
		outnu = eh->grid_coords[NDIMS];

	// interpolate the kernel anisotropy
	double hyperloc[NDIMS+2];
	size_t dir_ind[NDIMS+2]; 
	for(size_t i=0; i<=NDIMS; i++){
			hyperloc[i] = eh->grid_coords[i];
			dir_ind[i] = eh->dir_ind[i];
	}
	dir_ind[NDIMS+1] = igout;
	hyperloc[NDIMS+1] = outnu;
	InterpolationCube<NDIMS+2> icube_kernel;
	grid->scattering_delta[eh->s].set_InterpolationCube(&icube_kernel,hyperloc,dir_ind);
	double delta = grid->scattering_delta[eh->s].interpolate(icube_kernel);
	PRINT_ASSERT(fabs(delta),<,3.0);

	// rejection sample the new direction, but only if not absurdly forward/backward peaked
	// (delta=2.8 corresponds to a possible factor of 10 in the neutrino weight)
	Tuple<double,4> kup_tet_new;
	if(fabs(delta) < 2.8){
		double costheta=0;
		do{
			isotropic_kup_tet(hyperloc[NDIMS+1], kup_tet_new, &rangen);
			costheta = Metric::dot_Minkowski<3>(kup_tet_new,kup_tet_old) / (kup_tet_old[3]*kup_tet_new[3]);
		} while(reject_direction(costheta, delta));
	}
	else{
	        kup_tet_new = eh->kup_tet * hyperloc[NDIMS+1] / eh->nu();
		if(delta<0) for(size_t i=0; i<3; i++) kup_tet_new[i] = -eh->kup_tet[i];
	}
	eh->set_kup_tet(kup_tet_new);
	PRINT_ASSERT(eh->N,<,1e99);
}
