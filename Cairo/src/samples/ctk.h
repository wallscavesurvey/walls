/* ctk.h - 
 *  - description of homebrew toolkit classes
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

#ifndef CTK_H
#define CTK_H

#include <X11/Xlib.h>
#include <sys/select.h>
#include <sys/times.h>
#include <unistd.h>
#include <list>

using std::list;

class Ctkwin;

class Ctktimer
{
public:
	Ctktimer (int, int, int, void (void *), void *);
	struct timeval full;
	struct timeval rem;
	int reps;
	void (*fn)(void *);
	void * obj;
	bool operator< (const Ctktimer) const;
};

class Ctkapp       
{
public:
	Ctkapp();
	int go(void);
	void reg(Ctkwin *);
	void regtimer(int, int, int, void (void *), void *);
	Display *const getDisplay(void) {return dpy;};
private:
	void event(XEvent *);
	void setTimeout(void);
	void updateTimers(void);
	list <Ctkwin *> windows;
	list <Ctktimer> timers;
	//Xlib related stuff
	XEvent e;
	Display * dpy;
	//For select
	struct timeval * timeout;
	fd_set reads;
};

class Ctkwin
{
public:
	Ctkwin(Ctkapp *, int, int);
	Window getw(void) {return w;};
	virtual void event(XEvent *)=0;
protected:
	Ctkapp * app;
	Display *dpy;
	
	Window w;
	void decorate();
};

#endif // CTK_H
