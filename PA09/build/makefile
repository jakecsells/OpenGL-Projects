# Linux
CC=g++

# For Macs uncomment the next line and comment out the previous one
#CC=clang++
# Assuming you want to use a recent compiler

# Compiler flags
LIBS= -lglut -lGLEW -lGL -lassimp -lIL -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath
CXXFLAGS= -g -Wall -std=c++0x -I/usr/include/bullet/

all: ../bin/Matrix

../bin/Matrix: ../src/main.cpp
	$(CC) $(CXXFLAGS) ../src/main.cpp -o ../bin/Matrix $(LIBS)

clean:
		rm ../bin/Matrix
