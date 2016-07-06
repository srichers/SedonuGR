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

#include <omp.h>
#include <mpi.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include "physical_constants.h"
#include "Transport.h"
#include "Grid.h"
#include "Grid0DIsotropic.h"
#include "Grid1DSphere.h"
#include "Grid2DSphere.h"
#include "Grid2DCylinder.h"
#include "Grid3DCart.h"
#include "Species.h"
#include "Neutrino.h"
#include "nulib_interface.h"
#include "global_options.h"

using namespace std;
namespace pc = physical_constants;

// constructor
Transport::Transport(){
	verbose = -MAXLIM;
	MPI_nprocs = -MAXLIM;
	MPI_myID = -MAXLIM;
	equilibrium_T = -MAXLIM;
	equilibrium_Ye = -MAXLIM;
	equilibrium_damping = NaN;
	equilibrium_itmax = -MAXLIM;
	equilibrium_tolerance = NaN;
	T_min = NaN;
	T_max = NaN;
	Ye_min = NaN;
	Ye_max = NaN;
	rho_min = NaN;
	rho_max = NaN;
	max_particles = -MAXLIM;
	step_size = NaN;
	randomwalk_sphere_size = NaN;
	min_packet_energy = NaN;
	max_packet_energy = NaN;
	do_annihilation = -MAXLIM;
	radiative_eq = -MAXLIM;
	rank0 = -MAXLIM;
	grid = NULL;
	r_core = NaN;
	core_emit_method=-MAXLIM;
	n_emit_core = -MAXLIM;
	n_emit_core_per_bin = -MAXLIM;
	core_lum_multiplier = NaN;
	do_visc = -MAXLIM;
	do_relativity = -MAXLIM;
	n_emit_zones = -MAXLIM;
	n_emit_zones_per_bin = -MAXLIM;
	visc_specific_heat_rate = NaN;
	reflect_outer = -MAXLIM;
	n_subcycles = -MAXLIM;
	do_emit_by_bin = -MAXLIM;
	write_rays_every = -MAXLIM;
	write_spectra_every = -MAXLIM;
	write_zones_every = -MAXLIM;
	particle_total_energy = NaN;
	particle_core_abs_energy = NaN;
	particle_fluid_abs_energy = NaN;
	particle_escape_energy = NaN;
	importance_bias = NaN;
	min_importance = NaN;
	bias_path_length = -MAXLIM;
	max_path_length_boost = NaN;
	randomwalk_min_optical_depth = NaN;
	randomwalk_max_x = NaN;
	randomwalk_sumN = -MAXLIM;
}


