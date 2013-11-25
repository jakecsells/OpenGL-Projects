#OpenGL: Labyrinth

Documentation
-------------

###Controls

**RightClick** - Open Menu

**W** - Tilt the board forward

**S** - Tilt the board backward

**A** - Tilt the board to the left

**D** - Tilt the board to the right

**Mouse** - Use mouse movement to move the board (when toggled in menu)

**Menu Options**
- Play Game
- Play Tutorial
- Restart
- Pause/Resume
- Mouse Control
- Toggle Point
- RAVE MODE!
- Camera: Front View
- Camera: Top View
- Camera: Follow the Ball
- Quite

###Wednesday Update

This project was completed by Jessie Smith, Jake Sells, and Paul Squire.
We were able to get a maze with a bottom and holes and a ball to roll around in the maze.
This is all working, but at some points the ball will fall through, which we will
try to fix by the time the demo comes by.

###What we Did

We increased the model mesh scale in time for the demo, because it seemed that increasing
the size would make the collision more stable. We used a 3d triangle mesh with bullet to make the model operate correctly. We added some extra credit things, like RAVE MODE and mouse control.
Overall, the project was a success.

###Things to Note:

The ball will occasionally fall through the bottom. The board can flip over entirely if you want it to. This is considered an additional degree of freedom for the player.

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
