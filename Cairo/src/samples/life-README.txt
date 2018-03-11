Hi,
 this is my version of Conway's game of life.
I came up with the algorithm while bored in lecture.
I'm sure the implementation is not original, nor is it optimal.

It uses cairo for the drawing operations, double buffered
using pixmaps.  All cairo ops are in 'cgolwin.cpp', in the 
render function.

the game of life code itself is seperate, and is in cgol.h/cpp.
only thing is that cgolwin.cpp expects its list of cells back
in the proper format.

Any questions/comments/patches should go to

andrew.chant@utoronto.ca


