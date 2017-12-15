#include <REAL.H>
#include <cmath>
#include <Box.H>
#include "CONSTANTS.H"
#include "MG_F.H"
#include <ArrayLim.H>
#include <iostream>

#include "RAJA/RAJA.hpp"

//ask Brian CONSTANTS
//MultiGrid.cpp

//Average Kernel
void C_AVERAGE(const Box& bx,
const int nc,
FArrayBox& c,
const FArrayBox& f){
	
	const int *lo = bx.loVect();
	const int *hi = bx.hiVect();

        RAJA::RangeSegment iBounds(lo[0], hi[0]+1);
        RAJA::RangeSegment jBounds(lo[1], hi[1]+1);
        RAJA::RangeSegment kBounds(lo[2], hi[2]+1);
        RAJA::RangeSegment nBounds(0, nc);

        // Since we are modifying the FAB "c" inside the loop, we need to do
        // the lambda capture by reference using [&] rather than [=].
        RAJA::forallN<RAJA::NestedPolicy<
          RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
            iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {

              int i2 = 2*i;
              int j2 = 2*j;
              int k2 = 2*k;

              c(IntVect(i,j,k),n) =  (f(IntVect(i2+1,j2+1,k2),n) + f(IntVect(i2,j2+1,k2),n) + f(IntVect(i2+1,j2,k2),n) + f(IntVect(i2,j2,k2),n))*0.125;
              c(IntVect(i,j,k),n) += (f(IntVect(i2+1,j2+1,k2+1),n) + f(IntVect(i2,j2+1,k2+1),n) + f(IntVect(i2+1,j2,k2+1),n) + f(IntVect(i2,j2,k2+1),n))*0.125;

	});
}


//Interpolation Kernel
void C_INTERP(const Box& bx,
const int nc,
FArrayBox& f,
const FArrayBox& c){
	
	const int *lo = bx.loVect();
	const int *hi = bx.hiVect();

        RAJA::RangeSegment iBounds(lo[0], hi[0]+1);
        RAJA::RangeSegment jBounds(lo[1], hi[1]+1);
        RAJA::RangeSegment kBounds(lo[2], hi[2]+1);
        RAJA::RangeSegment nBounds(0, nc);

        // Since we are modifying the FAB "f" inside the loop, we need to do
        // the lambda capture by reference using [&] rather than [=].
        RAJA::forallN<RAJA::NestedPolicy<
          RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
            iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {

              int i2 = 2*i;
              int j2 = 2*j;
              int k2 = 2*k;

              f(IntVect(i2+1,j2+1,k2  ),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2  ,j2+1,k2  ),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2+1,j2  ,k2  ),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2  ,j2  ,k2  ),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2+1,j2+1,k2+1),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2  ,j2+1,k2+1),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2+1,j2  ,k2+1),n)       += c(IntVect(i,j,k),n);
              f(IntVect(i2  ,j2  ,k2+1),n)       += c(IntVect(i,j,k),n);

           });
}


