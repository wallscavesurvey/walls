/* ctk.cpp
 *  - implementation of ctk
 *    homebrew toolkit 
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

/* C++ Xlib skeleton */
/* Main class takes a pointer to a fn for the main loop
 * and you can add timers */
#include <X11/Xlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include "ctk.h"

Ctkapp::Ctkapp()
{
	dpy = XOpenDisplay(NULL);
	assert(dpy != NULL);
	timeout = NULL;
}

void Ctkapp::reg(Ctkwin * cw)
{
	//Add w to list of windows to monitor
	windows.push_back(cw);
}

void Ctkapp::regtimer(int s, int us, int reps, void fn (void *), void * obj)
{
	// append a timer struct onto the end of the list
	timers.push_back(Ctktimer(s,us,reps,fn, obj));
	if (timeout == NULL)
		timeout = new struct timeval;
}

void Ctkapp::setTimeout(void)
{
	if (timeout == NULL)
		return;
	list<Ctktimer>::const_iterator min = timers.begin();
	for (list<Ctktimer>::const_iterator i = ++(timers.begin()); i != timers.end(); i++)
		if (*i < *min)
			min = i;
	timeout->tv_sec = min->rem.tv_sec;
	timeout->tv_usec = min->rem.tv_usec;
}

void Ctkapp::updateTimers(void)
{
	for (list<Ctktimer>::iterator i = timers.begin(); i != timers.end(); ++i)
	{
		i->rem.tv_sec -= timeout->tv_sec;
		i->rem.tv_usec -= timeout->tv_usec;
		if (i->rem.tv_usec < 0)
		{
			i->rem.tv_usec += 1000000;
			i->rem.tv_sec -=1;
		}
		if (i->rem.tv_sec < 0)
			i->rem = i->full;
		if ((i->rem.tv_sec == 0) && (i->rem.tv_usec == 0))
			i->rem = i->full;
	}
}

int Ctkapp::go(void)
{
	int result;
	while(1)
	{
		setTimeout();
		FD_ZERO(&reads);
		FD_SET(ConnectionNumber(dpy), &reads);
		result = select(ConnectionNumber(dpy)+1, &reads, NULL, NULL, timeout);
		if (result > 0)
		{
			while (XPending(dpy) != 0)
			{
				XNextEvent(dpy, &e);
				(*(windows.begin()) )->event(&e);
			}
			//send off event to proper window.
		} else if (result == 0)
		{
			timers.begin()->fn(timers.begin()->obj); // Timer Expired
			timers.begin()->rem = timers.begin()->full;
			if (timers.begin()->reps >= 0)
			{
				if (--timers.begin()->reps < 0)
					timers.erase(timers.begin());					;
			}
			
		}
		else {
			return 1;
		}
		updateTimers();
	}
}



Ctkwin::Ctkwin(Ctkapp * theApp, int width, int height)
{
	app = theApp;
	dpy = app->getDisplay();

	XSetWindowAttributes attribs;
	attribs.background_pixel = WhitePixel(dpy, DefaultScreen(dpy));
	w = XCreateWindow(dpy, /* display */
				DefaultRootWindow(dpy),
				0,0,width,height, /* x,y,w,h */
				0, /* border width */
				CopyFromParent, /* depth */
				InputOutput, /* type */
				CopyFromParent, /* visualtype */
				CWBackPixel, /* valuemask */
				&attribs); /* values */
	XSelectInput(dpy, w, StructureNotifyMask | ExposureMask);
	XStoreName(dpy, w, "Goodbye Cruel World");
	decorate();
	XMapWindow(dpy, w);
	app->reg(this);
}

void Ctkwin::decorate()
{
	Atom bomb = XInternAtom(dpy, "_MOTIF_WM_HINTS", 1);
        struct mwmhints_t {
                unsigned int flags;
                unsigned int functions;
                unsigned int decorations;
                int  input_mode;
                unsigned int status;
        } mwmhints;
        mwmhints.decorations = mwmhints.functions =  (1l << 0);
        mwmhints.flags = (1l << 0) | (1l << 1);
        XChangeProperty(dpy,w,bomb,bomb,32,PropModeReplace,(unsigned char *)&mwmhints, sizeof(mwmhints)/sizeof(long));
        return;
}

Ctktimer::Ctktimer(int s, int us, int repeats, void function(void *), void * object)
{
	full.tv_sec = s;
	full.tv_usec = us;
	rem.tv_sec = full.tv_sec;
	rem.tv_usec = full.tv_usec;
	reps = repeats;
	fn = function;
	obj = object;
}

bool Ctktimer::operator< (const Ctktimer other) const
{
	if (this->rem.tv_sec < other.rem.tv_sec)
		return true;
	else if (this->rem.tv_sec > other.rem.tv_sec)
		return false;
	else if (this->rem.tv_usec < other.rem.tv_usec)
		return true;
	else
		return false;
}

void timedfn();

/*
int main(int argc, char ** argv)
{
	Ctkapp myapp;
	Ctkwin mywin(&myapp,100,100);
//	myapp.regtimer(1,0,-1, timedfn, NULL);
	myapp.go();
	return 0;
}
*/
