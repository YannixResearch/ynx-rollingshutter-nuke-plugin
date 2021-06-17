//---------------------------------------------------------------------
//
//	Program written by Channarong (Aon) Khamphusa for
//		Yannix 2018/06/13
//	Copyright Yannix (Thailand) Co., Ltd 2018. All rights reserved
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include <assert.h>
#include <iostream>
#include <cmath>

#include <DDImage/Tile.h>
#include <DDImage/Pixel.h>
#include <DDImage/Vector2.h>

//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

#include "YnxRollingShutterNode.h"

//---------------------------------------------------------------------
//	DEFINES AND INLINES
//---------------------------------------------------------------------

//	defines
#define FARAWAYDEPTH 1e10

#define DEFAULT_LOWER_BOUND_VALUE -2
#define DEFAULT_UPPER_BOUND_VALUE 2

//	debug flags
// #define DEBUG_KNOBS
// #define DEBUG_ENGINE
// #define DEBUG_VALIDATE
// #define DEBUG_REQUEST
// #define DEBUG_NORMAILIZE_POINT

//---------------------------------------------------------------------
//	normalize/unnormalize functions

//	convert from image space to normalized [0,1] [( 1 - height/width )/2,height/width + ( 1 - height/width )/2] space 
inline void normalizePoint( const Vector2 &p, 
									double imageWidth, double imageHeight, 
									double pixelAspectRatio, 
									Vector2 *normalizedP_ret )
{
#ifdef DEBUG_NORMAILIZE_POINT
	std::cout << "\n **********************************************" << std::endl;
	std::cout << "   normalizePoint( p = ( " << p.x << ", " << p.y 
				<< " ), width = " << imageWidth << ", height = " << imageHeight
				<< ", pixelAspectRatio = " << pixelAspectRatio << " )" << std::endl;
#endif
	//	unsqueeze
	normalizedP_ret->copy( p );
	normalizedP_ret->x *= pixelAspectRatio;
#ifdef DEBUG_NORMAILIZE_POINT
	std::cout << "  unsqueeze = ( " << normalizedP_ret->x << ", " << normalizedP_ret->y << " )" << std::endl;
#endif
	//	Convert coordinate to [0,1], [0,height/width]
	normalizedP_ret->divide( imageWidth * pixelAspectRatio );

#ifdef DEBUG_NORMAILIZE_POINT
	std::cout << "  convert to [0,1], [0,height/width] = ( " << normalizedP_ret->x << ", " << normalizedP_ret->y << " )" << std::endl;
#endif

	//	Convert coordinate to [0,1], [( 1 - height/width )/2 , height/width + ( 1 - height/width )/2 ]
	normalizedP_ret->y += ( 1 - ( imageHeight / imageWidth / pixelAspectRatio ) ) / 2;

#ifdef DEBUG_NORMAILIZE_POINT
	std::cout << "  convert to [-1,1], [-height/width,height/width] = ( " << normalizedP_ret->x << ", " << normalizedP_ret->y << " )" << std::endl;
#endif			
}

