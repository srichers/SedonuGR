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
#include <algorithm>
#include <stdio.h>
#include <cmath>
#include "cdf_array.h"
#include "locate_array.h"


// safe constructor
cdf_array::cdf_array(const int iorder){
	N = NaN;
	assert(iorder==1 || iorder==3);
	interpolation_order = iorder;
}

//------------------------------------------------------
// return the actual y value, not the integrated
//------------------------------------------------------
double cdf_array::get_value(const int i) const
{
	assert(i >= 0);
	assert(i < (int)size());
	if (i==0) return y[0];
	else return (y[i] - y[i-1]);
}





//------------------------------------------------------
// set the actual y value, not the integrated
// must be called in order
//------------------------------------------------------
void cdf_array::set_value(const int i, const double f)
{
	assert(i >= 0);
	assert(i < (int)size());
	y[i] = ( i==0 ? f : y[i-1]+f );
}

//------------------------------------------------------
// Normalize such that the last entry is 1.0
//------------------------------------------------------
void cdf_array::normalize(double cutoff)
{
	// check for zero array, set to all constant
	if (y.back() == 0){
		y.assign(y.size(),1.0);
		N = 0;
	}
	// normalize to end = 1.0
	else{
		N = y.back();
		assert(N > 0);
		for(unsigned i=0;i<y.size();i++)   y[i] /= ( N>0 ? N : 1 );
	}

	// set values below cutoff to 0
	if(cutoff>0){
		for(unsigned i=0; i<y.size(); i++){
			double val = get_value(i);
			if(val < cutoff) set_value(i,0);
			else set_value(i,val);
		}
		double tmp=N;
		normalize();
		N *= tmp;
	}
}

//---------------------------------------------------------
// Sample the probability distribution using binary search.
// Pass a number betwen 0 and 1.
// Returns the index of the first value larger than yval
// if larger than largest element, returns size
//---------------------------------------------------------
int cdf_array::get_index(const double yval) const
{
	assert(yval >= 0);
	assert(yval <= 1.0);
	int i = upper_bound(y.begin(), y.end(), yval) - y.begin();
	assert(i >= 0);
	assert(i < (int)size());
	return i;
}

//-----------------------------------------------------
// return centered tangent at the corresponding vertex.
// i==-1 means at the left boundary
// assumes the cdf is monotonic
//-----------------------------------------------------
double cdf_array::inverse_tangent(const int i, const locate_array* xgrid) const{
	int N = size();
	assert(i>=-1);
	assert(i<=(int)size()-1);

	// two-point stencil on boundary, 3-point stencil elsewhere. Keep out infinities.
	double result = 0.0;
	double left_secant  = (i==-1  ? numeric_limits<double>::infinity() : inverse_secant(i-1,i,xgrid));
	double right_secant = (i==N-1 ? numeric_limits<double>::infinity() : inverse_secant(i,i+1,xgrid));
	assert(!isinf<bool>(left_secant) || !isinf<bool>(right_secant));
	if     (i==-1 ) result = right_secant;
	else if(i==N-1) result = left_secant;
	else{
		if     (isinf<bool>(left_secant )) result = right_secant;
		else if(isinf<bool>(right_secant)) result = left_secant;
		else                         result = 0.5 * (right_secant + left_secant);
	}
	return result;
}
double cdf_array::tangent(const int i, const locate_array* xgrid) const{
	int N = size();
	assert(i>=-1);
	assert(i<=(int)size()-1);

	// two-point stencil on boundary, 3-point stencil elsewhere. Keep out infinities.
	double result = 0.0;
	double left_secant  = (i==-1  ? numeric_limits<double>::infinity() : secant(i-1,i,xgrid));
	double right_secant = (i==N-1 ? numeric_limits<double>::infinity() : secant(i,i+1,xgrid));
	assert(!isinf<bool>(left_secant) || !isinf<bool>(right_secant));
	if     (i==-1 ) result = right_secant;
	else if(i==N-1) result = left_secant;
	else{
		if     (isinf<bool>(left_secant )) result = right_secant;
		else if(isinf<bool>(right_secant)) result = left_secant;
		else                         result = 0.5 * (right_secant + left_secant);
	}
	return result;
}
//--------------------------------------
// return secant line between two points
// (CDF is x value, xgrid is y value)
//--------------------------------------
double cdf_array::inverse_secant(const int i, const int j, const locate_array* xgrid) const{
	return 1.0 / secant(i,j,xgrid);
}
double cdf_array::secant(const int i, const int j, const locate_array* xgrid) const{
	assert(i>=-1 && i<(int)size()-1);
	assert(j>=0  && j<(int)size()  );
	assert(j>i);

	double result = 0.0;
	if(i==-1) result = (y[j]-0.0 ) / ((*xgrid)[j]-xgrid->min );
	else      result = (y[j]-y[i]) / ((*xgrid)[j]-(*xgrid)[i]);
	return result;
}

