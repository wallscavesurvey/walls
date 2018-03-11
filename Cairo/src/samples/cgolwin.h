/* cgolwin.h
 *  - gameoflifeWin class described
 *  - uses homebrew 'ctk' toolkit
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

#include "ctk.h"
#include "cgol.h"
#include <cairo.h>
class gameoflifeWin : public Ctkwin
{
public:
	gameoflifeWin(Ctkapp *, int, int, int, int, bool);
	void event(XEvent *);
	static void advanceTimer(void *);
	void render();
//These two really shouldn't be private
//but need to be accessed from a passed pointer to this 
//given to advanceTimer.  I really need to find a better way
//of kludging timers.
	list<struct cell> * cells;
	gameoflife * game;
private:
	void initBuffers(void);
	void freeBuffers(void);
	void initBoard(void);
	cairo_t *onscreen_cr;
	cairo_surface_t *onscreen, *buffer, *grid, *cellpix;
	cairo_matrix_t matrix;
	int rows, cols, width, height;
	bool use_cairo_mask;
};

