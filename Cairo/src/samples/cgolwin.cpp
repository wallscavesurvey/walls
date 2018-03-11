/* cgolwin.cpp
 *  - implementation of gameoflifeWin
 *  - render does most of the important stuff
 *   - event takes the X events
 *
 *  (c) 2003 Andrew Chant
 *  licensed under GPL
 *
 *   This file is part of cgol.
 *  cgol is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  cgol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with cgol; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <iostream>
#include <assert.h>
#include "cgolwin.h"
using std::cout;

gameoflifeWin::gameoflifeWin(Ctkapp * app, int wi, int h, int r, int c, bool s)
 :Ctkwin(app,wi,h), buffer(NULL), rows(r), cols(c), width(wi), height(h),
 use_cairo_mask(s)
{
	game = new gameoflife(rows, cols);

	onscreen = cairo_xlib_surface_create(dpy, w, 
			DefaultVisual(dpy, DefaultScreen(dpy)),
			wi, h);
	onscreen_cr = cairo_create (onscreen);
	cairo_set_source_rgb (onscreen_cr, 0, 0, 0);
	cairo_paint (onscreen_cr);
	
	// MAX 24 fps, really just means 1/24 s between advancements
	app->regtimer(0,1000000/24,-1,advanceTimer, (void *)this);
}

void gameoflifeWin::initBuffers()
{
	buffer = cairo_surface_create_similar (onscreen,
						CAIRO_CONTENT_COLOR_ALPHA,
						width, height);
	grid = cairo_surface_create_similar (onscreen,
						CAIRO_CONTENT_COLOR_ALPHA,
						width, height);
	if (use_cairo_mask)
		cellpix = cairo_surface_create_similar (onscreen,
						CAIRO_CONTENT_ALPHA,
						width/cols+1, height/rows+1);

	initBoard();
}

void gameoflifeWin::freeBuffers(void)
{
	if (buffer)
	{
		cairo_surface_destroy (buffer);
		cairo_surface_destroy (grid);
		if (use_cairo_mask)
			cairo_surface_destroy (cellpix);
		buffer = grid = cellpix = NULL;
	}
}

void gameoflifeWin::initBoard()
{
	cairo_t *cr = cairo_create (grid);

	cairo_scale(cr, (double)width/(double)cols,
			(double)height/(double)rows); 
	cairo_get_matrix (cr, &matrix);

	cairo_set_line_width(cr, 0.1);
	cairo_set_source_rgb(cr, .8, .8, .8);

	for (int col = 0; col <= cols; col++)
	{
		cairo_move_to(cr, col, 0);
		cairo_rel_line_to(cr, 0, rows);
	}
	for (int row = 0; row <= rows; row++)
	{
		cairo_move_to(cr, 0, row);
		cairo_rel_line_to(cr, cols, 0);
	}
	cairo_stroke(cr);
	cairo_destroy(cr);
	
	if (use_cairo_mask)
	{
		cr = cairo_create (cellpix);
		cairo_set_matrix (cr, &matrix);
		cairo_arc(cr, .5,  .5, .5, 0, 2*M_PI);
		cairo_fill(cr);
		cairo_destroy (cr);
	}
	
}

void gameoflifeWin::event(XEvent * e)
{
	switch (e->type)
	{
	case Expose:
		if (((XExposeEvent *)e)->count == 0) // NOT RELIABLE :(
		{
			if (!XPending(dpy)) //This is an ugly hack
					render();
		}
		break;
	case ConfigureNotify:
		freeBuffers();
	case CreateNotify:
		width = ((XConfigureEvent *)e)->width;
		height = ((XConfigureEvent *)e)->height;
		cairo_xlib_surface_set_size (onscreen, width, height);
		break;
	default:
		//cout << "Unhandled Event " << e->type << "\n";
		;
	}
}

void gameoflifeWin::advanceTimer(void * obj)
{
	gameoflifeWin * game = (gameoflifeWin *)obj;
	game->game->advance(); // yeah ok bad naming shoot me
	game->render();
}

void gameoflifeWin::render()
{
	cairo_t *cr;

	if (!buffer)
		initBuffers();

	
	cells = game->getboard();

	cr = cairo_create (buffer);

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_paint (cr);

	// Copy gen'd grid to buffer
	cairo_set_source_surface (cr, grid, 0, 0);
	cairo_paint (cr);

	cairo_set_matrix (cr, &matrix);
	/* note: all cairo ops operate on a rowxcol coordinate system
	   	 the cairo transform matrix handles resizing etc */

	for (list<struct cell>::const_iterator i = cells->begin();
		i != cells->end(); i++)
	{
		double red,green,blue;
		red = green = blue = 0;
		//Note colour weighting avoids white (Max < 1 of each)
		if (i->dna & B1) blue += .50;
		if (i->dna & B2) blue += .25;
		if (i->dna & R1) red += .50;
		if (i->dna & R2) red += .25;
		if (i->dna & G1) green += .50;
		if (i->dna & G2) green += .25;
		
		if (use_cairo_mask)
		{
			double x, y;
			cairo_save (cr);
			x = i->x;
			y = i->y;
			cairo_user_to_device (cr, &x, &y);
			cairo_identity_matrix (cr);
			cairo_set_source_rgb (cr, red, green, blue);
			cairo_mask_surface (cr, cellpix, x, y);
			cairo_restore (cr);
		}
		else 
		{ 
			cairo_set_source_rgb (cr, red, green, blue);
			cairo_arc (cr, i->x + .5, i->y + .5, .5, 0, 2*M_PI);
			cairo_fill (cr);
		}
	}

	cairo_destroy (cr);

	cairo_set_source_surface (onscreen_cr, buffer, 0, 0);
	cairo_paint (onscreen_cr);
}
