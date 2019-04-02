//---------------------------------------------------------------------
//
//	Program written by Wiphoo (Terng) Methachawalit for
//		Yannix 2018/07/23
//	Copyright Yannix (Thailand) Co., Ltd 2018. All rights reserved
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include <assert.h>
#include <cmath>

//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include "InvertWarpFuncs.h"

//---------------------------------------------------------------------
//	DEFINES AND INLINES
//---------------------------------------------------------------------

//	maximum number of iterations for numerical solutions
#define MAX_NUM_ITERATIONS 100

//---------------------------------------------------------------------
//	GLOBALS
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//	FILE SCOPE FUNCTION PROTOTYPES
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//	FUNCTION BODIES
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS InvertWarpFuncs MEMBER CLASSES
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS InvertWarpFuncs STATIC MEMBERS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS InvertWarpFuncs MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
// InvertWarpFuncs::InvertWarpFuncs()
// {
// 	
// }
// InvertWarpFuncs::~InvertWarpFuncs()
// {
// 	
// }
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------


//	numerically invert this->applyWarp to within
//		a numericalError of this->numericalError
//	THROWS ynxValueException if it isn't able to solve a remove warp position.
//		Example : when something like a downward parabola occurs, 
//		this numerical method will not be able to invert it above the vertex.
//		Therefore, in Nuke lens distortion when r^4 is negative and r is high, you cannot remove warp. 
//		Normally this is not a problem because it typically happens in the overscan area only where r is very high.
Vector2 InvertWarpFuncs::removeWarp( const RollingShutterLensDistortionEngine &lensDistortionEngine,
											const Vector2 &initialRemoveWarpGuessQ,
											const Vector2 &q,
											double numericalError /*= DEFAULT_NUMERICAL_ERROR*/ ) 
									throw( ynxValueException )
{
	
	//	compute sqr of epsilon
	double epsilon = numericalError;
	double epsilonSqr = pow( epsilon, 2.0 );
	
	//	take initial guess of unwarped position
	Vector2 unwarpedQ( initialRemoveWarpGuessQ );
			
	//	compute error
	Vector2 warpedUnwarpedQ( lensDistortionEngine.applyWarp( unwarpedQ ) );
	Vector2 errorV( warpedUnwarpedQ - q );
				
	//	compute square of error
	double sqrError = errorV.sqrnorm();
							
	//	keep looping until the error is less than epsilon
	int iterCount = 0;
	while( sqrError > epsilonSqr )
	{
		//	numerically compute partial derivatives of 
		//		warped pixel when moving in pure x or y
		Vector2 unwarpedQ_dx( unwarpedQ );
		unwarpedQ_dx.x += epsilon;
		Vector2 partialX( lensDistortionEngine.applyWarp( unwarpedQ_dx ) - warpedUnwarpedQ );
		partialX /= epsilon;
		
		Vector2 unwarpedQ_dy( unwarpedQ );
		unwarpedQ_dy.y += epsilon;
		Vector2 partialY( lensDistortionEngine.applyWarp( unwarpedQ_dy ) - warpedUnwarpedQ );
		partialY /= epsilon;

		//	compute (a,b) such that a*partialX+b*partialY
		//		equals %error%
		//	This becomes the solution for how much to offset
		//		unwarpedQ to exactly hit target %q% if
		//		this->applyWarp() were exactly a linear
		//		function in 2D
		const Vector2 &U = partialX,
					&V = partialY,
					&P = errorV;
		double a, b;
		
		if( fabs(U.x) < fabs(V.x) )
			//	solve with V.x in denominator
		{
			double VyOverVx = V.y/V.x;
			
			a = ( P.y - P.x * VyOverVx ) / 
					( U.y - U.x * VyOverVx );
			b = ( P.x - a * U.x ) / V.x;
		}
		else
			//	solve with U.x in denominator
		{
			double UyOverUx = U.y / U.x;
			
			b = ( P.y - P.x * UyOverUx ) / 
					( V.y - V.x * UyOverUx );
			a = ( P.x - b * V.x ) / U.x;
		}
		
#ifdef DEBUG
		{
			//	create vector for approximation of solution
			Vector2 predictedP( a*U + b*V );
		
			//	assert that the above is correct
//			assert( ( predictedP - P ).sqrnorm() < epsilonSqr );
		}
#endif
		
		//	keep looping until improved error is better than current error
		Vector2 improvedUnwarpedQ,
				improvedWarpedUnwarpedQ,
				improvedErrorV;
		double improvedSqrError = HUGE;
		double stepScalar = 1;
		assert( sqrError < HUGE );
//cout << "starting[" << iterCount << "]..." << endl;
//		while( improvedSqrError >= sqrError )
		for( ; ; )
		{
			//	try improving guess
			improvedUnwarpedQ = unwarpedQ;
			improvedUnwarpedQ.x -= a*stepScalar;
			improvedUnwarpedQ.y -= b*stepScalar;
			stepScalar /= 2;
			
			//	compute new error
			try
			{ 
				improvedWarpedUnwarpedQ = lensDistortionEngine.applyWarp( improvedUnwarpedQ ); 
				improvedErrorV = improvedWarpedUnwarpedQ - q;
				improvedSqrError = improvedErrorV.sqrnorm();

				//	check for improvement
				if( improvedSqrError < sqrError )
					//	found an improvement
					break;
			}
			catch( ynxValueException &e )
			//	invalid point, so treat as "worse" and continue
			{}
			
			if( stepScalar < epsilon )
				//	no improvement all the way down to 
				//		a tiny stepScalar... must give
				//		up
				throw ynxValueException( "LensDistortionWarp::removeWarp() : tiny stepScalar, and still no improvement! Giving up!" );
		}
		
		//	update current best guess
		errorV = improvedErrorV;
		sqrError = improvedSqrError;
		unwarpedQ = improvedUnwarpedQ;
		warpedUnwarpedQ = improvedWarpedUnwarpedQ;
		
		//	increment iterCount
		iterCount ++;
		if( iterCount > MAX_NUM_ITERATIONS )
			throw ynxValueException( "LensDistortionWarp::removeWarp() : iterCount exceeds numerical maximum! Giving up!" );
	}
	
	//	return
	return unwarpedQ;
}
	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	END CLASS InvertWarpFuncs MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

