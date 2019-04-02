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

//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include "RollingShutterLensDistortionEngine.h"

//	invert warp func 
#include "InvertWarpFuncs.h"

//---------------------------------------------------------------------
//	DEFINES AND INLINES
//---------------------------------------------------------------------

#define EPSILON 1e-7
#define FARAWAYDEPTH 1e10

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
//	CLASS RollingShutterLensDistortionEngine MEMBER CLASSES
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS RollingShutterLensDistortionEngine STATIC MEMBERS
//
//---------------------------------------------------------------------

//	point position for top/middle/bottom points and whether their
//		values have been initialized
const Vector2 RollingShutterLensDistortionEngine::sTopPointPosition[3] = 
{ 
	Vector2( -1, 1 ), 
	Vector2( 0, 1 ), 
	Vector2( 1, 1 ), 
},
				RollingShutterLensDistortionEngine::sMiddlePointPosition[3] = 
{ 
	Vector2( -1, 0 ), 
	Vector2( 0, 0 ), 
	Vector2( 1, 0 ), 
},
				RollingShutterLensDistortionEngine::sBottomPointPosition[3] = 
{ 
	Vector2( -1, -1 ), 
	Vector2( 0, -1 ), 
	Vector2( 1, -1 ), 
};

//---------------------------------------------------------------------
//
//	CLASS RollingShutterLensDistortionEngine MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
RollingShutterLensDistortionEngine::RollingShutterLensDistortionEngine()
{
	//	Initial default
	this->setToIdentityDefaults();
}
RollingShutterLensDistortionEngine::~RollingShutterLensDistortionEngine()
{
	
}
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------

//	set top left/middle/right point previous and next parameters
void RollingShutterLensDistortionEngine::setTopLeftPrevNextPoint( const Vector2 &topLeftPrev, 
															const Vector2 &topLeftNext )
{
	//	Set top left previous point
	this->currentMotionData.top[0].previousPosition.copy( topLeftPrev );
	//	Set top left next point
	this->currentMotionData.top[0].nextPosition.copy( topLeftNext );
}

void RollingShutterLensDistortionEngine::setTopMiddlePrevNextPoint( const Vector2 &topMiddlePrev, 
																const Vector2 &topMiddleNext )
{
	//	Set top middle previous point
	this->currentMotionData.top[1].previousPosition.copy( topMiddlePrev );
	//	Set top middle next point
	this->currentMotionData.top[1].nextPosition.copy( topMiddleNext );
}

void RollingShutterLensDistortionEngine::setTopRightPrevNextPoint( const Vector2 &topRightPrev, 
																const Vector2 &topRightNext )
{
	//	Set top right previous point
	this->currentMotionData.top[2].previousPosition.copy( topRightPrev );
	//	Set top right next point
	this->currentMotionData.top[2].nextPosition.copy( topRightNext );
}

void RollingShutterLensDistortionEngine::setBottomLeftPrevNextPoint( const Vector2 &bottomLeftPrev, 
																const Vector2 &bottomLeftNext )
{
	//	Set bottom left previous point
	this->currentMotionData.bottom[0].previousPosition.copy( bottomLeftPrev );
	//	Set bottom left next point
	this->currentMotionData.bottom[0].nextPosition.copy( bottomLeftNext );
}

void RollingShutterLensDistortionEngine::setBottomMiddlePrevNextPoint( const Vector2 &bottomMiddlePrev, 
																	const Vector2 &bottomMiddleNext )
{
	//	Set bottom middle previous point
	this->currentMotionData.bottom[1].previousPosition.copy( bottomMiddlePrev );
	//	Set bottom middle next point
	this->currentMotionData.bottom[1].nextPosition.copy( bottomMiddleNext );
}

void RollingShutterLensDistortionEngine::setBottomRightPrevNextPoint( const Vector2 &bottomRightPrev, 
																	const Vector2 &bottomRightNext )
{
	//	Set bottom right previous point
	this->currentMotionData.bottom[2].previousPosition.copy( bottomRightPrev );
	//	Set bottom right next point
	this->currentMotionData.bottom[2].nextPosition.copy( bottomRightNext );
}

	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------

//	set to identity distortion and default all
//		distortion values
void RollingShutterLensDistortionEngine::setToIdentityDefaults()
{
	//	Rolling shutter ratio
	this->rollingShutterRatio = 0;
	
 	//	Top and bottom point depth
 	this->topPointDepth = this->bottomPointDepth = FARAWAYDEPTH;
}

//	check if this distortion object has any effect
//		at all or whether it is simply an identity
//		warp
bool RollingShutterLensDistortionEngine::isIdentity() const
{
	//	Check for identity of rolling shutter distortion	
	if( ( this->rollingShutterRatio - 0.0 ) <= EPSILON )
	{
		// Rolling shutter ratio is 0.
		return true;
	}
	
	//	Rolling shutter ratio is not 0
	return false;	
}

//	do a mathematically "forward" warp to a pixel.
//	NOTE for subclass implementers: %p% is in [0,1]
//		REMEMBER THAT!
Vector2 RollingShutterLensDistortionEngine::applyWarp( const Vector2 &p ) const throw( ynxValueException )
{
	// Normailize given position
	Vector2 ndcP( this->convertEffectivePixelToNdc( p.x ),
					this->convertEffectivePixelToNdc( p.y ) );


	//	Create parabolic fit for top
	ParabolicFit fitTopX( this->topPointWarpOffset[0].x, 
							this->topPointWarpOffset[1].x,
							this->topPointWarpOffset[2].x );
	ParabolicFit fitTopY( this->topPointWarpOffset[0].y, 
							this->topPointWarpOffset[1].y,
							this->topPointWarpOffset[2].y );						

	//	Interpolate top
	Vector2 interpolateTop( fitTopX.f( ndcP.x ), 
							fitTopY.f( ndcP.x ) );


	//	Create parabolic fit for bottom
	ParabolicFit fitBottomX( this->bottomPointWarpOffset[0].x, 
							this->bottomPointWarpOffset[1].x,
							this->bottomPointWarpOffset[2].x );
	ParabolicFit fitBottomY( this->bottomPointWarpOffset[0].y, 
							this->bottomPointWarpOffset[1].y,
							this->bottomPointWarpOffset[2].y );						

	Vector2 interpolateBottom( fitBottomX.f( ndcP.x ), 
							fitBottomY.f( ndcP.x ) );						

	//	Create parabolic fit for vertical
	ParabolicFit fitVerticalX( interpolateBottom.x, 0, 
								interpolateTop.x ),
				fitVerticalY( interpolateBottom.y, 0, 
								interpolateTop.y );

	//	compute amount of warp offset at p (in NDC)
	Vector2 warpOffset( fitVerticalX.f( ndcP.y ),
						fitVerticalY.f( ndcP.y ) );														

	//	add to ndcP
	ndcP += warpOffset;
	
	//	convert back to pixels and return
	return Vector2( this->convertNdcToEffectivePixel( ndcP.x ),
					this->convertNdcToEffectivePixel( ndcP.y ) );	
}

//	numerically invert this->applyWarp
Vector2 RollingShutterLensDistortionEngine::removeWarp( const Vector2 &q ) const throw( ynxValueException )
{
// 	//	do nothing when invert warp func do not set
// 	if( this->invertWarpFunc == NULL )
// 		return;
		
	return InvertWarpFuncs::removeWarp( *this, 
								/*initialRemoveWarpGuessQ = */ q, 
								q
								);
}

	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	END CLASS RollingShutterLensDistortionEngine MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

