/* cgol.cpp
 *  - implementation of gameoflife class
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


#include "cgol.h"
#include <iostream>
using std::cout;
using std::endl;

gameoflife::gameoflife(int r, int c)
{
	rows = r;
	cols = c;
	srand(time(NULL));
	/* create board layout structures */
	score = (unsigned int *)malloc(rows*cols*sizeof(unsigned int));
	cells = new list<struct cell>;
	/* initialize to random */
	for (int x = 0; x < cols; x++)
		for (int y = 0; y < rows; y++)
		if (random() % 2)
		{
			cell newcell;
			newcell.x = x;
			newcell.y = y;
			newcell.dna = ALIVE; // 1st gen black
			cells->push_back(newcell);
		}
}

void gameoflife::advance()
{
	/* clear score matrix */
	memset(score,0,rows*cols*sizeof(unsigned int));
	/* take list of 'live' cells, traverse, create score matrix */
	for (list<struct cell>::iterator i = cells->begin();
			i != cells->end(); i++)
	{
		score[i->y*cols + i->x] += i->dna; // mark alive & color
		// there has got to be a more efficient way of doing 
		// scoring
		if (i->y > 0) // go above one
		{
			//up is ok
			score[(i->y-1)*cols + i->x] +=1;

			if (i->x > 0) // up & left ok
				score[(i->y-1)*cols + i->x - 1 ] +=1;
			if (i->x + 1 < cols) // up & right ok
				score[(i->y-1)*cols + i->x + 1] +=1;
		}
		
		if (i->x > 0) // left ok
			score[i->y*cols + i->x -1] +=1;
		if (i->x + 1 < cols) // right ok
			score[i->y*cols + i->x +1] +=1;
		if ( (i->y + 1) < rows) // go down one
		{
			// down is ok
			score[(i->y +1) * cols + i->x] +=1;
			if (i->x > 0) // down & left ok
				score[(i->y + 1) * cols + i->x -1] +=1;
			if (i->x + 1 < cols) // down & right ok
				score[(i->y + 1) * cols + i->x + 1] +=1;
		}
	} 
	/* create new cell color (dna) for this round */
	unsigned int currdna = ALIVE;
	if (random() % 2) currdna |= R1;
	if (random() % 2) currdna |= R2;// red comp 
	if (random() % 2) currdna |= B1;
	if (random() % 2) currdna |= B2;
	if (random() % 2) currdna |= G1;
	if (random() % 2) currdna |= G2;
	
	/* from score matrix, create a new list of live cells */
	list<struct cell> * newcells = new list<struct cell>;
	for (int i = 0; i < rows * cols; i++)
	{	// cases: if score[i] == 3, score[i] & DNAMASK != 0
		if (score[i] == 3)
		{
			cell newcell;
			newcell.x = i % cols;
			newcell.y = i / cols;
			newcell.dna = currdna; 
			newcells->push_back(newcell);
		}
		else if ( (score[i] & ALIVE) && ( ((score[i] & ~DNAMASK) == 2) || ((score[i] & ~DNAMASK) == 3)) )
		{
			cell newcell;
			newcell.x = i % cols;
			newcell.y = i / cols;
			newcell.dna = score[i] & DNAMASK;
			newcells->push_back(newcell);
		}
	}
	/* replace old list with new list */
	delete cells;
	cells = newcells;
}

list<struct cell> * gameoflife::getboard()
{
	/* return current list of live cells */
	// cout << "returning " << cells->size() << " cells\n";
	return cells;
}

/*
int main (void)
{
	gameoflife MyGame(200,300);
	list<struct cell> * rval = MyGame.getboard();
	while (rval->begin() != rval->end())
	{
		MyGame.advance();
		rval = MyGame.getboard();
	}
	return 0;
} */