//----------------------------------------------------------------------------
// Initialize the transport module
// Includes setting up the grid, particles,
// and MPI work distribution
//----------------------------------------------------------------------------
void Transport::init(Lua* lua)
{ 
	// get mpi rank
	MPI_Comm_size( MPI_COMM_WORLD, &MPI_nprocs );
	MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID  );
	rank0 = (MPI_myID==0);
	if(rank0){
		cout << "# Initializing transport..." << endl;
		cout << "#   Using " << MPI_nprocs << " MPI ranks" << endl;
            #ifdef _OPENMP
		    #pragma omp parallel
		    #pragma omp single
		    cout << "#   Using " << omp_get_num_threads()  << " threads on each MPI rank." << endl;
            #endif
	}

	// figure out what emission models we're using
	do_visc      = lua->scalar<int>("do_visc");
	do_relativity = lua->scalar<int>("do_relativity");
	if(do_visc) visc_specific_heat_rate = lua->scalar<double>("visc_specific_heat_rate");
	reflect_outer = lua->scalar<int>("reflect_outer");
	n_subcycles = lua->scalar<int>("n_subcycles");
	PRINT_ASSERT(n_subcycles,>=,1);
	do_emit_by_bin = lua->scalar<int>("do_emit_by_bin");
	if(do_emit_by_bin){
		n_emit_zones_per_bin = lua->scalar<int>("n_emit_therm_per_bin");
		n_emit_core_per_bin  = lua->scalar<int>("n_emit_core_per_bin");
	}
	else{
		n_emit_zones = lua->scalar<int>("n_emit_therm");
		n_emit_core  = lua->scalar<int>("n_emit_core");
	}

	// read simulation parameters
	exponential_decay = lua->scalar<int>("exponential_decay");
	verbose      = lua->scalar<int>("verbose");
	do_annihilation = lua->scalar<int>("do_annihilation");
	radiative_eq = lua->scalar<int>("radiative_eq");
	equilibrium_T       = lua->scalar<int>("equilibrium_T");
	equilibrium_Ye      = lua->scalar<int>("equilibrium_Ye");
	if(equilibrium_T || equilibrium_Ye){
		equilibrium_damping   = lua->scalar<double>("equilibrium_damping");
		equilibrium_itmax     = lua->scalar<int>("equilibrium_itmax");
		equilibrium_tolerance = lua->scalar<double>("equilibrium_tolerance");
	}
	step_size     = lua->scalar<double>("step_size");
	randomwalk_sphere_size = lua->scalar<double>("randomwalk_sphere_size");
	if(randomwalk_sphere_size>0){
		randomwalk_min_optical_depth = lua->scalar<double>("randomwalk_min_optical_depth");
		init_randomwalk_cdf(lua);
	}
	importance_bias = lua->scalar<double>("importance_bias");
	if(importance_bias>0) min_importance = lua->scalar<double>("min_importance");
	bias_path_length = lua->scalar<int>("bias_path_length");
	if(bias_path_length) max_path_length_boost = lua->scalar<double>("max_path_length_boost");
	min_packet_energy = lua->scalar<double>("min_packet_energy");
	max_packet_energy = lua->scalar<double>("max_packet_energy");

	// output parameters
	write_zones_every   = lua->scalar<double>("write_zones_every");
	write_rays_every    = lua->scalar<double>("write_rays_every");
	write_spectra_every = lua->scalar<double>("write_spectra_every");

	//=================//
	// SET UP THE GRID //
	//=================//
	// read the grid type
	string grid_type = lua->scalar<string>("grid_type");

	// create a grid of the appropriate type
	if     (grid_type == "Grid0DIsotropic") grid = new Grid0DIsotropic;
	else if(grid_type == "Grid1DSphere"   ) grid = new Grid1DSphere;
	else if(grid_type == "Grid2DSphere"   ) grid = new Grid2DSphere;
	else if(grid_type == "Grid2DCylinder" ) grid = new Grid2DCylinder;
	else if(grid_type == "Grid3DCart"     ) grid = new Grid3DCart;
	else{
		if(rank0) std::cout << "# ERROR: the requested grid type is not implemented." << std::endl;
		exit(3);}
	grid->init(lua);

	// Reserve all the memory we might need right now. Speeds up particle additions.
	max_particles = lua->scalar<int>("max_particles");
	particles.reserve(max_particles);

	//===============//
	// GENERAL SETUP //
	//===============//
	// figure out which zones are in this processors work load
	// a processor will do work in range [start,end)
	my_zone_end.resize(MPI_nprocs);
	for(int proc=0; proc<MPI_nprocs; proc++){
		// how much work does this processor do?
		int my_job = (int)(grid->z.size()/(1.0*MPI_nprocs));
		if(my_job < 1) my_job = 1;

		// where does this processor start and stop its work? (only the end needs to be stored)
		int my_zone_start = proc*my_job;
		my_zone_end[proc] = my_zone_start + my_job;

		// make sure last guy finishes it all
		if(proc == MPI_nprocs-1) my_zone_end[proc] = grid->z.size();

		// make sure nobody goes overboard
		if(my_zone_end[proc] >= grid->z.size()) my_zone_end[proc] = grid->z.size();
	}

	// setup and seed random number generator(s)
	rangen.init();

	//==================//
	// SET UP TRANSPORT //
	//==================//
	double grey_opac = lua->scalar<double>("grey_opacity");
	int num_nut_species = 0;
	if(grey_opac < 0){ // get everything from NuLib
		// read the fortran module into memory
		if(rank0) cout << "# Initializing NuLib..." << endl;
		string nulib_table = lua->scalar<string>("nulib_table");
		nulib_init(nulib_table);
		num_nut_species = nulib_get_nspecies();

		// eos
		string eos_filename = lua->scalar<string>("nulib_eos");
		nulib_eos_read_table((char*)eos_filename.c_str());

	}
	else{
		if(rank0) cout << "#   Using grey opacity for electron anti/neutrinos (0 chemical potential)" << endl;
		num_nut_species = 2;
	}

	// create the species arrays
	if(rank0) cout << "# Setting up misc. transport tools..." << endl;
	for(int i=0; i<num_nut_species; i++){
		Neutrino* neutrinos_tmp = new Neutrino;
		neutrinos_tmp->nulibID = i;
		neutrinos_tmp->num_species = num_nut_species;
		neutrinos_tmp->init(lua, this);
		species_list.push_back(neutrinos_tmp);
	}

	// complain if we're not simulating anything
	n_active.resize(species_list.size(),0);
	n_escape.resize(species_list.size(),0);
	if(species_list.size() == 0)
	{
		if(rank0) cout << "ERROR: you must simulate at least one species of particle." << endl;
		exit(7);
	}

	// set global min/max values (make range infinite to catch errors)
	T_min   =  numeric_limits<double>::infinity();
	T_max   = -numeric_limits<double>::infinity();
	Ye_min  =  numeric_limits<double>::infinity();
	Ye_max  = -numeric_limits<double>::infinity();
	rho_min =  numeric_limits<double>::infinity();
	rho_max = -numeric_limits<double>::infinity();
	for(unsigned i=0; i<species_list.size(); i++)
	{
		if(species_list[i]->T_min   < T_min  ) T_min   = species_list[i]->T_min;
		if(species_list[i]->T_max   > T_max  ) T_max   = species_list[i]->T_max;
		if(species_list[i]->Ye_min  < Ye_min ) Ye_min  = species_list[i]->Ye_min;
		if(species_list[i]->Ye_max  > Ye_max ) Ye_max  = species_list[i]->Ye_max;
		if(species_list[i]->rho_min < rho_min) rho_min = species_list[i]->rho_min;
		if(species_list[i]->rho_max > rho_max) rho_max = species_list[i]->rho_max;
	}

	PRINT_ASSERT(T_min,>=,0);
	PRINT_ASSERT(T_max,>,T_min);
	PRINT_ASSERT(Ye_min,>=,0);
	PRINT_ASSERT(Ye_max,>,Ye_min);
	PRINT_ASSERT(Ye_max,<=,1.0);

	//=================//
	// SET UP THE CORE //
	//=================//
	if(rank0) cout << "# Initializing the core...";
	r_core = lua->scalar<double>("r_core");   // cm
	if(n_emit_core>0 || n_emit_core_per_bin>0){
		core_emit_method = lua->scalar<int>("core_emit_method");
		PRINT_ASSERT(core_emit_method==1,||,core_emit_method==2);
		int iorder = lua->scalar<int>("cdf_interpolation_order");
		core_species_luminosity.interpolation_order = iorder;
		for(unsigned s=0; s<species_list.size(); s++) species_list[s]->core_emis.interpolation_order = iorder;

		if(core_emit_method==1){ // give temperature, mu, multiplier
			core_lum_multiplier = lua->scalar<double>("core_lum_multiplier");
			double T_core = lua->scalar<double>("T_core") / pc::k_MeV;    // K
			double munue_core = lua->scalar<double>("core_nue_chem_pot") * pc::MeV_to_ergs; // erg
			init_core(r_core, T_core, munue_core);
		}
		if(core_emit_method==2){ // give temperature, mu, luminosity
			PRINT_ASSERT(species_list.size(),==,3);
			vector<double> T_core(species_list.size(),0);
			vector<double> L_core(species_list.size(),0);
			vector<double> mu_core(species_list.size(),0);
			string param;
			for(unsigned i=0; i<species_list.size(); i++){
				param = "T" + std::to_string(i) + "_core";
				T_core[i] = lua->scalar<double>(param.c_str()) / pc::k_MeV; //K
				param = "L" + std::to_string(i) + "_core";
				L_core[i] = lua->scalar<double>(param.c_str()); //erg/s
				param = "mu" + std::to_string(i) + "_core";
				mu_core[i] = lua->scalar<double>(param.c_str()) * pc::MeV_to_ergs; //erg
			}
			init_core(r_core,T_core,mu_core,L_core);
		}
	}
	if(rank0) cout << "finished." << endl;

	// check the parameters
	if(rank0) cout << "# Checking parameters...";
	check_parameters();
	if(rank0) cout << "finished." << endl;

	// explicitly set global radiation quantities to 0
	L_core_lab.resize(species_list.size());
	L_net_lab.resize(species_list.size());
	L_net_esc.resize(species_list.size());
	E_avg_lab.resize(species_list.size());
	E_avg_esc.resize(species_list.size());
	N_net_lab.resize(species_list.size());
	N_net_esc.resize(species_list.size());
	for(unsigned s=0; s<species_list.size(); s++){
		L_core_lab[s] = 0;
		L_net_lab[s] = 0;
		L_net_esc[s] = 0;
		E_avg_lab[s] = 0;
		E_avg_esc[s] = 0;
		N_net_lab[s] = 0;
		N_net_esc[s] = 0;
	}
	particle_total_energy = 0;
	particle_core_abs_energy = 0;
	particle_fluid_abs_energy = 0;
	particle_escape_energy = 0;
}

