Adding Shader Loader to OpenGL Example
======================================

Building This Example
---------------------

*This example requires GLM*
*On ubuntu it can be installed with this command*

>$ sudo apt-get install libglm-dev

To build this example just 

>$ cd build
>$ make

The excutable will be put in bin

Mac OS X Things
---------------

*Mac OS X requires some changing of the headers*

*Also std::chrono may or may not work on OS X*

*Should that be the case use gettimeofday*

Things to Note
--------------

File I/O length is shorter than I think it should be.

Added a correction of length - 1 to compile the shaders
correctly.

If the shader is not compiling correctly please check
this offset correction.
