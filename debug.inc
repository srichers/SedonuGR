#Only enable the commented compatability flags if compiling/running fails. Some systems (e.g. Hopper) won't compile with some of them enabled.

export
#general options
TARGET=$(CURDIR)/gomc
BASEDIR=$(CURDIR)

#compiler configuration
F90=mpif90
CXX=mpicxx
CC=mpicc

F90FLAGS= -O3 -openmp
CXXFLAGS= -O0 -g -lifcore #-openmp
CCFLAGS= -O0 -g

#GSL
GSL_BASE=$(GSL_DIR)
GSL_INCL=-I$(GSL_BASE)/include
GSL_LIBS=-L$(GSL_BASE)/lib -lgsl -lgslcblas

#LUA
#LUA_BASE=$(CURDIR)/external/lua-5.2.2/install
LUA_INCL=-I$(LUA_BASE)/include
LUA_LIBS=-L$(LUA_BASE)/lib -llua5.2 -Wl,-rpath,$(LUA_BASE)/lib

#HDF5
HDF5DIR=$(CURDIR)/external/hdf5_intel
HDF5INCS=-I$(HDF5DIR)/include
HDF5LIBS=-L$(HDF5DIR)/lib -lhdf5 -lhdf5_fortran -Wl,-rpath,$(HDF5DIR)/lib

#NuLib
NULIB_DIR=$(CURDIR)/external/NuLib
NULIB_LIBS=$(NULIB_DIR)/src/nulib.a #GNU: -lgfortran

#OTHER
OTHER_LIBS= #PGI: -ldl