//-----------------------------------
// set up core (without reading lua)
//-----------------------------------
void Transport::init_core(const double r_core /*cm*/, const vector<double>& T_core /*K*/, const vector<double>& mu_core /*erg*/, const vector<double>& L_core /*erg/s*/){
	PRINT_ASSERT(n_emit_core>0,||,n_emit_core_per_bin>0);
	PRINT_ASSERT(r_core,>,0);
	PRINT_ASSERT(species_list.size(),>,0);

	// set up core emission spectrum function (erg/s)
	core_species_luminosity.resize(species_list.size());
	for(unsigned s=0; s<species_list.size(); s++){
		species_list[s]->set_cdf_to_BB(T_core[s],mu_core[s],species_list[s]->core_emis);
		species_list[s]->core_emis.N = L_core[s]; //erg/s
		core_species_luminosity.set_value(s,species_list[s]->integrate_core_emis());
	}
	core_species_luminosity.normalize();
}
void Transport::init_core(const double r_core /*cm*/, const double T_core /*K*/, const double munue_core /*erg*/){
	PRINT_ASSERT(n_emit_core>0,||,n_emit_core_per_bin>0);
	PRINT_ASSERT(r_core,>,0);
	PRINT_ASSERT(T_core,>,0);
	PRINT_ASSERT(species_list.size(),>,0);

	// set up core emission spectrum function (erg/s)
	core_species_luminosity.resize(species_list.size());
	for(unsigned s=0; s<species_list.size(); s++){
		double chempot = munue_core * (double)species_list[s]->lepton_number; // erg
		species_list[s]->set_cdf_to_BB(T_core, chempot, species_list[s]->core_emis);
		species_list[s]->core_emis.N *= pc::pi * (4.0*pc::pi*r_core*r_core) * species_list[s]->weight * core_lum_multiplier; // erg/s
		core_species_luminosity.set_value(s, species_list[s]->integrate_core_emis());
	}
	core_species_luminosity.normalize();
}


void Transport::check_parameters() const{
	if(n_emit_zones>0 && radiative_eq){
		cout << "ERROR: Emitting particles at beginning of timestep AND re-emitting them is inconsistent." << endl;
		exit(10);
	}
}

//------------------------------------------------------------
// take a transport time step 
//------------------------------------------------------------
void Transport::step()
{
	assert(particles.empty());

	// reset radiation quantities
	if(rank0 && verbose) cout << "# Clearing radiation..." << endl;
	reset_radiation();

	// emit, propagate, and normalize. steady_state means no propagation time limit.
	for(int i=0; i<n_subcycles; i++){
	  if(rank0 && verbose) cout << "#   subcycle " << i+1 << "/" << n_subcycles << endl;
		emit_particles();
		propagate_particles();
	}
	if(MPI_nprocs>1) reduce_radiation();      // so each processor has necessary info to solve its zones
	normalize_radiative_quantities();
	if(do_annihilation) calculate_annihilation();

	// solve for T_gas and Ye structure
	if(equilibrium_T || equilibrium_Ye){
		if(equilibrium_T || equilibrium_Ye) solve_eq_zone_values();  // solve T,Ye s.t. E_abs=E_emit and N_abs=N_emit
		else update_zone_quantities();         // update T,Ye based on heat capacity and number of leptons
	}
	if(MPI_nprocs>1) synchronize_gas();       // each processor broadcasts its solved zones to the other processors

	// post-processing
	if(rank0) calculate_timescales();
}


