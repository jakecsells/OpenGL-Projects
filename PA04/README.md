#OpenGL: Creating a Model Loader

Documentation
-------------

###Controls

Controls are removed for this lab. It is a show case of the model loader.

So you don't have to go into the source code to change the name of the obj
file, name your obj file: "test.obj". Or you can change the name of the obj
file in the global variable **OBJ_FILE**.

###What I did

I followed the tutorial that was given on the website. Most of the code
was used as a guide for what I actually implemented. 

The main loops for the loading OBJ files are similar to that of the
tutorial's. After the loops pulls in the information, it is sent back
to the Vertex struct. The color is random for each face as well.

**UPDATE:** I added the model to show a complex model that can be loaded
into my program.

My model loader can only load triangulated models that are exported without
texture mapping. I have added error checking when models that cannot be
loaded are attempted to be loaded.

I understand what a robust model loader should do, and I repsect it, but
other model loaders are just so much better than mine, that I decided to
keep mine simple.

###Things to Note

I did not submit my blender model because we were told not to.
If this is required, please let me know and I can upload it to my
github repository.


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
