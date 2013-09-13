#OpenGL: Menus, Keyboard, and Mouse Interaction

Documentation
-------------

###Controls

**Right Mouse Click**--Open simple menu

**Left Mouse Click**--Reverse rube rotation

**W Key**--Increase cube rotation speed by 10.0 degrees

**S Key**--Decrease cube rotation speed by 10.0 degrees

**A Key**--Rotate cube clockwise

**D Key**--Rotate cube counter-clockwise

**ESC**--Quit the program

###What I did

I used a global called ROTATE_SPEED, which would change
the speed of the rotation when using the 'w' and 's' keys
and it will toggle reversing the rotation by multiplying
the speed by -1.0f. The rotation can change depending
on the 'a' and 'd' keys.

###Things to Note

Correction of file length minus 1 from the previous lab is
removed and added with a null terminator.


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
