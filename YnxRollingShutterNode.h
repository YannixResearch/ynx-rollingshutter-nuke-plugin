//---------------------------------------------------------------------
//
//	Program written by Channarong (Aon) Khamphusa for
//		Yannix 2018-06-13
//	Copyright Yannix (Thailand) Co., Ltd 2018. All rights reserved
//
//---------------------------------------------------------------------
#if !defined(___YnxRollingShutterNode_h)
#define ___YnxRollingShutterNode_h

//---------------------------------------------------------------------
//
//	STANDARD INCLUDES
//
//---------------------------------------------------------------------

//	Nuke
#include <DDImage/Iop.h>
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Filter.h>

//---------------------------------------------------------------------
//
//	NON-STANDARD INCLUDES
//
//---------------------------------------------------------------------

//	yannix lens distortion engine
#include "RollingShutterLensDistortionEngine.h"

//---------------------------------------------------------------------
//
//	DEFINES
//
//---------------------------------------------------------------------

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

//	Help for this node
const char * const HELP = "YnxRollingShutterNode\n"
"\n"
"Apply or remove rolling shutter distortion on an image. Since rolling shutter distortion is "
"caused by the fact that different scanlines of the image are captured at different times, this "
"node assumes the scanline at the center of the image is captured at zero time offset and top and "
"bottom of image are captured correspondingly forward or backward in time. This node does a simple "
"first order model of the motion in the plate by specifying the motion at top-left, top-middle, top-"
"right, bottom-left, bottom-middle, and bottom-right of frame. \"rollingShutterRatio\" is the "
"temporal offset of top and bottom scanlines as a fraction of the motion to previous/next frames. "
"A rolling shutter ratio of +1 results in the top of the image warped to the position at next frame, "
"and the bottom of the image warped to the position of the previous frame. A rolling shutter ratio "
"of -1 results in the top of the image warped to the position at previous frame, and the bottom of "
"the image warped to the position of the next frame.";

//	command for this node when create in nuke ( just like node name for creating in nuke )
const char * const CLASS = "YnxRollingShutterNode";

//---------------------------------------------------------------------
//	Notes - Iop mean Image operation class for more information
//				/opt/Nuke11.0v2/include/DDImage/Iop.h
//
// using namespace std;
// using namespace DD::Image;
//---------------------------------------------------------------------
//
//	class YnxRollingShutterNode
//
//---------------------------------------------------------------------
class YnxRollingShutterNode : public DD::Image::Iop
{
	//---------------------------------------------------------------------
	//	public member classes
	//---------------------------------------------------------------------
	public:

	//---------------------------------------------------------------------
	//	public member data
	//---------------------------------------------------------------------
	public:
		
		//	description for this node
		static const DD::Image::Iop::Description description;

	//---------------------------------------------------------------------
	//	protected member data
	//---------------------------------------------------------------------
	protected:
		
		//----------------------------------
		//	nuke node parameter
		//
		
		//	distortionengine class
		RollingShutterLensDistortionEngine rollingShutterLensDistortionEngine;
		
		//	is undistort
		bool isUndistort;
	
	//---------------------------------------------------------------------
	//	private member data
	//---------------------------------------------------------------------
	private:
	
	//---------------------------------------------------------------------
	//	public contructors/destructors
	//---------------------------------------------------------------------
	public:
		YnxRollingShutterNode( Node *node );
		
		virtual ~YnxRollingShutterNode();
		
	//---------------------------------------------------------------------
	//	public access functions
	//---------------------------------------------------------------------
	public:
	
	//---------------------------------------------------------------------
	//	public member functions
	//---------------------------------------------------------------------
	public:
	
		//----------------------------------
		//	Nuke methods
		//
		
		//	This function is do all work
		virtual void engine( int y, int x, int rowSize, DD::Image::ChannelMask channelMask, DD::Image::Row &out );
		
		/*! Return help information for this node. This information is in the
          pop-up window that the user gets when they hit the [?] button in
          the lower-left corner of the control panel.

          The first character should not be a punctuation mark, they are all
          reserved for future use.
       */
		virtual const char* node_help() const
		{ return HELP; }
		

		/*! \fn const char* Op::Class() const;

         Return the command name that will be stored in Nuke scripts. This
         must be unique for each different function and must be the same
         string (ie the same pointer) as is in the Op::Description object
         that constructed this instance.

         If you don't plan to make a Nuke operator you can have this return
         null.
       */
		virtual const char* Class() const
		{ return CLASS; }
		
		//	This function is used for validate parameter value
		void _validate( bool for_real );
		
		//	This function is used for request region of data before send into engine func
		void _request(int x, int y, int r, int t, DD::Image::ChannelMask channels, int count);
		
		//	Function for create knob ( knobs are fundamentals of all user interface elements available to NUKE Ops. )
		//		For more information https://learn.foundry.com/nuke/developers/63/ndkdevguide/knobs-and-handles/index.html
		virtual void knobs( DD::Image::Knob_Callback f );
				
	//---------------------------------------------------------------------
	//	public operator overloads
	//---------------------------------------------------------------------
	public:
	
	//---------------------------------------------------------------------
	//	protected member functions
	//---------------------------------------------------------------------
	protected:
	
		//	get bounding box from given $x, $y, $r, $t
		DD::Image::Box getBoundingBox( int x, int y, int r, int t );
	
	//---------------------------------------------------------------------
	//	private member functions
	//---------------------------------------------------------------------
	private:
		
		//	compute bounding box by sample the position and warp to get bounding box
		void computeDistortBoundingBox( double x, double y, double r, double t, double *x_ret, double *y_ret, double *r_ret, double *t_ret, int numSampleX = 32, int numSampleY = 32 );
 		void computeUndistortBoundingBox( double x, double y, double r, double t, double *x_ret, double *y_ret, double *r_ret, double *t_ret, int numSampleX = 32, int numSampleY = 32 );
		
		//	get bounding box from given input pixel position by do warp position then
		//			check is the output position is exceed the min, max or not
		//			if exceed change the value to the new one
		void getDistortMinMaxBoundingBox( double x, double y, double *x_ret, double *y_ret, double *r_ret, double *t_ret );
		void getUndistortMinMaxBoundingBox( double x, double y, double *x_ret, double *y_ret, double *r_ret, double *t_ret );
	
};
//---------------------------------------------------------------------
//	END class YnxRollingShutterNode
//---------------------------------------------------------------------

#endif
//---------------------------------------------------------------------
//
//	EOF
//
//---------------------------------------------------------------------