//-----------------------------------------------------------------------
//      
//     Gauss-Seidel Red-Black (GSRB):
//     Apply the GSRB relaxation to the state phi for the equation
//     L(phi) = alpha*a(x)*phi(x) - beta*Div(b(x)Grad(phi(x))) = rhs(x)
//     central differenced, according to the arrays of boundary
//     masks (m#) and auxiliary data (f#).
//     
//     In general, if the linear operator L=gamma*y-rho, the GS relaxation
//     is y = (R - rho)/gamma.  Near a boundary, the ghost data is filled
//     using a polynomial interpolant based on the "old" phi values, so
//     L=(gamma-delta)*y - rho + delta*yOld.  The resulting iteration is
//     
//     y = (R - delta*yOld + rho)/(gamma - delta)
//     
//     This expression is valid additionally in the interior provided
//     delta->0 there.  delta is constructed by summing all the
//     contributions to the central stencil element coming from boundary 
//     interpolants.  The f#s contain the corresponding coefficient of 
//     the interpolating polynomial.  The masks are set > 0 if the boundary 
//     value was filled with an interpolant involving the central stencil 
//     element.
//     
//-----------------------------------------------------------------------
void C_GSRB_3D(
const Box& bx,
const Box& bbx,
const int nc,
const int rb,
const Real alpha,
const Real beta,
FArrayBox& phi,
const FArrayBox& rhs,
const FArrayBox& a,
const FArrayBox& bX,
const FArrayBox& bY,
const FArrayBox& bZ,
const FArrayBox& f0,
const Mask& m0,
const FArrayBox& f1,
const Mask& m1,
const FArrayBox& f2,
const Mask& m2,
const FArrayBox& f3,
const Mask& m3,
const FArrayBox& f4,
const Mask& m4,
const FArrayBox& f5,
const Mask& m5,
const Real* h)
{
	//box extends:
	const int *lo = bx.loVect();
	const int *hi = bx.hiVect();
	//blo
	const int *blo = bbx.loVect();
	const int *bhi = bbx.hiVect();
	    
	//some parameters
	Real omega= 1.15;
	Real dhx = beta/(h[0]*h[0]);
	Real dhy = beta/(h[1]*h[1]);
	Real dhz = beta/(h[2]*h[2]);

        RAJA::StaticIndexSet<RAJA::TypedListSegment<RAJA::Index_type>> iBounds;
        auto *idx = new RAJA::Index_type((hi[0]-lo[0])/2+1);

        RAJA::RangeSegment jBounds(lo[1], hi[1]+1);
        RAJA::RangeSegment kBounds(lo[2], hi[2]+1);
        RAJA::RangeSegment nBounds(0, nc);

        RAJA::StaticIndexSet<RAJA::TypedListSegment<RAJA::Index_type>> colorset;


	// This is technically just Gauss-Seidel, not the red-black variation.
	// The index set for i depends on the values of j and k; I don't know
	// how to capture that without pre-computing unique index sets for i,
	// given all combinations of j and k. There must be a simpler way to do
	// it than that, because ultimately the i loop is starting either at
	// lo[0] or lo[0]+1.

        RAJA::forallN<RAJA::NestedPolicy<
          RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
            jBounds, kBounds, nBounds, [&](int j, int k, int n) {

            int count = 0;
            int ioff = (lo[0] + j + k + rb)%2;
            for (int i = lo[0] + ioff; i <= hi[0]; i+=2) {
              idx[count] = i;
              std::cout << count << " " << " " << idx[count] << std::endl;
              ++count;
            }
            iBounds.push_back(RAJA::ListSegment(idx, count));

            RAJA::forall<RAJA::seq_exec> (iBounds, [&](int i) {


                 //BC terms
                 Real cf0 = ( (i==blo[0]) && (m0(IntVect(blo[0]-1,j,k))>0) ? f0(IntVect(blo[0],j,k)) : 0. );
                 Real cf1 = ( (j==blo[1]) && (m1(IntVect(i,blo[1]-1,k))>0) ? f1(IntVect(i,blo[1],k)) : 0. );
                 Real cf2 = ( (k==blo[2]) && (m2(IntVect(i,j,blo[2]-1))>0) ? f2(IntVect(i,j,blo[2])) : 0. );
                 Real cf3 = ( (i==bhi[0]) && (m3(IntVect(bhi[0]+1,j,k))>0) ? f3(IntVect(bhi[0],j,k)) : 0. );
                 Real cf4 = ( (j==bhi[1]) && (m4(IntVect(i,bhi[1]+1,k))>0) ? f4(IntVect(i,bhi[1],k)) : 0. );
                 Real cf5 = ( (k==bhi[2]) && (m5(IntVect(i,j,bhi[2]+1))>0) ? f5(IntVect(i,j,bhi[2])) : 0. );

                 //assign ORA constants
                 double gamma = alpha * a(IntVect(i,j,k))
                              + dhx * (bX(IntVect(i,j,k)) + bX(IntVect(i+1,j,k)))
                              + dhy * (bY(IntVect(i,j,k)) + bY(IntVect(i,j+1,k)))
                              + dhz * (bZ(IntVect(i,j,k)) + bZ(IntVect(i,j,k+1)));

                 double g_m_d = gamma
                              - dhx * (bX(IntVect(i,j,k))*cf0 + bX(IntVect(i+1,j,k))*cf3)
                              - dhy * (bY(IntVect(i,j,k))*cf1 + bY(IntVect(i,j+1,k))*cf4)
                              - dhz * (bZ(IntVect(i,j,k))*cf2 + bZ(IntVect(i,j,k+1))*cf5);

                 double rho =  dhx * (bX(IntVect(i,j,k))*phi(IntVect(i-1,j,k),n) + bX(IntVect(i+1,j,k))*phi(IntVect(i+1,j,k),n))
                            + dhy * (bY(IntVect(i,j,k))*phi(IntVect(i,j-1,k),n) + bY(IntVect(i,j+1,k))*phi(IntVect(i,j+1,k),n))
                            + dhz * (bZ(IntVect(i,j,k))*phi(IntVect(i,j,k-1),n) + bZ(IntVect(i,j,k+1))*phi(IntVect(i,j,k+1),n));

                 double res = rhs(IntVect(i,j,k),n) - gamma * phi(IntVect(i,j,k),n) + rho;

                 phi(IntVect(i,j,k),n) += omega/g_m_d * res;
          });

        });
}

