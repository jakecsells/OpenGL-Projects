#OpenGL: Adding a Moon to the Planet

Documentation
-------------

###Controls

**Right Mouse Click** - Open simple menu

**Left Mouse Click** - Reverse rube rotation

**Up Arrow** - Increase 'planet' rotation speed by 10.0 degrees

**Down Arrow** - Decrease 'planet' rotation speed by 10.0 degrees

**Left Arrow** - Rotate the 'planet' counter-clockwise

**Right Arrow** -Rotate the 'planet' clockwise

**W Key** - Increase 'moon' rotation speed by 10.0 degrees

**S Key** - Decrease 'moon' rotation speed by 10.0 degrees

**A Key** - Rotate 'moon' counter-clockwise

**D Key** - Rotate 'moon' clockwise

**ESC** - Quit the program

###What I did

Just as before by adding global variables to control the cube
rotation, I added more globals (uh oh), to keep track of the new
'moon' model.
The 'moon' is scaled and translated together. Also, the 'moon' is
translated before the 'planet' is rotated so the translation does not
inherit the rotation from the 'planet'.


###Things to Note

The null terminator is now actually a null terminator and not
a new line character... how embarrassing.


Building The Project
--------------------

*This example requires GLM*
*On ubuntu it can be installed with this command*

>$ sudo apt-get install libglm-dev

To build this example just 

>$ cd build
>$ make

The excutable will be put in bin

###Mac OS X

*Mac OS X requires some changing of the headers*

*Remove*
>CC=g++

*Add*
>CC=clang++

*std::chrono may need to be replaced with gettimeofday*