//---------------------------------
// write all the necessary output
//---------------------------------
void Transport::write(const int it) const{
	if(rank0){
		// write zone state when appropriate
		if(write_zones_every>0) if(it%write_zones_every==0 && write_zones_every>0){
			if(verbose) cout << "# writing zone file " << it << endl;
			grid->write_zones(it);
		}

		// write ray data when appropriate
		if(write_rays_every>0) if(it%write_rays_every==0 && write_rays_every>0){
			if(verbose) cout << "# writing ray file " << it << endl;
			grid->write_rays(it);
		}

		// print out spectrum when appropriate
		if(write_spectra_every>0) if(it%write_spectra_every==0 && write_spectra_every>0){
			if(verbose) cout << "# writing spectrum files " << it << endl;
			for(unsigned i=0; i<species_list.size(); i++) species_list[i]->spectrum.print(it,i);
		}
	}
}

//----------------------------
// reset radiation quantities
//------------------------------
void Transport::reset_radiation(){
	// clear global radiation quantities
	for(unsigned i=0; i<species_list.size(); i++){
		species_list[i]->spectrum.wipe();
		n_active[i] = 0;
		n_escape[i] = 0;
		L_core_lab[i] = 0;
		L_net_lab[i] = 0;
		L_net_esc[i] = 0;
		E_avg_lab[i] = 0;
		E_avg_esc[i] = 0;
		N_net_lab[i] = 0;
		N_net_esc[i] = 0;
	}

	#pragma omp parallel for schedule(guided)
	// prepare zone quantities for another round of transport
	for(unsigned z_ind=0;z_ind<grid->z.size();z_ind++)
	{
		Zone* z = &(grid->z[z_ind]);
		z->nue_abs  = 0;
		z->anue_abs = 0;
		z->e_abs    = 0;
		z->l_emit   = 0;
		z->e_emit   = 0;
		z->Q_annihil = 0;

		for(unsigned s=0; s<species_list.size(); s++){
			z->distribution[s].wipe();
			species_list[s]->set_eas(z_ind);
		}
	}
}

