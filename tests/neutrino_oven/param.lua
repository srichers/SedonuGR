
-- Included Physics

do_GR         = 0
do_visc       = 0
do_relativity = 0
do_annihilation = 0
do_randomwalk = 0
use_scattering_kernels = 0
reflect_outer = 1

-- Equilibrium Solving

equilibrium_T  = 0
equilibrium_Ye = 0
equilibrium_damping = 0.
equilibrium_itmax = 100
equilibrium_tolerance = 1e-10

-- Opacity and Emissivity

neutrino_type = "grey"
grey_opac  = 0 --1e-2
grey_abs_frac = 1e-2
grey_chempot = 0
nugrid_start = 0
nugrid_stop = 500
nugrid_n = 500

-- Escape Spectra

spec_n_mu       = 1
spec_n_phi      = 1

-- Distribution Function

distribution_type = "Polar"
distribution_nmu = 2
distribution_nphi = 2
distribution_polar_basis = 0

-- Grid and Model

grid_type = "Grid1DSphere"
model_type = "custom"
model_file = "oven.mod"

-- Output

write_zones_every   = 1
write_rays_every    = 1
write_spectra_every = 1
output_zones_distribution = 0
output_hdf5 = 0

-- Particle Creation

max_particles  = 2e7
n_subcycles = 1
do_emit_by_bin = 1
n_emit_core_per_bin    = 100
n_emit_therm_per_bin   = 1
max_time_hours = -1

-- Inner Source

r_core = 1.5e5
T_core = {10}
core_chem_pot = {0}
core_lum_multiplier = {1.0}

-- General Controls

verbose       = 1
max_n_iter =  1
min_step_size = 0.01
max_step_size = 0.4

-- Biasing

min_packet_weight = 0.1

-- Random Walk

randomwalk_max_x = 2
randomwalk_sumN = 1000
randomwalk_npoints = 200
randomwalk_min_optical_depth = 10
randomwalk_absorption_depth_limit = 1.0
randomwalk_interpolation_order = 1