//	convert from normalized [0,1], [( 1 - height/width )/2 , height/width + ( 1 - height/width )/2 ] space
//		to image space
inline void unnormalizePoint( const Vector2 &p, 
										double imageWidth, double imageHeight, 
										double pixelAspectRatio, 
										Vector2 *unnormalizedP_ret )
									throw( ynxValueException )
{
	//	Convert coordinate to [0,1], [( 1 - height/width )/2 , height/width + ( 1 - height/width )/2 ]
	unnormalizedP_ret->copy( p ); 
 	unnormalizedP_ret->y -= ( 1 - ( imageHeight / imageWidth / pixelAspectRatio ) ) / 2;			

	//	Convert coordinate to [0,1], [0,height/width]
	unnormalizedP_ret->multiply( imageWidth * pixelAspectRatio );

	//	Unsqueeze
	unnormalizedP_ret->x /= pixelAspectRatio;			
}


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
//	CLASS YnxRollingShutterNode MEMBER CLASSES
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS YnxRollingShutterNode STATIC MEMBERS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	CLASS YnxRollingShutterNode MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
YnxRollingShutterNode::YnxRollingShutterNode( Node *node ) : DD::Image::Iop( node )
{
	//	initialize rollingShutterRatio
	this->rollingShutterLensDistortionEngine.setRollingShutterRatio( 0.0 );
	
	//	initialize top/bottom point depth
	this->rollingShutterLensDistortionEngine.setTopPointDepth( FARAWAYDEPTH );
	this->rollingShutterLensDistortionEngine.setBottomPointDepth( FARAWAYDEPTH );
	
	//	point position for top/middle/bottom points and whether their
	//		values have been initialized
	this->rollingShutterLensDistortionEngine.setTopLeftPrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	this->rollingShutterLensDistortionEngine.setTopMiddlePrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	this->rollingShutterLensDistortionEngine.setTopRightPrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	this->rollingShutterLensDistortionEngine.setBottomLeftPrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	this->rollingShutterLensDistortionEngine.setBottomMiddlePrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	this->rollingShutterLensDistortionEngine.setBottomRightPrevNextPoint( Vector2( 0, 0 ), Vector2( 0, 0 ) );
	
	//	set default for undistort
	this->isUndistort = false;
	
}
YnxRollingShutterNode::~YnxRollingShutterNode()
{
	
}
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------
	
//	This function is do all work
void YnxRollingShutterNode::engine( int y, int x, int r,
                              			DD::Image::ChannelMask channelMask, DD::Image::Row &outputRow )
{

#ifdef DEBUG_ENGINE
	std::cout << "YnxRollingShutterNode::engine( y = " << y << ", x = " << x << ", r = " << r << " )" << std::endl;
#endif
	//	construct pixel for store pixel value 	
	DD::Image::Pixel pixel( channelMask );
		
	//	construct ynx vector2 for send position to apply warp and get the results
	Vector2 normalizedInputPositionXYYnxVector, normalizedOutputPositionXYYnxVector,
			inputPositionXYYnxVector, outputPositionXYYnxVector;
	
	//	get width/height
	int inputWidth = this->format().width(),
	    inputHeight = this->format().height();
	
	//	loop all position in this row ( x ) 
	//		and apply warp
	for( ; x < r ; x++ )
	{
		//	normarlize input position before apply warp
		inputPositionXYYnxVector = Vector2( x, y );
		::normalizePoint( inputPositionXYYnxVector, inputWidth, inputHeight, 1, &normalizedInputPositionXYYnxVector );
		bool isCannotWarp = false;
		if( this->isUndistort )
		{
			try
			{
				//	remove warp and get output position
				normalizedOutputPositionXYYnxVector = this->rollingShutterLensDistortionEngine.removeWarp( normalizedInputPositionXYYnxVector );
			}
			catch( ynxValueException &e )
			{
				//	set flag can't to true
				//		if can't warp the pixel value will be 0 ( black )
				isCannotWarp = true;
			}		
		}
		
		else
		{
			try
			{
				//	apply warp and get output position
				normalizedOutputPositionXYYnxVector = this->rollingShutterLensDistortionEngine.applyWarp( normalizedInputPositionXYYnxVector );
			}
			catch( ynxValueException &e )
			{
				//	set flag can't to true
				//		if can't warp the pixel value will be 0 ( black )			
				isCannotWarp = true;
			}
		}
		
		//	unnormalize output position
		::unnormalizePoint( normalizedOutputPositionXYYnxVector, inputWidth, inputHeight, 1, &outputPositionXYYnxVector );
		
		//	if can't warp position
		//		set that position pixel value to black 
		if( isCannotWarp )
		{
			foreach( channel, channelMask )
			{
				pixel[channel] = 0;
			}
		}
		
		else
		{
			//	get pixel value to set to image
			input(0)->sample( 
								//	llx
								outputPositionXYYnxVector.x + 0.5f, 
								//	lly
								outputPositionXYYnxVector.y + 0.5f,
								//	size to get data x ( in pixel )
								1.0f,
								//	size to get data y ( in pixel )
								1.0f,
								//	pixel result
								pixel );
		}

		//	loop all channel and set value that we get 					
 		foreach( channel, channelMask )
 		{
 			outputRow.writable( channel );
 			( (float *)outputRow[channel] )[x] = pixel[channel];
 		}
	}
}

