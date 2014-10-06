MeV = 1.1605e10 # Kelvin
km = 1.0e5      # cm

nx = 1

R_max = 10*km
R_min = 9.99999*km
dx = (R_max-R_min)/nx

rho = 3.16227766017e+11
temp = 1*MeV
ye = 0.3

print '1D_sphere',nx,R_min

for i  in range(1,nx+1):
    R = R_min + i*dx
    print R, rho, temp, ye
