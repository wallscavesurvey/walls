/* global.h --- prototypes for functions and global variables 
 *
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 */

#ifndef GLOBAL_D_H
#define GLOBAL_D_H

#include "global_ed.h"

void init_stats();

/* lossless_d.c*/
int lossless_undoscanline(	pixel *psl,			/* previous scanline */
							pixel *sl,			/* current scanline */
							int no);			/* number of values in it */

#endif