//---------------------------------------------------------
// Sample the probability distribution using binary search.
// Pass a random number betwen 0 and 1.
// Returns the corresponding cubic-interpolated xgrid value
// assumes the CDF is monotonic
//---------------------------------------------------------
// first, the cubic hermit spline functions
double h00(const double t){
	assert(0.0<=t && t<=1.0);
	return (1.0+2.0*t)*(1.0-t)*(1.0-t);
}
double h10(const double t){
	assert(0<=t && t<=1.0);
	return t*(1.0-t)*(1.0-t);
}
double h01(const double t){
	assert(0.0<=t && t<=1.0);
	return t*t*(3.0-2.0*t);
}
double h11(const double t){
	assert(0.0<=t && t<=1.0);
	return t*t*(t-1.0);
}
double cdf_array::invert(const double rand, const locate_array* xgrid, const int i_in) const{
	double result = 0;
	assert(interpolation_order==1 || interpolation_order==3 || interpolation_order==0);
	if     (interpolation_order==0) result = invert_piecewise(rand,xgrid,i_in);
	else if(interpolation_order==1) result = invert_linear(rand,xgrid,i_in);
	else if(interpolation_order==3) result = invert_cubic(rand,xgrid,i_in);
	assert(result>0);
	return result;
}
double cdf_array::interpolate(const double x, const locate_array* xgrid) const
{
	double result = 0;
	assert(interpolation_order==1 || interpolation_order==3 || interpolation_order==0);
	if     (interpolation_order==0) result = interpolate_piecewise(x,xgrid);
	else if(interpolation_order==1) result = interpolate_linear(x,xgrid);
	else if(interpolation_order==3) result = interpolate_cubic(x,xgrid);
	assert(result>0);
	return result;
}
double cdf_array::invert_cubic(const double rand, const locate_array* xgrid, const int i_in) const
// INCONSISTENCY - the emissivity is integrated assuming piecewise constant.
// This is inconsistent with cubic interpolation.
{
	assert(rand>=0 && rand<=1);
	int i = (i_in<0 ? get_index(rand) : i_in);
	assert(i<(int)size());
	assert(i>=0);

	// check for degenerate case (left and right values are equal)
	double yRight = y[i];
	double xRight = (*xgrid)[i];
	double yLeft = (i>0 ?        y[i-1] : 0         );
	double xLeft = (i>0 ? (*xgrid)[i-1] : xgrid->min);
	if(yRight == yLeft) return yRight;

	// get left and right tangents
	double mLeft  = inverse_tangent(i-1,xgrid);
	double mRight = inverse_tangent(i  ,xgrid);
	assert(!isinf<bool>(mLeft ));
	assert(!isinf<bool>(mRight));

	// prevent overshoot, ensure monotonicity
	double slope = inverse_secant(i-1,i,xgrid);
	double limiter = 3.0;
	double alpha = mLeft/slope;
	double beta = mRight/slope;
	assert(alpha>0);
	assert(beta>0);
	if(alpha*alpha + beta*beta > limiter*limiter){
		double tau = limiter/sqrt(alpha*alpha+beta*beta);
		mLeft = tau*alpha*slope;
		mRight = tau*beta*slope;
	}

	// return interpolated function
	double h = yRight-yLeft;
	double t = 0;
	if(i_in<0) t = (rand-yLeft)/h;
	else t = rand*(yRight-yLeft)/h;
	assert(t>=0 && t<=1);
	double result = xLeft*h00(t) + h*mLeft*h10(t) + xRight*h01(t) + h*mRight*h11(t);
	result = max(xLeft,result);
	result = min(xRight,result);
	assert(xLeft<=result);
	assert(xRight>=result);
	return result;
}
double cdf_array::interpolate_cubic(const double x, const locate_array* xgrid) const
{
	// get the upper index
	int i = xgrid->locate(x);
	assert(i<(int)size());
	assert(i>=0);

	// check for degenerate case (left and right values are equal)
	double yRight = y[i];
	double xRight = (*xgrid)[i];
	double yLeft = (i>0 ?        y[i-1] : 0         );
	double xLeft = (i>0 ? (*xgrid)[i-1] : xgrid->min);
	if(yRight == yLeft) return yRight;

	// get left and right tangents
	double mLeft  = tangent(i-1,xgrid);
	double mRight = tangent(i  ,xgrid);
	assert(!isinf<bool>(mLeft ));
	assert(!isinf<bool>(mRight));

	// prevent overshoot, ensure monotonicity
	double slope = inverse_secant(i-1,i,xgrid);
	double limiter = 3.0;
	double alpha = mLeft/slope;
	double beta = mRight/slope;
	assert(alpha>0);
	assert(beta>0);
	if(alpha*alpha + beta*beta > limiter*limiter){
		double tau = limiter/sqrt(alpha*alpha+beta*beta);
		mLeft = tau*alpha*slope;
		mRight = tau*beta*slope;
	}

	// return interpolated function
	double h = xRight-xLeft;
	double t = (x-xLeft)/h;
	assert(t>=0 && t<=1);
	double result = yLeft*h00(t) + h*mLeft*h10(t) + yRight*h01(t) + h*mRight*h11(t);
	result = max(yLeft,result);
	result = min(yRight,result);
	assert(yLeft<=result);
	assert(yRight>=result);
	return result;
}
double cdf_array::interpolate_linear(const double x, const locate_array* xgrid) const
{
	// get the upper/lower indices
	int upper = xgrid->locate(x);
	assert(upper>=0);
	int lower = upper-1;
	assert(lower<xgrid->size());

	// get the x and y values of the left and right sides
	double x1,x2,y1,y2;
	if(upper==0){
		x1 = xgrid->min;
		x2 = (*xgrid)[0];
		y1 = 0;
		y2 = y[0];
	}
	else{
		x1 = (*xgrid)[lower];
		x2 = (*xgrid)[upper];
		y1 = y[lower];
		y2 = y[upper];
	}

	double slope = (y2-y1)/(x2-x1);
	double result = y1 + slope*(x-x1);
	assert(result <= (*xgrid)[xgrid->size()-1]);
	assert(result >= xgrid->min);
	return result;
}
double cdf_array::invert_linear(const double rand, const locate_array* xgrid, const int i_in) const
{
	assert(rand>=0 && rand<=1);
	int i = (i_in<0 ? get_index(rand) : i_in);
	assert(i<(int)size());
	assert(i>=0);

	// check for degenerate case (left and right values are equal)
	double xLeft = (i>0 ? (*xgrid)[i-1] : xgrid->min);
	double yLeft = (i>0 ?        y[i-1] : 0         );
	double yRight = y[i];
	double xRight = (*xgrid)[i];
	if(yRight == yLeft) return yRight;

	// get the slope between the two adjacent points
	double slope = inverse_secant(i-1,i,xgrid);
	assert(!isinf<bool>(slope));

	// return interpolated function
	double result = 0;
	if(i_in<0) result = xLeft + slope*(rand-yLeft);
	else result = xLeft + rand*slope*(yRight-yLeft);
	assert(xLeft<=result);
	assert(xRight>=result);
	return result;
}
double cdf_array::invert_piecewise(const double rand, const locate_array* xgrid, const int i_in) const
{
	assert(rand>=0 && rand<=1);
	int i = (i_in<0 ? get_index(rand) : i_in);
	assert(i<(int)size());
	assert(i>=0);

	return xgrid->center(i);
}
double cdf_array::interpolate_piecewise(const double x, const locate_array* xgrid) const
{
	if(x<(*xgrid)[0]) return 0.0;

	int upper = xgrid->locate(x);
	assert(upper>=0);
	int lower = upper-1;
	assert(lower<xgrid->size());

	return y[lower];
}

//------------------------------------------------------
// Simple printout
//------------------------------------------------------
void cdf_array::print() const{
	for(unsigned i=0;i<y.size();i++)
		printf("%5d %10.4e %10.4e\n",i,get_value(i),y[i]);
}

//------------------------------------------------------
// Clear the arrays
//------------------------------------------------------
void cdf_array::wipe()
{
	y.assign(y.size(), 1.0);
}

//------------------------------------------------------------
// just returning the size of the array
//------------------------------------------------------------
unsigned cdf_array::size() const
{
	return y.size();
}
