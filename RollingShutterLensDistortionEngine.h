//---------------------------------------------------------------------
//
//	Program written by Wiphoo (Terng) Methachawalit for
//		Yannix 2018/07/23
//	Copyright Yannix (Thailand) Co., Ltd 2018. All rights reserved
//
//---------------------------------------------------------------------
#if !defined(___RollingShutterLensDistortionEngine_h)
#define ___RollingShutterLensDistortionEngine_h

//---------------------------------------------------------------------
//
//	STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include <vector>

//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

#ifdef YNX_STANDALONE
//	using yannix minimal as a header for yxnexception and ynxmath (Vector2 and ParabolicFit)
#	include "YnxMinimal.h"
#else
//	yannix exception
#	include <ynxexception/ynxValueException.h>
//	yannix math
#	include <ynxmath/Vector2.h>
#	include <ynxmath/interpolate/ParabolicFit.h>
#endif

//---------------------------------------------------------------------
//
//	DEFINES
//
//---------------------------------------------------------------------

#ifdef YNX_STANDALONE
	using Vector2 = YnxMinimal::Vector2;
	using ParabolicFit = YnxMinimal::ParabolicFit;
	using ynxValueException = YnxMinimal::ynxValueException;
#endif


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
//	class RollingShutterPointMotion
//
//---------------------------------------------------------------------

//	definition object for motion of a single point
class RollingShutterPointMotion
{
	public:
		//	position of this point on previous/next frame
		Vector2 previousPosition, nextPosition;

	
	public:
		//contructors/destructors
		RollingShutterPointMotion()
		{
		}
		
		RollingShutterPointMotion( const Vector2 &previousPosition, 
									const Vector2 &nextPosition )
		{
			this->set( previousPosition, nextPosition );
		}					

	public:
		//	set function
		void set( const Vector2 &previousPosition, 
					const Vector2 &nextPosition )
		{
			this->previousPosition.copy( previousPosition );
			this->nextPosition.copy( nextPosition );
		}
		
		// copy function
		void copy( const RollingShutterPointMotion &other )
		{

			this->previousPosition.copy( other.previousPosition );
			this->nextPosition.copy( other.nextPosition );				
		}

		//	interpolate position where f(-1) is previousPosition
		//		f(0) is currentPosition and f(1) is nextPosition
		Vector2 interpolatePosition( double t, const Vector2 &currentPosition ) const
		{
			ParabolicFit x( this->previousPosition.x, 
							currentPosition.x, 
							this->nextPosition.x ),
						y( this->previousPosition.y, 
							currentPosition.y, 
							this->nextPosition.y );
			return Vector2( x.f( t ), y.f( t ) );			
		}
};
//---------------------------------------------------------------------
//	END class RollingShutterPointMotion
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	class RollingShutterSingleFrameMotion
//
//---------------------------------------------------------------------

//	motion definition of a set of points on a single frame
class RollingShutterSingleFrameMotion
{
	public:
		//	topL/M/R and bottomL/M/R point motion
		RollingShutterPointMotion top[3], bottom[3];

	public:
		//contructors/destructors
		RollingShutterSingleFrameMotion()
		{
		}
		
		RollingShutterSingleFrameMotion( const RollingShutterPointMotion &top0,
											const RollingShutterPointMotion &top1,
											const RollingShutterPointMotion &top2,
											const RollingShutterPointMotion &bottom0,
											const RollingShutterPointMotion &bottom1,
											const RollingShutterPointMotion &bottom2 )
		{
		
			this->set( top0, top1, top2,
						bottom0, bottom1, bottom2 );
		
		}

	public:
		//	set function
		void set( const RollingShutterPointMotion &top0,
					const RollingShutterPointMotion &top1,
					const RollingShutterPointMotion &top2,
					const RollingShutterPointMotion &bottom0,
					const RollingShutterPointMotion &bottom1,
					const RollingShutterPointMotion &bottom2 )
		{			
			//	Set top rolling shutter point motion
			//		Top
			this->top[0].copy( top0 );
			this->top[1].copy( top1 );
			this->top[2].copy( top2 );
			//		Bottom
			this->bottom[0].copy( bottom0 );
			this->bottom[1].copy( bottom1 );
			this->bottom[2].copy( bottom2 );
			
		}
					
