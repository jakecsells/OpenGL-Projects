#OpenGL: Air Hockey

Documentation
-------------

###Controls

**Up Arrow** - Move player away from camera

**Down Arrow** - Move player towards camera

**Left Arrow** - Move player left

**Right Arrow** - Move player right

**Space** - "Jump" the player

**W Key** - Move computer (player2) away from camera

**S Key** - Move computer (player2) towards camera

**A Key** - Move computer (player2) left

**D Key** - Move computer (player2) right

**ESC** - Quit the program

**Mouse Movement** - Move the player in the direction of the mouse

**Touch Screen** - Move the player in the direction of the touch

**RightClick** - Open Menu

###What we did

This project was completed by Jessie Smith, Jake Sells, and Paul Squire.
We used large models because it went with our theme. We also have the puck 
as a paddle because we ran out of time and forgot our puck on a different
computer.

We have an AI that does whatever it wants. The corners and goal cause the
puck to fly back into the middle.

###Things to Note

All of this was pretty messed up, but it does work and the physics
does work pretty well. So that's it.


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