//-----------------------------------------------------------------------
//
//     Fill in a matrix x vector operator here
//
void C_ADOTX(
const Box& bx,
const int nc,
FArrayBox& y,
const FArrayBox& x,
Real alpha,
Real beta,
const FArrayBox& a,
const FArrayBox& bX,
const FArrayBox& bY,
const FArrayBox& bZ,
const Real* h)
{

	//box extends:
	const int *lo = bx.loVect();
	const int *hi = bx.hiVect();
	
	//some parameters
	Real dhx = beta/(h[0]*h[0]);
	Real dhy = beta/(h[1]*h[1]);
	Real dhz = beta/(h[2]*h[2]);

        RAJA::RangeSegment iBounds(lo[0], hi[0]+1);
        RAJA::RangeSegment jBounds(lo[1], hi[1]+1);
        RAJA::RangeSegment kBounds(lo[2], hi[2]+1);
        RAJA::RangeSegment nBounds(0, nc);

        // Since we are modifying the FAB "y" inside the loop, we need to do
        // the lambda capture by reference using [&] rather than [=].
        RAJA::forallN<RAJA::NestedPolicy<
          RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
            iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {

                y(IntVect(i,j,k),n) = alpha * a(IntVect(i,j,k)) * x(IntVect(i,j,k),n)
                - dhx * (   bX(IntVect(i+1,j,  k  )) * ( x(IntVect(i+1,j,  k),  n) - x(IntVect(i,  j,  k  ),n) )
                - bX(IntVect(i,  j,  k  )) * ( x(IntVect(i,  j,  k),  n) - x(IntVect(i-1,j,  k  ),n) )
                )
                - dhy * (   bY(IntVect(i,  j+1,k  )) * ( x(IntVect(i,  j+1,k),  n) - x(IntVect(i,  j  ,k  ),n) )
                - bY(IntVect(i,  j,  k  )) * ( x(IntVect(i,  j,  k),  n) - x(IntVect(i,  j-1,k  ),n) )
                )
                - dhz * (   bZ(IntVect(i,  j,  k+1)) * ( x(IntVect(i,  j,  k+1),n) - x(IntVect(i,  j  ,k  ),n) )
                - bZ(IntVect(i,  j,  k  )) * ( x(IntVect(i,  j,  k),  n) - x(IntVect(i,  j,  k-1),n) )
                );

        });
}

