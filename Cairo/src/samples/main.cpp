/* main.cpp
 *  - throws all the objects together
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

#include <iostream>
#include "cgolwin.h"
#include "ctk.h"
using std::cout;
using std::endl;
void usage(char * name)
{
	std::cout << "Usage: " << name << " [-w width] [-h height] [-c columns] [-r rows] [-s (use mask surface)]\n";
}

int main(int argc, char * argv[])
{
	int width = 200;
	int height = 200;
	int rows = 20;
	int cols = 20;
	char action;
	int traverse = 0;
	bool s = false;
	
	while (++traverse < argc)
	{
		if (argv[traverse][0] == '-')
			action = argv[traverse][1];
		else
			action = argv[traverse][0];
		switch(action)
		{
		case 'w':
			if (++traverse > argc)
			{
				usage(argv[0]);
				exit(0);
			}
			width = atoi(argv[traverse]);
			break;
		case 'h':
			if (++traverse > argc)
			{
				usage(argv[0]);
				exit(0);
			}
			height = atoi(argv[traverse]);
			break;
		case 'r':
			if (++traverse > argc)
			{
				usage(argv[0]);
				exit(0);
			}
			rows = atoi(argv[traverse]);
			break;
		case 'c':
			if (++traverse > argc)
			{
				usage(argv[0]);
				exit(0);
			}
			cols = atoi(argv[traverse]);
			break;
		case 's':
			s = true;
			break;
		default:
			usage(argv[0]);
			exit(0);
			break;
		}
		
	}
	
	Ctkapp myapp;
	gameoflifeWin mywin(&myapp,width,height, rows, cols, s);
	myapp.go();
	return 0;
}
