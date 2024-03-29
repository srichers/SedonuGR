FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip gfortran build-essential libhdf5-serial-dev openmpi-bin pkg-config libopenmpi-dev openmpi-bin libblas-dev liblapack-dev libpnetcdf-dev git python-is-python3 liblua5.2-dev libgsl-dev
RUN pip3 install numpy matplotlib h5py scipy sympy
ENV USER=jenkins
ENV LOGNAME=jenkins
