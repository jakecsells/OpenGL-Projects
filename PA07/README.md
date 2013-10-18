#Model Loading with Assimp and Textures 

Documentation
-------------

###Controls

To load different models, add the model name and the texture as an agrument to the program.
For example:

>$ ./Matrix bunny.obj whitefur.png

###What we Did

Our team, comprised of Jake Sells (me), Jessie Smith, and Paul Squire.
We used a new loadOBJ function, we used assimp and importer to read the file
and triangulate the obj file. Along with the verticies and other important
obj information, we kept the uv information as well.

There is still a problem with multiple meshes, where loading a model with multiple
meshes will sometimes break some of the meshes and only render a fraction of the mesh.

###Things to Note

We did not add any obj files to load. So you can use your own.
If you are having difficulty seeing a model, please use a single-mesh model.
This assignment was turned in late, sorry about that.


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
