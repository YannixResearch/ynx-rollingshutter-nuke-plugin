README

This folder contains the source code for Yannix's Rolling Shutter Nuke Plugin. Run the makefile for your Nuke platform.

The compilation step creates binary files, YnxRollingShutterNode.so and libynxlensdistortionengines.so . Copy these two files to your Nuke plugin folder.

This folder also contains a sample test folder containing a Nuke file that utilizes this plugin. 
If the plugin was successfully compiled and installed, the Nuke script should open without any errors. The Nuke script has a checkerboard node (#1) 
that is passed into a rolling shutter node (#2) and then through another rolling shutter node that inverts the rolling shutter (#3). 
There is a image reader node (#4) that is the output of #2 that was generated at Yannix. To test the functionality of the Yannix Rolling Shutter plugin, 
compare results #2 and #4 to confirm what is produced on your system matches what is produced at Yannix. 
To confirm that the rolling shutter inversion is also functioning correctly, compare #1 and the output from #3 to see that they match exactly.