//-----------------------------
// calculate annihilation rates
//-----------------------------
void Transport::calculate_annihilation() const{
	if(rank0 && verbose) cout << "# Calculating annihilation rates...";

	// remember what zones I'm responsible for
	int start = ( MPI_myID==0 ? 0 : my_zone_end[MPI_myID - 1] );
	int end = my_zone_end[MPI_myID];
	PRINT_ASSERT(end,>=,start);
	PRINT_ASSERT(start,>=,0);
	PRINT_ASSERT(end,<=,(int)grid->z.size());

	vector<double> H_nunu_lab(species_list.size(),0);

    #pragma omp parallel for schedule(guided)
	for(int z_ind=start; z_ind<end; z_ind++){

		// based on nulib's prescription for which species each distribution represents
		unsigned s_nue    = 0;
		unsigned s_nubare = 1;
		PRINT_ASSERT(species_list[s_nue]->weight,==,species_list[s_nubare]->weight);
		double Q_tmp = 0;
		double vol = grid->zone_lab_volume(z_ind);
		double zone_annihil_net = 0;
		Q_tmp = Neutrino::annihilation_rate(grid->z[z_ind].distribution[s_nue],
				 grid->z[z_ind].distribution[s_nubare],
				 true,species_list[s_nue]->weight);
		zone_annihil_net += Q_tmp;
		#pragma omp atomic
		H_nunu_lab[0] += Q_tmp*vol;
		PRINT_ASSERT(H_nunu_lab[1],==,0);

		if(species_list.size()==3){
			unsigned s_nux = 2;
			Q_tmp = Neutrino::annihilation_rate(grid->z[z_ind].distribution[s_nux],
					grid->z[z_ind].distribution[s_nux],
					false,species_list[s_nux]->weight);
			zone_annihil_net += 2.0 * Q_tmp;
			#pragma omp atomic
			H_nunu_lab[2] += 2.0 * Q_tmp*vol;
		}

		else if(species_list.size()==4){
			unsigned s_nux = 2;
			unsigned s_nubarx = 3;
			PRINT_ASSERT(species_list[s_nux]->weight,==,species_list[s_nubarx]->weight);
			Q_tmp = Neutrino::annihilation_rate(grid->z[z_ind].distribution[s_nux   ],
					grid->z[z_ind].distribution[s_nubarx],
					false,species_list[s_nux]->weight);
			zone_annihil_net += 2.0 * Q_tmp;
			#pragma omp atomic
			H_nunu_lab[2] += 2.0 * Q_tmp*vol;
			PRINT_ASSERT(H_nunu_lab[3],==,0);
		}

		else if(species_list.size()==6){
			unsigned s_numu = 2;
			unsigned s_nubarmu = 3;
			unsigned s_nutau = 4;
			unsigned s_nubartau = 5;
			PRINT_ASSERT(species_list[s_numu]->weight,==,species_list[s_nubarmu]->weight);
			PRINT_ASSERT(species_list[s_nutau]->weight,==,species_list[s_nubartau]->weight);
			Q_tmp = Neutrino::annihilation_rate(grid->z[z_ind].distribution[s_numu   ],
					  grid->z[z_ind].distribution[s_nubarmu],
					  false,species_list[s_numu]->weight);
			zone_annihil_net += Q_tmp;
			#pragma omp atomic
			H_nunu_lab[2] += Q_tmp*vol;
			PRINT_ASSERT(H_nunu_lab[3],==,0);
			Q_tmp = Neutrino::annihilation_rate(grid->z[z_ind].distribution[s_nutau   ],
					  grid->z[z_ind].distribution[s_nubartau],
					  false,species_list[s_nutau]->weight);
			zone_annihil_net += Q_tmp;
			#pragma omp atomic
			H_nunu_lab[4] += Q_tmp*vol;
			PRINT_ASSERT(H_nunu_lab[5],==,0);
		}
		else{
			cout << "ERROR: wrong species list size in calculate_annihilation" << endl;
			exit(34);
		}

		// set the value in grid
		grid->z[z_ind].Q_annihil = zone_annihil_net;
	}

	// synchronize global quantities between processors
	vector<double> send(species_list.size(),0);
	vector<double> receive(species_list.size(),0);
	for(unsigned s=0; s<species_list.size(); s++) send[s] = H_nunu_lab[s];
	MPI_Allreduce(&send.front(), &receive.front(), species_list.size(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	for(unsigned s=0; s<species_list.size(); s++) H_nunu_lab[s] = receive[s];

	// write to screen
	if(rank0) cout << "finished." << endl;
	if(rank0 && verbose) {
		cout << "#   { ";
		for(unsigned s=0; s<species_list.size(); s++) cout << H_nunu_lab[s] << " ";
		cout << " } erg/s H_annihil" << endl;
	}
}

//-----------------------------
// calculate various timescales
//-----------------------------
void Transport::calculate_timescales() const{
    #pragma omp parallel for
	for(unsigned i=0;i<grid->z.size();i++){
		Zone *z = &(grid->z[i]);
		double e_gas = z->rho*pc::k*z->T/pc::m_p;  // gas energy density (erg/ccm)
		z->t_eabs  = e_gas / z->e_abs;
		z->t_eemit = e_gas / z->e_emit;
		z->t_labs  = z->rho/pc::m_p / (z->nue_abs-z->anue_abs);
		z->t_lemit = z->rho/pc::m_p / z->l_emit;
	}
}

//----------------------------------------------------------------------------
// normalize the radiative quantities
//----------------------------------------------------------------------------
void Transport::normalize_radiative_quantities(){
	if(verbose && rank0) cout << "# Normalizing Radiative Quantities" << endl;
	double net_visc_heating = 0;
	double net_neut_heating = 0;

	// normalize zone quantities
	double multiplier = (double)n_subcycles;
	double inv_multiplier = 1.0/multiplier;
    #pragma omp parallel for reduction(+:net_visc_heating,net_neut_heating)
	for(unsigned z_ind=0;z_ind<grid->z.size();z_ind++)
	{
		Zone *z = &(grid->z[z_ind]);
		double inv_mult_four_vol = 1.0/(multiplier * grid->zone_lab_volume(z_ind)); // Lorentz invariant - same in lab and comoving frames. Assume lab_dt=1.0
		PRINT_ASSERT(inv_mult_four_vol,>=,0);

		if(!grid->good_zone(z_ind)){
			PRINT_ASSERT(z->e_abs,==,0.0);
			PRINT_ASSERT(z->nue_abs,==,0.0);
			PRINT_ASSERT(z->anue_abs,==,0.0);
			PRINT_ASSERT(z->e_emit,==,0.0);
			PRINT_ASSERT(z->l_emit,==,0.0);
		}

		z->e_abs    *= inv_mult_four_vol;       // erg      --> erg/ccm/s
		z->e_emit   *= inv_mult_four_vol;       // erg      --> erg/ccm/s
		z->nue_abs  *= inv_mult_four_vol;       // num      --> num/ccm/s
		z->anue_abs *= inv_mult_four_vol;       // num      --> num/ccm/s
		z->l_emit   *= inv_mult_four_vol;       // num      --> num/ccm/s

		// erg*dist --> erg/ccm, represents *all* species if nux
		for(unsigned s=0; s<species_list.size(); s++) z->distribution[s].rescale(inv_mult_four_vol * pc::inv_c);

		// tally heat absorbed from viscosity and neutrinos
		if(do_visc && grid->good_zone(z_ind)){
			net_visc_heating += zone_comoving_visc_heat_rate(z_ind);      // erg/s
			net_neut_heating += z->e_abs * grid->zone_comoving_volume(z_ind);
		}
	}

	// normalize global quantities
	for(unsigned s=0; s<species_list.size(); s++){
		species_list[s]->spectrum.rescale(inv_multiplier); // erg/s in each bin. Assume lab_dt=1.0
		E_avg_lab[s] /= L_net_lab[s];
		E_avg_esc[s] /= L_net_esc[s];
		L_core_lab[s] *= inv_multiplier; // assume lab_dt=1.0
		L_net_lab[s] *= inv_multiplier; // assume lab_dt=1.0
		L_net_esc[s] *= inv_multiplier; // assume lab_dt=1.0
		N_net_lab[s] *= inv_multiplier; // assume lab_dt=1.0
		N_net_esc[s] *= inv_multiplier; // assume lab_dt=1.0
	}
	particle_total_energy *= inv_multiplier;
	particle_core_abs_energy *= inv_multiplier;
	particle_fluid_abs_energy *= inv_multiplier;
	particle_escape_energy *= inv_multiplier;

	// output useful stuff
	if(rank0 && verbose){
		unsigned long total_active = 0;
		for(unsigned i=0; i<species_list.size(); i++){
			double per_esc = (100.0*(double)n_escape[i])/(double)n_active[i];
			total_active += n_active[i];
			cout << "#   " << n_escape[i] << "/" << n_active[i] << " " << species_list[i]->name << " escaped. (" << per_esc << "%)" << endl;
		}

		// particle information (all lab-frame)
		cout << "#   " << total_active << " total active particles" << endl;
		cout << "#   " << particle_total_energy << " ERG/S TOTAL PARTICLE LUMINOSITY " << endl; // assume lab_dt=1.0
		cout << "#   " << particle_fluid_abs_energy << " ERG/S TOTAL DESTROYED (fluid) PARTICLE ENERGY " << endl; // assume lab_dt=1.0
		cout << "#   " << particle_core_abs_energy << " ERG/S TOTAL DESTROYED (core)  PARTICLE ENERGY " << endl; // assume lab_dt=1.0
		cout << "#   " << particle_escape_energy << " ERG/S TOTAL ESCAPED PARTICLE ENERGY " << endl; // assume lab_dt=1.0

		if(do_visc) cout << "#   " << net_visc_heating << " erg/s H_visc (comoving sum)" << endl;
		cout << "#   " << net_neut_heating << " erg/s H_abs (comoving sum)" << endl;

		cout << "#   SUM OF EMISSIONS: " << L_core_lab[0]+L_core_lab[1]+L_core_lab[2]+L_net_lab[0]+L_net_lab[1]+L_net_lab[2] << endl;

		double CmH = 0;
		for(unsigned s=0; s<species_list.size(); s++) CmH += L_net_lab[s];
		CmH -= particle_fluid_abs_energy; // assume lab+dt=1.0
		cout << "#   " << CmH << " erg/s L_emit-L_abs (lab-frame)" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<L_core_lab.size(); s++) cout << setw(12) << L_core_lab[s] << "  ";
		cout << "} erg/s L_core" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<L_net_lab.size(); s++) cout << setw(12) << L_net_lab[s] << "  ";
		cout << "} erg/s L_emit (lab-frame sum, includes re-emission)" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<L_net_esc.size(); s++) cout << setw(12) << L_net_esc[s] << "  ";
		cout << "} erg/s L_esc" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<E_avg_lab.size(); s++) cout << setw(12) << E_avg_lab[s]*pc::h_MeV << "  ";
		cout << "} MeV E_avg_lab (average lab-frame emitted energy)" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<E_avg_esc.size(); s++) cout << setw(12) << E_avg_esc[s]*pc::h_MeV << "  ";
		cout << "} MeV E_avg_esc" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<N_net_lab.size(); s++) cout << setw(12) << N_net_lab[s] << "  ";
		cout << "} 1/s N_lab" << endl;

		cout << "#   { ";
		for(unsigned s=0; s<N_net_esc.size(); s++) cout << setw(12) << N_net_esc[s] << "  ";
		cout << "} 1/s N_esc" << endl;

		double dyedt = -(N_net_esc[0]-N_net_esc[1]) / (grid->total_rest_mass()/pc::m_n);
		cout << "#   " << dyedt << " 1/s global dYe_dt" << endl;

		// just latex output to make plugging numbers into the paper easier
		//cout << CmH/1e51 << " & " << dyedt;
		//for(unsigned s=0; s<species_list.size(); s++) cout << " & " << L_net_lab[s]/1e51;
		//for(unsigned s=0; s<species_list.size(); s++) cout << " & " << L_net_esc[s]/1e51;
		//for(unsigned s=0; s<species_list.size(); s++) cout << " & " << E_avg_lab[s]*pc::h_MeV;
		//for(unsigned s=0; s<species_list.size(); s++) cout << " & " << E_avg_esc[s]*pc::h_MeV;
		//cout << endl;
	}
}