		//	copy function
		void copy( const RollingShutterSingleFrameMotion &other )
		{
			//	Copy point motion 
			//		Top
			this->top[0].copy( other.top[0] );
			this->top[1].copy( other.top[1] );
			this->top[2].copy( other.top[2] );
			//		Bottom
			this->bottom[0].copy( other.bottom[0] );
			this->bottom[1].copy( other.bottom[1] );
			this->bottom[2].copy( other.bottom[2] );

		}
};
//---------------------------------------------------------------------
//	END class RollingShutterSingleFrameMotion
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//
//	class RollingShutterLensDistortionEngine
//
//---------------------------------------------------------------------
class RollingShutterLensDistortionEngine
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
		
		//	Rolling shutter ratio
		//		0 is top of current frame same as bottom of previous frame ( At the same time ).
		//		1 is top of current frame equals as bottom of previous frame.
		double rollingShutterRatio;

		//	Top and bottom point depth
 		double topPointDepth, bottomPointDepth;
	
		//	point position for top/middle/bottom points and whether their
		//		values have been initialized
		static const Vector2 sTopPointPosition[3], 
						sMiddlePointPosition[3], 
						sBottomPointPosition[3];
			
		//	warp offset for top and bottom points (in NDC [-1,1]x[-1,1])
		Vector2 bottomPointWarpOffset[3], 
				topPointWarpOffset[3];
		
		//	current frame motion
		RollingShutterSingleFrameMotion currentMotionData;
		
	//---------------------------------------------------------------------
	//	private member data
	//---------------------------------------------------------------------
	private:
	
	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
	public:
		RollingShutterLensDistortionEngine();
		
		~RollingShutterLensDistortionEngine();
		
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------
	public:
	
		//	get/set rolling shutter ratio
		double getRollingShutterRatio()
		{	return this->rollingShutterRatio;	}
		void setRollingShutterRatio( double value )	
		{	this->rollingShutterRatio = value;	}
		double *getRollingShutterRatioPtr()
		{	return &this->rollingShutterRatio;	}
		
		//	get/set top and bottom point depth
		double getTopPointDepth()
		{	return this->topPointDepth;	}
		void setTopPointDepth( double value )
		{	this->topPointDepth = value;	}
		double *getTopPointDepthPtr()
		{	return &this->topPointDepth;	}
		double getBottomPointDepth()
		{	return this->bottomPointDepth;	}
		void setBottomPointDepth( double value )
		{	this->bottomPointDepth = value;	}
		double *getBottomPointDepthPtr()
		{	return &this->bottomPointDepth;	}
				
		//	set top left/middle/right point previous and next parameters
		void setTopLeftPrevNextPoint( const Vector2 &topLeftPrev, 
										const Vector2 &topLeftNext );
										
		void setTopMiddlePrevNextPoint( const Vector2 &topMiddlePrev, 
										const Vector2 &topMiddleNext );
										
		void setTopRightPrevNextPoint( const Vector2 &topRightPrev, 
										const Vector2 &topRightNext );
		
		void setBottomLeftPrevNextPoint( const Vector2 &bottomLeftPrev, 
										const Vector2 &bottomLeftNext );
		
		void setBottomMiddlePrevNextPoint( const Vector2 &bottomMiddlePrev, 
										const Vector2 &bottomMiddleNext );
		
		void setBottomRightPrevNextPoint( const Vector2 &bottomRightPrev, 
										const Vector2 &bottomRightNext );
		
		//	get NDC point position
		static Vector2 getTopPointPosition( int index )
		{ return sTopPointPosition[index]; }
		static Vector2 getMiddlePointPosition( int index )
		{ return sMiddlePointPosition[index]; }
		static Vector2 getBottomPointPosition( int index )
		{ return sBottomPointPosition[index]; }
		
		//	get bottom/top point warp offset
		Vector2 getBottomPointWarpOffset( int i ) const
		{ return this->bottomPointWarpOffset[i]; }
		Vector2 getTopPointWarpOffset( int i ) const
		{ return this->topPointWarpOffset[i]; }
		
		//	get pointer to current motion data
		RollingShutterSingleFrameMotion *getCurrentMotionDataPtr()
		{	return &this->currentMotionData; }
			
	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------
	public:
		
		//	set to identity distortion and default all
		//		distortion values
		void setToIdentityDefaults();	

		//	precompute values for calling this->applyWarp a particular frame
		//	NOTE that this function must be called after this->setRollingShutterRatio()
		//		before this->applyWarp()
		void precompute()
		{	
		
			//	get motion data on this frame
			const RollingShutterSingleFrameMotion *currentMotionData = &this->currentMotionData;
			
			//	set point warp offsets
			for( int i = 0 ; i < 3 ; i ++ )
			{
				//	get bottom point position
				const Vector2 &currentBottomPosition = RollingShutterLensDistortionEngine::sBottomPointPosition[i];
				Vector2 warpedBottomPointPosition( currentMotionData->bottom[i].interpolatePosition( 
																		-rollingShutterRatio, 
																		currentBottomPosition ) );
				
				//	convert to offset
				this->bottomPointWarpOffset[i] = warpedBottomPointPosition - currentBottomPosition;

				//	get top point position
				const Vector2 &currentTopPosition = RollingShutterLensDistortionEngine::sTopPointPosition[i];
				Vector2 warpedTopPointPosition( currentMotionData->top[i].interpolatePosition( 
																		rollingShutterRatio, 
																		currentTopPosition ) );
				
				//	convert to offset
				this->topPointWarpOffset[i] = warpedTopPointPosition - currentTopPosition;

			}
			
		}
			
		//	check if this distortion object has any effect
		//		at all or whether it is simply an identity
		//		warp
		bool isIdentity() const;
			
		//	do a mathematically "forward" warp to a pixel.
		//	NOTE for subclass implementers: %p% is in [0,1]
		//		REMEMBER THAT!
		Vector2 applyWarp( const Vector2 &p ) const throw( ynxValueException );
		
		//	numerically invert this->applyWarp
		Vector2 removeWarp( const Vector2 &q ) const throw( ynxValueException );

	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------
	public:
		
	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------
	protected:
	
		//	convert NDC [-1,1] to point in [0,1] coordinate
		//	%ndcVal% is value in NDC [-1,1]
		static inline double convertNdcToEffectivePixel( double ndcVal )
		{
			return ( ndcVal + 1 ) / 2.;
		}
			
		//	convert point in [0,1] coordinate to NDC [-1,1]
		//	%effectiveVal% is value in [0,1] coordinates which is converted from pixel
		static inline double convertEffectivePixelToNdc( double effectiveVal )
		{
			return ( 2 * effectiveVal ) - 1;
		}
	
};
//---------------------------------------------------------------------
//	END class RollingShutterLensDistortionEngine
//---------------------------------------------------------------------

#endif
//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

