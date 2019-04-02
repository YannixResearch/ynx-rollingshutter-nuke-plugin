//---------------------------------------------------------------------
//
//	Program written by Wiphoo (Terng) Methachawalit for
//		Yannix 2018/07/23
//	Copyright Yannix (Thailand) Co., Ltd 2018. All rights reserved
//
//---------------------------------------------------------------------
#if !defined(___InvertWarpFuncs_h)
#define ___InvertWarpFuncs_h

//---------------------------------------------------------------------
//
//	STANDARD INCLUDES
//
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include "RollingShutterLensDistortionEngine.h"

#ifdef YNX_STANDALONE
//	ynxminimal
#	include "YnxMinimal.h"
#else
//	ynxmath
#	include <ynxmath/Vector2.h>
#endif

//---------------------------------------------------------------------
//
//	DEFINES
//
//---------------------------------------------------------------------

//	acceptable pixel error when doing numerical inverses
#define DEFAULT_NUMERICAL_ERROR 1e-7

//---------------------------------------------------------------------
//
//	INLINES
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	GLOBALS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	class InvertWarpFuncs
//
//---------------------------------------------------------------------
class InvertWarpFuncs
{
	//---------------------------------------------------------------------
	//	public member classes
	//---------------------------------------------------------------------
	public:

	//---------------------------------------------------------------------
	//	public member data
	//---------------------------------------------------------------------
	public:

	//---------------------------------------------------------------------
	//	protected member data
	//---------------------------------------------------------------------
	protected:
	
	//---------------------------------------------------------------------
	//	private member data
	//---------------------------------------------------------------------
	private:
	
	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
	public:
// 		InvertWarpFuncs();
// 		
// 		~InvertWarpFuncs();
		
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------
	public:
	
	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------
	public:
	
		//	numerically invert this->applyWarp to within
		//		a numericalError of this->numericalError
		//	THROWS ynxValueException if it isn't able to solve a remove warp position.
		//		Example : when something like a downward parabola occurs, 
		//		this numerical method will not be able to invert it above the vertex.
		//		Therefore, in Nuke lens distortion when r^4 is negative and r is high, you cannot remove warp. 
		//		Normally this is not a problem because it typically happens in the overscan area only where r is very high.
		static Vector2 removeWarp( const RollingShutterLensDistortionEngine &lensDistortionEngine,
											const Vector2 &initialRemoveWarpGuessQ,
											const Vector2 &q,
											double numericalError = DEFAULT_NUMERICAL_ERROR ) 
								throw( ynxValueException );

	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------
	public:
	
	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------
	protected:
	
};
//---------------------------------------------------------------------
//	END class InvertWarpFuncs
//---------------------------------------------------------------------

#endif
//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