//----------------------------------------------------------------------------
// sum up the number of particles in all species
//----------------------------------------------------------------------------
int Transport::total_particles() const{
	return particles.size();
}


//----------------------------------------------------------------------------
// randomly sample the nu-integrated emissivities of all
// species to determine the species of a new particle
// emitted from the core
//----------------------------------------------------------------------------
int Transport::sample_core_species() const
{
	// randomly sample the species (precomputed CDF)
	double z = rangen.uniform();
	return core_species_luminosity.get_index(z);
}



//----------------------------------------------------------------------------
// randomly sample the nu-integrated emissivities of all
// species to determine the species of a new particle
// emitted from a zone
//----------------------------------------------------------------------------
// note: could store a zone_species_cdf structure in transport,
// but this would use more memory. Here, trading CPU cycles for 
// memory. If we are CPU limited, we could change this
void Transport::sample_zone_species(Particle *p, const int zone_index) const
{
	CDFArray species_cdf;
	double integrated_emis;
	species_cdf.resize(species_list.size());

	// set values and normalize
	for(unsigned i=0; i<species_list.size(); i++)
	{
		integrated_emis = species_list[i]->integrate_zone_biased_emis(zone_index);
		species_cdf.set_value(i,integrated_emis);
	}
	species_cdf.normalize();
	PRINT_ASSERT(species_cdf.N,>=,0);

	// randomly sample the species
	double z = rangen.uniform();
	p->s = species_cdf.get_index(z);
	if(importance_bias>0) p->e *= species_list[p->s]->integrate_zone_emis(zone_index) / species_list[p->s]->integrate_zone_biased_emis(zone_index);
	PRINT_ASSERT(p->e,>,0);
	PRINT_ASSERT(p->e,<,INFINITY);
}


