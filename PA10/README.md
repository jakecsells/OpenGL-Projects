#OpenGL: Lighting

Documentation
-------------

###Controls

**RightClick** - Open Menu

**Mene Selection** - Toggle specific lights and views

###What we did

This project was completed by Jessie Smith, Jake Sells, and Paul Squire.
It was very difficult to complete because we could not output the compile
errors that occur in our shader files. We had to hard code much of the
shaders because we had difficulty getting data into the shader.

###Things to Note:

Our spotlight will move when the camera is moved for some reason, so it will
not show on other views than the default. We will need to fix this.


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