//-----------------------------------------------------------------------
//
//     Fill in a matrix x vector operator here
//
void C_NORMA(
const Box& bx,
const int nc,
Real& res,
const Real alpha,
const Real beta,
const FArrayBox& a,
const FArrayBox& bX,
const FArrayBox& bY,
const FArrayBox& bZ,
const Real* h)
{

    //box extends:
    const int *lo = bx.loVect();
    const int *hi = bx.hiVect();

    //some parameters
    Real dhx = beta/(h[0]*h[0]);
    Real dhy = beta/(h[1]*h[1]);
    Real dhz = beta/(h[2]*h[2]);
	
    RAJA::ReduceMax<RAJA::seq_reduce, double> maxval(0.0);

    RAJA::RangeSegment iBounds(lo[0], hi[0]+1);
    RAJA::RangeSegment jBounds(lo[1], hi[1]+1);
    RAJA::RangeSegment kBounds(lo[2], hi[2]+1);
    RAJA::RangeSegment nBounds(0, nc);

    RAJA::forallN<RAJA::NestedPolicy<
    RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
      iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {

            Real tmpval = std::abs(alpha * a(IntVect(i,j,k))
                        + dhx * ( bX(IntVect(i+1,j,k)) + bX(IntVect(i,j,k)) )
                        + dhy * ( bY(IntVect(i,j+1,k)) + bY(IntVect(i,j,k)) )
                        + dhz * ( bZ(IntVect(i,j,k+1)) + bZ(IntVect(i,j,k)) ))
                        + std::abs( dhx * bX(IntVect(i+1,j,k)) ) + std::abs( dhx * bX(IntVect(i,j,k)) )
                        + std::abs( dhy * bY(IntVect(i,j+1,k)) ) + std::abs( dhy * bY(IntVect(i,j,k)) )
                        + std::abs( dhz * bZ(IntVect(i,j,k+1)) ) + std::abs( dhz * bZ(IntVect(i,j,k)) );
            maxval.max(tmpval);
      });
    res = maxval;
}

//-----------------------------------------------------------------------
//
//     Fill in fluxes
//
void C_FLUX(
const Box& xbx,
const Box& ybx,
const Box& zbx,
const int nc,
FArrayBox& x,
FArrayBox& xflux,
FArrayBox& yflux,
FArrayBox& zflux,
Real alpha,
Real beta,
const FArrayBox& a,
const FArrayBox& bX,
const FArrayBox& bY,
const FArrayBox& bZ,
const Real* h)
{

	//box extends:
	const int *xlo = xbx.loVect();
	const int *xhi = xbx.hiVect();
	const int *ylo = ybx.loVect();
	const int *yhi = ybx.hiVect();
	const int *zlo = zbx.loVect();
	const int *zhi = zbx.hiVect();
	
	//some parameters
	Real dhx = beta/(h[0]*h[0]);
	Real dhy = beta/(h[1]*h[1]);
	Real dhz = beta/(h[2]*h[2]);

	//fill the fluxes:

        RAJA::RangeSegment nBounds(0, nc);

        {
          RAJA::RangeSegment iBounds(xlo[0], xhi[0]+1);
          RAJA::RangeSegment jBounds(xlo[1], xhi[1]+1);
          RAJA::RangeSegment kBounds(xlo[2], xhi[2]+1);

          RAJA::forallN<RAJA::NestedPolicy<
            RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
              iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {
                xflux(IntVect(i,j,k),n) = - dhx * bX(IntVect(i,j,k))*( x(IntVect(i,j,k),n) - x(IntVect(i-1,j,k),n) );
          });
        }

        //y-flux
        {
          RAJA::RangeSegment iBounds(ylo[0], yhi[0]+1);
          RAJA::RangeSegment jBounds(ylo[1], yhi[1]+1);
          RAJA::RangeSegment kBounds(ylo[2], yhi[2]+1);

          RAJA::forallN<RAJA::NestedPolicy<
            RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
              iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {
                yflux(IntVect(i,j,k),n) = - dhy * bY(IntVect(i,j,k))*( x(IntVect(i,j,k),n) - x(IntVect(i,j-1,k),n) );
          });
        }

        //z-flux
        {
          RAJA::RangeSegment iBounds(zlo[0], zhi[0]+1);
          RAJA::RangeSegment jBounds(zlo[1], zhi[1]+1);
          RAJA::RangeSegment kBounds(zlo[2], zhi[2]+1);

          RAJA::forallN<RAJA::NestedPolicy<
            RAJA::ExecList<RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec, RAJA::seq_exec>>> (
              iBounds, jBounds, kBounds, nBounds, [&](int i, int j, int k, int n) {
                zflux(IntVect(i,j,k),n) = - dhz * bZ(IntVect(i,j,k))*( x(IntVect(i,j,k),n) - x(IntVect(i,j,k-1),n) );
          });
        }
}