//------------------------------------------------------------
// Combine the radiation tallies in all zones
// from all processors using MPI reduce
// after this, radiation quantities on all procs match
//------------------------------------------------------------
void Transport::reduce_radiation()
{
	if(verbose && rank0) cout << "# Reducing Radiation" << endl;
	int my_begin, my_end, size;

	// reduce the spectra
	if(verbose && rank0) cout << "#   spectra" << endl;
	for(unsigned i=0; i<species_list.size(); i++) species_list[i]->spectrum.MPI_average();

	// reduce the global radiation scalars
	double sendsingle = 0;

	sendsingle = particle_total_energy;
	MPI_Allreduce(&sendsingle,&particle_total_energy,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	particle_total_energy /= (double)MPI_nprocs;

	sendsingle = particle_fluid_abs_energy;
	MPI_Allreduce(&sendsingle,&particle_fluid_abs_energy,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	particle_fluid_abs_energy /= (double)MPI_nprocs;

	sendsingle = particle_core_abs_energy;
	MPI_Allreduce(&sendsingle,&particle_core_abs_energy,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	particle_core_abs_energy /= (double)MPI_nprocs;

	sendsingle = particle_escape_energy;
	MPI_Allreduce(&sendsingle,&particle_escape_energy,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	particle_escape_energy /= (double)MPI_nprocs;

	// reduce the global radiation counters
	if(verbose && rank0) cout << "#   global scalars" << endl;
	vector<double> send(species_list.size(),0);
	vector<double> receive(species_list.size(),0);

	for(unsigned i=0; i<species_list.size(); i++) send[i] = L_net_esc[i];
	MPI_Allreduce(&send.front(),&receive.front(),L_net_esc.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) L_net_esc[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = L_core_lab[i];
	MPI_Allreduce(&send.front(),&receive.front(),L_core_lab.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) L_core_lab[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = L_net_lab[i];
	MPI_Allreduce(&send.front(),&receive.front(),L_net_lab.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) L_net_lab[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = E_avg_esc[i];
	MPI_Allreduce(&send.front(),&receive.front(),E_avg_esc.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) E_avg_esc[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = E_avg_lab[i];
	MPI_Allreduce(&send.front(),&receive.front(),E_avg_lab.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) E_avg_lab[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = N_net_esc[i];
	MPI_Allreduce(&send.front(),&receive.front(),N_net_esc.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) N_net_esc[i] = receive[i]/(double)MPI_nprocs;

	for(unsigned i=0; i<species_list.size(); i++) send[i] = N_net_lab[i];
	MPI_Allreduce(&send.front(),&receive.front(),N_net_lab.size(),MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) N_net_lab[i] = receive[i]/(double)MPI_nprocs;


	// reduce the numbers of particles active/escaped
	if(verbose && rank0) cout << "#   particle numbers" << endl;
	vector<long> sendint(species_list.size(),0);
	vector<long> receiveint(species_list.size(),0);

	for(unsigned i=0; i<species_list.size(); i++) sendint[i] = n_escape[i];
	MPI_Allreduce(&sendint.front(), &receiveint.front(), n_escape.size(), MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) n_escape[i] = receiveint[i];

	for(unsigned i=0; i<species_list.size(); i++) sendint[i] = n_active[i];
	MPI_Allreduce(&sendint.front(), &receiveint.front(), n_active.size(), MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
	for(unsigned i=0; i<species_list.size(); i++) n_active[i] = receiveint[i];

	//-- EACH PROCESSOR GETS THE REDUCTION INFORMATION IT NEEDS
	if(verbose && rank0) cout << "#   fluid" << endl;
	//for(int proc=0; proc<MPI_nprocs; proc++){

		// set the begin and end indices so a process covers range [begin,end)
		my_begin = 0;
		my_end = grid->z.size();

		// set the computation size and create the send/receive vectors
		size = my_end - my_begin;
		send.resize(size);
		receive.resize(size);

		// reduce distribution
		for(unsigned s=0; s<species_list.size(); s++)
			for(int i=my_begin; i<my_end; i++) grid->z[i].distribution[s].MPI_average();

		// reduce e_abs
		for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].e_abs;
		MPI_Allreduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		for(int i=my_begin; i<my_end; i++) grid->z[i].e_abs = receive[i-my_begin] / (double)MPI_nprocs;

		// reduce nue_abs
		for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].nue_abs;
		MPI_Allreduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		for(int i=my_begin; i<my_end; i++) grid->z[i].nue_abs = receive[i-my_begin] / (double)MPI_nprocs;

		// reduce anue_abs
		for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].anue_abs;
		MPI_Allreduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		for(int i=my_begin; i<my_end; i++) grid->z[i].anue_abs = receive[i-my_begin] / (double)MPI_nprocs;

		// reduce e_emit
		for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].e_emit;
		MPI_Allreduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		for(int i=my_begin; i<my_end; i++) grid->z[i].e_emit = receive[i-my_begin] / (double)MPI_nprocs;

		// reduce l_emit
		for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].l_emit;
		MPI_Allreduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		for(int i=my_begin; i<my_end; i++) grid->z[i].l_emit = receive[i-my_begin] / (double)MPI_nprocs;

		// format for single reduce
		//my_begin = ( proc==0 ? 0 : my_zone_end[proc-1] );
		//my_end = my_zone_end[proc];
		//for(int i=my_begin; i<my_end; i++) send[i-my_begin] = grid->z[i].l_emit;
		//MPI_Reduce(&send.front(), &receive.front(), size, MPI_DOUBLE, MPI_SUM, proc, MPI_COMM_WORLD);
		//for(int i=my_begin; i<my_end; i++) grid->z[i].l_emit = receive[i-my_begin] / (double)MPI_nprocs;
	//}
}

// after this, only the processor's chunk of radiation quantities
// is correct, but all gas quantities are correct.
void Transport::synchronize_gas()
{
	if(verbose && rank0) cout << "# Synchronizing Gas" << endl;
	vector<double> buffer;
	int my_begin, my_end, size;

	//-- EACH PROCESSOR SENDS THE GRID INFORMATION IT SOLVED
	for(int proc=0; proc<MPI_nprocs; proc++){

		// set the begin and end indices so a process covers range [begin,end)
		my_begin = ( proc==0 ? 0 : my_zone_end[proc-1] );
		my_end = my_zone_end[proc];

		// set the computation size and create the send/receive vectors
		size = my_end - my_begin;
		buffer.resize(size);

		// broadcast T_gas
		if(equilibrium_T){
			if(proc==MPI_myID) for(int i=my_begin; i<my_end; i++) buffer[i-my_begin] = grid->z[i].T;
			MPI_Bcast(&buffer.front(), size, MPI_DOUBLE, proc, MPI_COMM_WORLD);
			if(proc!=MPI_myID) for(int i=my_begin; i<my_end; i++) grid->z[i].T = buffer[i-my_begin];
		}

		// broadcast Ye
		if(equilibrium_Ye){
			if(proc==MPI_myID) for(int i=my_begin; i<my_end; i++) buffer[i-my_begin] = grid->z[i].Ye;
			MPI_Bcast(&buffer.front(), size, MPI_DOUBLE, proc, MPI_COMM_WORLD);
			if(proc!=MPI_myID) for(int i=my_begin; i<my_end; i++) grid->z[i].Ye = buffer[i-my_begin];
		}

		// broadcast Q_annihil
		if(do_annihilation){
			if(proc==MPI_myID) for(int i=my_begin; i<my_end; i++) buffer[i-my_begin] = grid->z[i].Q_annihil;
			MPI_Bcast(&buffer.front(), size, MPI_DOUBLE, proc, MPI_COMM_WORLD);
			if(proc!=MPI_myID) for(int i=my_begin; i<my_end; i++) grid->z[i].Q_annihil = buffer[i-my_begin];
		}
	}
}


// rate at which viscosity energizes the fluid (erg/s)
double Transport::zone_comoving_visc_heat_rate(const int z_ind) const{
	if(visc_specific_heat_rate >= 0) return visc_specific_heat_rate * grid->z[z_ind].rho * grid->zone_comoving_volume(z_ind);
	else                             return grid->z[z_ind].H_vis    * grid->z[z_ind].rho * grid->zone_comoving_volume(z_ind);
}


// update zone quantities based on heat capacity and lepton capacity
void Transport::update_zone_quantities(){
	if(verbose && rank0) cout << "# Updating Zone Quantities" << endl;
	// remember what zones I'm responsible for
	int start = ( MPI_myID==0 ? 0 : my_zone_end[MPI_myID - 1] );
	int end = my_zone_end[MPI_myID];

	// solve radiative equilibrium temperature and Ye (but only in the zones I'm responsible for)
	// don't solve if out of density bounds
    #pragma omp parallel for schedule(guided)
	for (int i=start; i<end; i++) if( (grid->z[i].rho >= rho_min) && (grid->z[i].rho <= rho_max) )
	{
		Zone *z = &(grid->z[i]);

		// adjust the temperature based on the heat capacity (erg/K)
		if(equilibrium_T){
			// PRINT_ASSERT(z.heat_cap,>,0);
			// z.T_gas += (z->e_abs-z->e_emit) / z->heat_cap;
			// PRINT_ASSERT(z->T_gas,>=,T_min);
			// PRINT_ASSERT(z->T_gas,<=,T_max);
		}

		// adjust the Ye based on the lepton capacity (number of leptons)
		if(equilibrium_Ye){
			double Nbary = grid->zone_rest_mass(i) / mean_mass(i);
			PRINT_ASSERT(Nbary,>,0);
			z->Ye += (z->nue_abs-z->anue_abs - z->l_emit) / Nbary;
			PRINT_ASSERT(z->Ye,>=,Ye_min);
			PRINT_ASSERT(z->Ye,<=,Ye_max);
		}
	}
}

string Transport::filename(const char* filebase, const int iw, const char* suffix){
	string number_string;
	stringstream iwstream;
	iwstream << iw;
	if     (iw < 10)    number_string = "0000" + iwstream.str();
	else if(iw < 100)   number_string = "000"  + iwstream.str();
	else if(iw < 1000)  number_string = "00"   + iwstream.str();
	else if(iw < 10000) number_string = "0"    + iwstream.str();
	else                number_string =          iwstream.str();

	string filename = string(filebase) + "_" + number_string + suffix;
	return filename;
}

double Transport::mean_mass(const double Ye){
	return 1.0 / (Ye/pc::m_p + (1.0-Ye)/pc::m_n);
}

int Transport::number_of_bins() const{
	int number_energy_bins = 0;
	for(unsigned s = 0; s<species_list.size(); s++) number_energy_bins += species_list[s]->number_of_bins();
	return number_energy_bins;
}

double Transport::importance(const double abs_opac, const double scat_opac, const double dx) const{
	if(importance_bias<=0) return 1.0;
	double taubar = (abs_opac + scat_opac) * dx;
	double result = (importance_bias*taubar<=1.0 ? 1.0 : exp(1.0 - importance_bias * taubar));
	return max(result,min_importance);
}