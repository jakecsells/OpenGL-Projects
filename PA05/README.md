#OpenGL: Creating a Model Loader

Documentation
-------------

###Controls

Controls are removed for this lab. It is a show case of the model loader.

To load different models, add the model name as an agrument to the program.
For example:

>$ ./Matrix bunny.obj

###What I did

Our team, comprised of Jake Sells (me), Jessie Smith, and Paul Squire.
We used a new loadOBJ function, we used assimp and importer to read the file
and triangulate the obj file. Once that is done, we loaded the faces and then
the geometry and later the color.

The color is random for each vertex, which of course makes the model look a
litle wierd.

###Things to Note

We did not add any obj files to load. So you can use your own.


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
