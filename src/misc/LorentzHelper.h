/*
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

#ifndef _LORENTZ_H
#define _LORENTZ_H
#include "Particle.h"

enum Frame {com,lab};

class LorentzHelper{

private:
	Particle p[2];
	double absopac[2], scatopac[2];
	double dist[2];
	double v[3]; // velocity of the fluid in the lab frame
	bool exponential_decay;

	double lorentz_factor(const double v[3], const int vsize) const;
	double doppler_shift(const double v[3], const double D[3], const int size) const;
	void lorentz_transform_particle(Particle* p, const double v[3], const int vsize) const;

public:
	LorentzHelper(const double v[3], const bool exp_dec);


	//==========//
	// particle //
	//==========//
	int p_s() const;
	double p_tau() const;
	ParticleFate p_fate() const;
	const double* p_D(Frame f, const int size) const;
	const double* p_x(const int size) const;

	Particle particle_copy(const Frame f) const;
	const Particle* particle_readonly(const Frame f) const;

	double p_e (const Frame f) const;
	double p_nu(const Frame f) const;

	template<Frame f>
	void set_p(const Particle* p);

	void scale_p_e(const double e);

	void set_p_tau(const double tau);

	void set_p_x(const double x[3], const int size);

	void set_p_fate(const ParticleFate fate);

	//=========//
	// opacity //
	//=========//
	double abs_fraction() const;
	double net_opac(const Frame f) const;
	double abs_opac(const Frame f) const;
	double scat_opac(const Frame f) const;
	double tau_opac(const Frame f) const;

	template<Frame f>
	void set_opac(const double absopac, const double scatopac);


	//==========//
	// distance //
	//==========//
	double distance(const Frame f) const;

	template<Frame f>
	void set_distance(const double d);

};

#endif