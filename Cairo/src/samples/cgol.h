/* cgol.h
 *  - defines gameoflife class & helpers
 *  (c) 2003 Andrew Chant
 *  licensed under GPL
 *
 *   This file is part of cgol.
    cgol is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    cgol is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cgol; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef CGOL_H
#define CGOL_H

#include <list>
using std::list;

/*
class myapp: public ctkapp()
{
public:
	myapp();
	void eventloop(void);
};
*/
/* cell structure for use by gameoflife */
struct cell
{
	unsigned int dna; // alive & color
	int x;
	int y;
};

/* provide the logic for game of life */
class gameoflife 
{
public:
	gameoflife(int,int);
	void advance();
	list<struct cell> * getboard();
private:
	int rows, cols;
	unsigned int * score;
	list<struct cell> * cells;
};

const unsigned int R1 = (1 << 4);
const unsigned int R2 = (1 << 5);
const unsigned int B1 = (1 << 6);
const unsigned int B2 = (1 << 7);
const unsigned int G1 = (1 << 8);
const unsigned int G2 = (1 << 9);
const unsigned int ALIVE = (1 << 10);
const unsigned int DNAMASK = R1 | R2 | B1 | B2 | G1 | G2 | ALIVE;
#endif // CGOL_H
