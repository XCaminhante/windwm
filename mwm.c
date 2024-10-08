//@+leo-ver=5-thin
//@+node:caminhante.20240209172034.11: * @file mwm.c
//@@tabwidth -2
//@@language c
//@+others
//@-others
/*
 * Copyright 2010 Johan Veenhuizen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "wind.h"

#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)

#define MWM_FUNC_ALL (1L << 0)
#define MWM_FUNC_RESIZE (1L << 1)
#define MWM_FUNC_MOVE (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE (1L << 5)

#define MWM_DECOR_ALL (1L << 0)
#define MWM_DECOR_BORDER (1L << 1)
#define MWM_DECOR_RESIZEH (1L << 2)
#define MWM_DECOR_TITLE (1L << 3)
#define MWM_DECOR_MENU (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

typedef struct {
	long flags;
	long functions;
	long decorations;
	long inputmode;
	long status;
} mwmhints;

static Atom MOTIF_WM_HINTS;

static void reloadmwmhints(struct client *);
static mwmhints *getmwmhints(struct client *);

void mwm_startwm(void)
{
	MOTIF_WM_HINTS = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
}

void mwm_manage(struct client *c)
{
	reloadmwmhints(c);
}

void mwm_propertynotify(struct client *c, XPropertyEvent *e)
{
	if (e->atom == MOTIF_WM_HINTS)
		reloadmwmhints(c);
}

/*
 * We ignore all hints except the title decor hint. If the window
 * doesn't want a title, we remove it's frame completely.
 */
static void reloadmwmhints(struct client *c)
{
	Bool undecorated = False;

	mwmhints *h = getmwmhints(c);
	if (h != NULL) {
		if (h->flags & MWM_HINTS_DECORATIONS) {
			/*
			 * If MWM_DECOR_ALL is set, it means use all
			 * decorations EXCEPT the ones specified...
			 */
			if ((h->decorations & MWM_DECOR_ALL) != 0)
				h->decorations = ~h->decorations;

			if ((h->decorations & MWM_DECOR_TITLE) == 0)
				undecorated = True;
		}
		XFree(h);
	}

	csetundecorated(c, undecorated);
}

static mwmhints *getmwmhints(struct client *c)
{
	unsigned long n = 0;
	mwmhints *h = getprop(cgetwin(c), MOTIF_WM_HINTS,
			MOTIF_WM_HINTS, 32, &n);
	if (h != NULL && n != 5) {
		XFree(h);
		h = NULL;
	}
	return h;
}
//@-leo
