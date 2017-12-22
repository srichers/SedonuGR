#ifndef _MULTIDARRAY_H
#define _MULTIDARRAY_H 1

#include <vector>
#include "global_options.h"
#include "Axis.h"
#include "mpi.h"

using namespace std;

class MultiDInterface{
public:
	vector<double> y0;
	vector<Axis> axes;

	virtual ~MultiDInterface() {}
	virtual double get(const unsigned int ind[]) const=0;
	virtual void set(const unsigned int ind[], const double y)=0;
	virtual double interpolate(const double x[], const unsigned int ind[]) const=0;
	virtual void calculate_slopes()=0;
	virtual void wipe()=0;
	virtual unsigned size() const= 0;
	virtual unsigned Ndims() const= 0;
	virtual unsigned direct_index(const unsigned ind[]) const = 0;
	virtual void add(const unsigned ind[], const double to_add) = 0;
	virtual void direct_add(const unsigned lin_ind, const double to_add) = 0;
	virtual void MPI_combine() = 0;
	virtual void MPI_AllCombine() = 0;
	virtual void write_HDF5(H5::H5File file, const string name) = 0;
};



//=====================//
// INTERPOLATION ARRAY //
//=====================//
template<unsigned int ndims>
class MultiDArray : public MultiDInterface{
public:

	vector<Tuple<double,ndims> > dydx;
	Tuple<unsigned int,ndims> stride;

	MultiDArray(const vector<Axis>& axes){
		this->axes = axes;
		assert(axes.size()==ndims);
		int size = 1;
		for(int i=ndims-1; i>=0; i--){
			stride[i] = size;
			size *= axes[i].size();
		}
		y0.resize(size);
	}


	unsigned direct_index(const unsigned ind[ndims]) const{
		int result = 0;
		for(int i=0; i<ndims; i++){
			PRINT_ASSERT(ind[i],<,axes[i].size());
			result += ind[i]*stride[i];
		}
		PRINT_ASSERT(result,<,y0.size());
		return result;
	}
	void indices(const int z_ind, unsigned ind[ndims]) const{
		PRINT_ASSERT(z_ind,<,y0.size());
		for(int i=0; i<ndims; i++){
			ind[i] = z_ind % stride[i];
			PRINT_ASSERT(ind[i],<,axes[i].size());
		}
	}

	// get center value based on grid index
	double get(const unsigned int ind[ndims]) const{
		return y0[direct_index(ind)];
	}
	void set(const unsigned int ind[ndims], const double y){
		y0[direct_index(ind)] = y;
	}

	// get interpolated value
	double interpolate(const double x[ndims], const unsigned int ind[ndims]) const{
		unsigned int z_ind = direct_index(ind);
		double result = y0[z_ind];
		if(dydx.size()>0) for(int i=0; i<ndims; i++)
			result += dydx[z_ind][i] * (x[i] - axes[i].mid[ind[i]]);
		return result;
	}

	// set the slopes
	void calculate_slopes(){
		unsigned int ind[ndims], indp[ndims], indm[ndims];
		unsigned int zp, zm;
		double x, xp, xm;
		double y, yp, ym;
		double dxL, dxR;
		double dyL, dyR;
		double sL, sR;
		double slope;

		dydx.resize(y0.size());
		for(unsigned int z=0; z<dydx.size(); z++){
			indices(z, ind);
			for(unsigned int i=0; i<ndims; i++){

				// get the index for the plus and minus values
				for(unsigned int j=0; j<ndims; j++){
					indp[j] = ind[j];
					indm[j] = ind[j];
				}
				indp[i]++;
				indm[i]--;
				zp = direct_index(indp);
				zm = direct_index(indm);

				// get plus and minus values
				y=y0[z];
				x=axes[i].mid[ind[i]];
				if(ind[i] > 0){
					xm = axes[i].mid[indm[i]];
					ym = y0[zm];
					dxL = x-xm;
					dyL = y-ym;
					sL = dyL/dxL;
				}


				// get the actual slope
				if(ind[i]==0) slope = sR;
				else if(ind[i]==axes[i].size()-1) slope = sL;
				else slope = (dxR*sL + dxL*sR) / (dxR+dxL);
				dydx[z][i] = slope;
			}
		}
	}

	void wipe(){
		for(unsigned z=0; z<y0.size(); z++)
			y0[z] = 0;
		for(unsigned z=0; z<dydx.size(); z++)
			for(unsigned i=0; i<ndims; i++)
				dydx[z][i] = 0;

	}

	unsigned size() const{
		return y0.size();
	}

	unsigned Ndims() const{
		return ndims;
	}

	void add(const unsigned ind[ndims], const double to_add){
		unsigned lin_ind = direct_index(ind);
		direct_add(lin_ind, to_add);
	}
	void direct_add(const unsigned lin_ind, const double to_add){
		PRINT_ASSERT(lin_ind,<,y0.size());
		PRINT_ASSERT(y0[lin_ind],>=,0);
		PRINT_ASSERT(to_add,<,INFINITY);
		PRINT_ASSERT(y0[lin_ind],<,INFINITY);

		#pragma omp atomic
		y0[lin_ind] += to_add;
	}

	void MPI_combine()
	{
		int MPI_myID;
		MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID);
		MPI_Request request;
		const int tag = 0;

		// average the flux (receive goes out of scope after section)
		if(MPI_myID==0) MPI_Reduce(MPI_IN_PLACE, &y0.front(), size(), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
		else            MPI_Reduce(&y0.front(),  &y0.front(), size(), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	}

	void MPI_AllCombine()
	{
		int MPI_myID;
		MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID);
		MPI_Request request;
		const int tag = 0;

		// average the flux (receive goes out of scope after section)
		MPI_Allreduce(MPI_IN_PLACE, &y0.front(), size(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	}

	void write_HDF5(H5::H5File file, const string name) {
		hsize_t dims[ndims];
		for(unsigned i=0; i<ndims; i++) dims[i] = axes[i].size(); // number of bins
		H5::DataSpace dataspace(ndims,dims);
		H5::DataSet dataset = file.createDataSet(name+"(erg|ccm,lab)",H5::PredType::IEEE_F64LE,dataspace);

		// write the data (converting to single precision)
		// assumes phi increases fastest, then mu, then nu
		dataset.write(&y0.front(), H5::PredType::IEEE_F64LE);
		dataset.close();
	}
};



#endif
