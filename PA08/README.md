#OpenGL: Adding Physics

Documentation
-------------

###Controls

**Up Arrow** - Move cylinder away from camera

**Down Arrow** - Move cylinder towards camera

**Left Arrow** - Move cylinder left

**Right Arrow** - Move cylinder right

**Space** - "Jump" the sphere

**W Key** - Move sphere away from camera

**S Key** - Move sphere towards camera

**A Key** - Move sphere left

**D Key** - Move sphere right

**ESC** - Quit the program

###What we did

We redid our structure of the model loading and storage, to
better allow for the integration of bullet physics engine.

We deprecated the textres from the previous lab, and we will
be adding them back at a later time.

This assignment was completed by Jessie Smith, Jake Sells, and
Paul Squire.

###Things to Note

The sphere and cylinder are quite small, but we will be fixing that
at a later time as well. But we thought they were so cute, we left
them teeny tiny.


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