//	Function for create knob ( knobs are fundamentals of all user interface elements available to NUKE Ops. )
//		For more information https://learn.foundry.com/nuke/developers/63/ndkdevguide/knobs-and-handles/index.html
void YnxRollingShutterNode::knobs( DD::Image::Knob_Callback f )
{
#ifdef DEBUG_KNOBS
	std::cout << "YnxRollingShutterNode::knobs()" << std::endl;
#endif	

	//	get pointer to relevant parameters
	double *rollingShutterRatioPtr = this->rollingShutterLensDistortionEngine.getRollingShutterRatioPtr();
	double *topPointDepthPtr = this->rollingShutterLensDistortionEngine.getTopPointDepthPtr();
	double *bottomPointDepthPtr = this->rollingShutterLensDistortionEngine.getBottomPointDepthPtr();
	RollingShutterSingleFrameMotion *currentMotionDataPtr = this->rollingShutterLensDistortionEngine.getCurrentMotionDataPtr();
			
	
	Bool_knob(f, &this->isUndistort, "undistort");
	
	//	knob for to set value for rolling shutter ratio
	Double_knob(f, rollingShutterRatioPtr, DD::Image::IRange(0, 1), "rollingShutterRatio");
	
	//	knob for to set value for top point depth
	Double_knob(f, topPointDepthPtr, DD::Image::IRange(0, FARAWAYDEPTH), "topPointDepth");
	
	//	knob for to set value for bottom point depth
	Double_knob(f, bottomPointDepthPtr, DD::Image::IRange(0, FARAWAYDEPTH), "bottomPointDepth");
	
	//------------------------------------
	//	Top
	
	//	knob for to set value for top left previous x
	Double_knob(f, &currentMotionDataPtr->top[0].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topLeftPrevX");
	
	//	knob for to set value for top left previous y
	Double_knob(f, &currentMotionDataPtr->top[0].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topLeftPrevY");
	
	//	knob for to set value for top left next x
	Double_knob(f, &currentMotionDataPtr->top[0].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topLeftNextX");
	
	//	knob for to set value for top left next y
	Double_knob(f, &currentMotionDataPtr->top[0].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topLeftNextY");

	//	knob for to set value for top middle previous x
	Double_knob(f, &currentMotionDataPtr->top[1].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topMiddlePrevX");
	
	//	knob for to set value for top middle previous y
	Double_knob(f, &currentMotionDataPtr->top[1].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topMiddlePrevY");
	
	//	knob for to set value for top middle next x
	Double_knob(f, &currentMotionDataPtr->top[1].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topMiddleNextX");
	
	//	knob for to set value for top middle next y
	Double_knob(f, &currentMotionDataPtr->top[1].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topMiddleNextY");
	
	//	knob for to set value for top right previous x
	Double_knob(f, &currentMotionDataPtr->top[2].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topRightPrevX");
	
	//	knob for to set value for top right previous y
	Double_knob(f, &currentMotionDataPtr->top[2].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topRightPrevY");
	
	//	knob for to set value for top right next x
	Double_knob(f, &currentMotionDataPtr->top[2].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topRightNextX");
	
	//	knob for to set value for top right next y
	Double_knob(f, &currentMotionDataPtr->top[2].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "topRightNextY");
	
	//------------------------------------
	//	Bottom

	//	knob for to set value for bottom left previous x
	Double_knob(f, &currentMotionDataPtr->bottom[0].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomLeftPrevX");
	
	//	knob for to set value for bottom left previous y
	Double_knob(f, &currentMotionDataPtr->bottom[0].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomLeftPrevY");
	
	//	knob for to set value for bottom left next x
	Double_knob(f, &currentMotionDataPtr->bottom[0].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomLeftNextX");
	
	//	knob for to set value for bottom left next y
	Double_knob(f, &currentMotionDataPtr->bottom[0].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomLeftNextY");

	//	knob for to set value for bottom middle previous x
	Double_knob(f, &currentMotionDataPtr->bottom[1].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomMiddlePrevX");
	
	//	knob for to set value for bottom middle previous y
	Double_knob(f, &currentMotionDataPtr->bottom[1].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomMiddlePrevY");
	
	//	knob for to set value for bottom middle next x
	Double_knob(f, &currentMotionDataPtr->bottom[1].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomMiddleNextX");
	
	//	knob for to set value for bottom middle next y
	Double_knob(f, &currentMotionDataPtr->bottom[1].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomMiddleNextY");
	
	//	knob for to set value for bottom right previous x
	Double_knob(f, &currentMotionDataPtr->bottom[2].previousPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomRightPrevX");
	
	//	knob for to set value for bottom right previous y
	Double_knob(f, &currentMotionDataPtr->bottom[2].previousPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomRightPrevY");
	
	//	knob for to set value for bottom right next x
	Double_knob(f, &currentMotionDataPtr->bottom[2].nextPosition.x, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomRightNextX");
	
	//	knob for to set value for bottom right next y
	Double_knob(f, &currentMotionDataPtr->bottom[2].nextPosition.y, DD::Image::IRange( DEFAULT_LOWER_BOUND_VALUE, DEFAULT_UPPER_BOUND_VALUE ), "bottomRightNextY");
	
	
}

//	This function is used for validate parameter value
void YnxRollingShutterNode::_validate( bool for_real )
{
#ifdef DEBUG_VALIDATE
	std::cout << "YnxRollingShutterNode::_validate()" << std::endl;
#endif
	//	do nothing if there's no input 	
	if( !this->input(0) )
	{ return; }

#ifdef DEBUG_VALIDATE
	RollingShutterSingleFrameMotion *currentMotionDataPtr = this->rollingShutterLensDistortionEngine.getCurrentMotionDataPtr();
	
	std::cout << "	this->rollingShutterRatio = " << this->rollingShutterLensDistortionEngine.getRollingShutterRatio() << std::endl;
	std::cout << "	this->topPointDepth = " << this->rollingShutterLensDistortionEngine.getTopPointDepth() << std::endl;
	std::cout << "	this->bottomPointDepth = " << this->rollingShutterLensDistortionEngine.getBottomPointDepth() << std::endl;
	std::cout << std::endl;
	std::cout << "		this->topLeftPrev(X,Y) = " << currentMotionDataPtr->top[0].previousPosition.x << ", " << currentMotionDataPtr->top[0].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->topLeftNext(X,Y) = " << currentMotionDataPtr->top[0].nextPosition.x << ", " << currentMotionDataPtr->top[0].nextPosition.y << " ) " << std::endl;
	std::cout << std::endl;
	std::cout << "		this->topMiddlePrev(X,Y) = " << currentMotionDataPtr->top[1].previousPosition.x << ", " << currentMotionDataPtr->top[1].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->topMiddleNext(X,Y) = " << currentMotionDataPtr->top[1].nextPosition.x << ", " << currentMotionDataPtr->top[1].nextPosition.y << " ) " << std::endl;
	std::cout << std::endl;
	std::cout << "		this->topRightPrev(X,Y) = " << currentMotionDataPtr->top[2].previousPosition.x << ", " << currentMotionDataPtr->top[2].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->topRightNext(X,Y) = " << currentMotionDataPtr->top[2].nextPosition.x << ", " << currentMotionDataPtr->top[2].nextPosition.y << " ) " << std::endl;
	std::cout << std::endl;
	std::cout << "		this->bottomLeftPrev(X,Y) = " << currentMotionDataPtr->bottom[0].previousPosition.x << ", " << currentMotionDataPtr->bottom[0].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->bottomLeftNext(X,Y) = " << currentMotionDataPtr->bottom[0].nextPosition.x << ", " << currentMotionDataPtr->bottom[0].nextPosition.y << " ) " << std::endl;
	std::cout << std::endl;
	std::cout << "		this->bottomMiddlePrev(X,Y) = " << currentMotionDataPtr->bottom[1].previousPosition.x << ", " << currentMotionDataPtr->bottom[1].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->bottomMiddleNext(X,Y) = " << currentMotionDataPtr->bottom[1].nextPosition.x << ", " << currentMotionDataPtr->bottom[1].nextPosition.y << " ) " << std::endl;
	std::cout << std::endl;
	std::cout << "		this->bottomRightPrev(X,Y) = " << currentMotionDataPtr->bottom[2].previousPosition.x << ", " << currentMotionDataPtr->bottom[2].previousPosition.y << " ) " << std::endl;
	std::cout << "		this->bottomRightNext(X,Y) = " << currentMotionDataPtr->bottom[2].nextPosition.x << ", " << currentMotionDataPtr->bottom[2].nextPosition.y << " ) " << std::endl;
			
#endif
	//	precompute the rolling shutter warp 
	this->rollingShutterLensDistortionEngine.precompute();

#ifdef DEBUG_VALIDATE
	std::cout << "		this->getBottomPointWarpOffset(X,Y)[0] = " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(0).x << ", " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(0).y << std::endl;
	std::cout << "		this->getBottomPointWarpOffset(X,Y)[1] = " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(1).x << ", " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(1).y << std::endl;
	std::cout << "		this->getBottomPointWarpOffset(X,Y)[2] = " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(2).x << ", " << this->rollingShutterLensDistortionEngine.getBottomPointWarpOffset(2).y << std::endl;

	std::cout << "		this->getTopPointWarpOffset(X,Y)[0] = " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(0).x << ", " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(0).y << std::endl;
	std::cout << "		this->getTopPointWarpOffset(X,Y)[1] = " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(1).x << ", " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(1).y << std::endl;
	std::cout << "		this->getTopPointWarpOffset(X,Y)[2] = " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(2).x << ", " << this->rollingShutterLensDistortionEngine.getTopPointWarpOffset(2).y << std::endl;		
#endif
		
	//	validate the input of this node
	this->input(0)->validate( for_real );

	//	copy data from input into info_
	this->copy_info();
	this->set_out_channels( DD::Image::Mask_All );
	
 	this->info_.black_outside( false );
 	
	//	compute bounding box by sampling point and warp to get min, max to decide as bounding box
	DD::Image::Box boundingBox = this->getBoundingBox( this->input0().info().x(), 
							  this->input0().info().y(), 
							  this->input0().info().r(), 
							  this->input0().info().t() );
	
	//	copy input's bounding box
	DD::Image::Box inputBoundingBox = this->info_;
	
	//	merge the bouding box of input and bounding box after sampling some pixel
	inputBoundingBox.merge( boundingBox );
	
	//	set the new bounding box size
	this->info_.set( inputBoundingBox );
	
}

//	This function is used for request region of data before send into engine func
void YnxRollingShutterNode::_request( int x, int y, int r, int t, DD::Image::ChannelMask channels, int count )
{
#ifdef DEBUG_REQUEST
	std::cout << "YnxRollingShutterNode::_request( x = " << x << ", y = " << y << ", r = " << r << ", t = " << t << " )" << std::endl;
#endif
	
	//	get bounding box by sampling the position and warp
	//		to get bounding box
	DD::Image::Box boundingBox = this->getBoundingBox( x, y, r, t );
	
	//	request the region that bounding box intersect with input image
	boundingBox.intersect( this->input0().info() );
	this->input0().request( boundingBox.x(),
						boundingBox.y(),
						boundingBox.r(),
						boundingBox.t(),
						channels,
						count );
	
}

	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------
																		
//	get bounding box from given $x, $y, $r, $t
DD::Image::Box YnxRollingShutterNode::getBoundingBox( int x, int y, int r, int t )
{
	//	normalize given input into 0 - 1
	double xIn_unit = double( x ) / this->format().width();
	double yIn_unit = double( y ) / this->format().height();
	double rIn_unit = double( r ) / this->format().width();
	double tIn_unit = double( t ) / this->format().height();
	
	double xOut_unit, yOut_unit, rOut_unit, tOut_unit;
	
	//	in case undistort do compute bounding box by warp position with undistort
	if( this->isUndistort )
	{
		this->computeUndistortBoundingBox( xIn_unit, yIn_unit, rIn_unit, tIn_unit, &xOut_unit, &yOut_unit, &rOut_unit, &tOut_unit );
	}
	
	//	in case undistort do compute bounding box by warp position with distort
	else
	{
		this->computeDistortBoundingBox( xIn_unit, yIn_unit, rIn_unit, tIn_unit, &xOut_unit, &yOut_unit, &rOut_unit, &tOut_unit );
	}
	
	//	unnormalize the position  after warped
	double xOut_pixel = xOut_unit * this->format().width();
	double yOut_pixel = yOut_unit * this->format().height();
	double rOut_pixel = rOut_unit * this->format().width();
	double tOut_pixel = tOut_unit * this->format().height();
	
	//	return the bounding box
	return DD::Image::Box( int( floor( xOut_pixel ) ) - 2, int( floor( yOut_pixel ) ) - 2, int( ceil( rOut_pixel ) ) + 2, int( ceil( tOut_pixel ) ) + 2 );
}

	//---------------------------------------------------------------------
	//	private member functions
	//---------------------------------------------------------------------

//	compute bounding box by sample the position and warp to get bounding box
void YnxRollingShutterNode::computeDistortBoundingBox( double x, double y, double r, double t, double *x_ret, double *y_ret, double *r_ret, double *t_ret, int numSampleX /*= 32*/, int numSampleY /*= 32*/ )
{
	//	initialize the output x, y, r, t ( r = max number of x ( width ), t = max number of y ( height ) )
	//		to compute and get min/max value to decide as bounding box
	*x_ret = HUGE_VAL;
	*y_ret = HUGE_VAL;
	*r_ret = -HUGE_VAL;
	*t_ret = -HUGE_VAL;
	
	//	compute distant bewteen x, y and r, t
	double dx = r - x, 
			dy = t - y;
	
	//	compute the position from sampling x, y
	for( int i = 0 ; i < numSampleX ; i++ )
	{
		//	compute sample position
		double sampleX = x + dx * double( i ) / numSampleX;
		double sampleR = x + dx * double( i + 1 ) / numSampleX;
		
		//	try warp and get min, max position
		this->getDistortMinMaxBoundingBox( sampleX, y, x_ret, y_ret, r_ret, t_ret );
		this->getDistortMinMaxBoundingBox( sampleR, t, x_ret, y_ret, r_ret, t_ret );
	}
	for( int j = 0 ; j < numSampleY ; j++ )
	{
		//	compute sample position
 		double sampleY = y + dy * double( j + 1 ) / numSampleY; 
 		double sampleT = y + dy * double( j ) / numSampleY;
		
		//	try warp and get min, max position
		this->getDistortMinMaxBoundingBox( x, sampleY, x_ret, y_ret, r_ret, t_ret );
		this->getDistortMinMaxBoundingBox( r, sampleT, x_ret, y_ret, r_ret, t_ret );
	}
}
void YnxRollingShutterNode::computeUndistortBoundingBox( double x, double y, double r, double t, double *x_ret, double *y_ret, double *r_ret, double *t_ret, int numSampleX /*= 32*/, int numSampleY /*= 32*/ )
{
	//	initialize the output x, y, r, t ( r = max number of x ( width ), t = max number of y ( height ) )
	//		to compute and get min/max value to decide as bounding box
	*x_ret = HUGE_VAL;
	*y_ret = HUGE_VAL;
	*r_ret = -HUGE_VAL;
	*t_ret = -HUGE_VAL;

	//	compute distant bewteen x, y and r, t	
	double dx = r - x, 
			dy = t - y;
	
	//	compute the position from sampling x, y
	for( int i = 0 ; i < numSampleX ; i++ )
	{
		//	compute sample position
		double sampleX = x + dx * double( i ) / double( numSampleX );
		double sampleR = x + dx * double( i + 1 ) / double( numSampleX );
		
		//	try warp and get min, max position
		this->getUndistortMinMaxBoundingBox( sampleX, y, x_ret, y_ret, r_ret, t_ret );
		this->getUndistortMinMaxBoundingBox( sampleR, t, x_ret, y_ret, r_ret, t_ret );
	}
	
	for( int j = 0 ; j < numSampleY ; j++ )
	{
		//	compute sample position
 		double sampleY = y + dy * double( j + 1 ) / double( numSampleY );
 		double sampleT = y + dy * double( j ) / double( numSampleY );
		
		//	try warp and get min, max position
		this->getUndistortMinMaxBoundingBox( x, sampleY, x_ret, y_ret, r_ret, t_ret );
		this->getUndistortMinMaxBoundingBox( r, sampleT, x_ret, y_ret, r_ret, t_ret );
	}
	
}

//	get bounding box from given input pixel position by do warp position then
//			check is the output position is exceed the min, max or not
//			if exceed change the value to the new one
void YnxRollingShutterNode::getDistortMinMaxBoundingBox( double x, double y, double *x_ret, double *y_ret, double *r_ret, double *t_ret )
{
	//	construct ynx vector2 ( rolling shutter engines use ynx vector2 to warp position )
	//		for input and output
	Vector2 inputVec = Vector2( x, y );
	Vector2 outputVec;
	
	//	try to warp the position
	try
	{
		outputVec = this->rollingShutterLensDistortionEngine.removeWarp( inputVec );
	}
	catch( ynxValueException &e )
	{
		//	can't warp so return
		return;
	}
	
	//	compare to get min, max of bounding box
	*x_ret = std::min( *x_ret, outputVec.x );
	*y_ret = std::min( *y_ret, outputVec.y );
	*r_ret = std::max( *r_ret, outputVec.x );
	*t_ret = std::max( *t_ret, outputVec.y );
}
void YnxRollingShutterNode::getUndistortMinMaxBoundingBox( double x, double y, double *x_ret, double *y_ret, double *r_ret, double *t_ret )
{
	//	construct ynx vector2 ( rolling shutter engines use ynx vector2 to warp position )
	//		for input and output	
	Vector2 inputVec = Vector2( x, y );
	Vector2 outputVec;
	
	//	try to warp the position
	try
	{
		outputVec = this->rollingShutterLensDistortionEngine.applyWarp( inputVec );
	}
	catch( ynxValueException &e )
	{
		//	can't warp so return
		return;
	}
	
	//	compare to get min, max of bounding box	
	*x_ret = std::min( *x_ret, outputVec.x );
	*y_ret = std::min( *y_ret, outputVec.y );
	*r_ret = std::max( *r_ret, outputVec.x );
	*t_ret = std::max( *t_ret, outputVec.y );
}


/*! This is a function that creates an instance of the operator, and is
   needed for the Iop::Description to work.
 */
static DD::Image::Iop* YnxRollingShutterNodeCreate(Node* node)
{
  return new YnxRollingShutterNode(node);
}

const DD::Image::Iop::Description YnxRollingShutterNode::description ( CLASS, "Yannix/YnxRollingShutterNode",
                                                     					YnxRollingShutterNodeCreate );

//---------------------------------------------------------------------
//
//	END CLASS YnxRollingShutterNode MEMBER FUNCTIONS
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

