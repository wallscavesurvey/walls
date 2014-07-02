/* global.h --- prototypes for functions and global variables 
 *
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 *
 */
#ifndef GLOBAL_E_H
#define GLOBAL_E_H

#include "global_ed.h"

/****** Function prototypes */

/* lossless.c */
void lossless_doscanline(pixel *psl, pixel *sl, int no);

/* initialize.c */
void prepareLUTs();
void init_stats();

#endif
