//@+leo-ver=5-thin
//@+node:caminhante.20240208143459.31: * @file client.c
//@@tabwidth -2
//@@language c
//@+others
//@+node:caminhante.20240208151545.1: ** /sobre
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
//@+node:caminhante.20240208151541.1: ** /includes
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "list.h"
#include "wind.h"
//@+node:caminhante.20240208150805.1: ** /protótipos
static void cmap(struct client *);
static void cunmap(struct client *);
static void cpop(struct client *);
static void cpush(struct client *);
static void cfocusapp(struct client *, Time);
static void reloadwmtransientfor(struct client *);
static void reloadwmhints(struct client *);
static void reloadwmnormalhints(struct client *);
static void reloadwmname(struct client *);
static void reloadwmprotocols(struct client *);
static void cupdatedesk(struct client *);
static void cbuttonpress(struct client *, XButtonEvent *);
static void keypress(struct client *, XKeyEvent *);
static void keypress_delete(struct client *, unsigned, Time);
static void keypress_pushapp(struct client *, unsigned, Time);
static void keypress_fullscreen(struct client *, unsigned, Time);
static void keypress_sticky(struct client *, unsigned, Time);
static void focusin(struct client *, XFocusChangeEvent *);
static void focusout(struct client *, XFocusChangeEvent *);
static void configurerequest(struct client *, XConfigureRequestEvent *);
static void propertynotify(struct client *, XPropertyEvent *);
static void maprequest(struct client *, XMapRequestEvent *);
static void cunmapnotify(struct client *, XUnmapEvent *);
static void destroynotify(struct client *, XDestroyWindowEvent *);
static void clientmessage(struct client *, XClientMessageEvent *);
static void colormapnotify(struct client *, XColormapEvent *);
static void cevent(void *, XEvent *);
static void cinstallcolormaps(struct client *);
static void crelease(struct client *, int);
static void getclientstack(struct client ***, int *);
static void csendwmproto(struct client *, Atom, Time);
static void creframe(struct client *);
static Bool cisframed(struct client *);
static struct client *getfronttask(void);
static Bool expectsfocus(struct client *);
static void cunmanage(struct client *);
static void cwithdraw(struct client *);
static void smartpos(struct client *);
static void randpos(struct geometry *);
static Bool samedesk(struct client *, struct client *);
static unsigned long overlaparea(struct geometry, struct geometry);
static void move(struct client *, int, int);
//@+node:caminhante.20240208150759.1: ** /constantes, variáveis e structs
static LIST_DEFINE(winstack);

// Current desk
static Desk curdesk = 0;

// Number of desks
static Desk ndesk = 1;

// True if restacking needed
static Bool needrestack = False;

// Dummy window for window stacking
static Window stacktop = None;

static struct {
	KeySym keysym;
	unsigned modifiers;
	void (*function)(struct client *, unsigned, Time);
} keymap[] = {
	{ XK_BackSpace, Mod1Mask, keypress_delete },
	{ XK_Escape, Mod1Mask, keypress_pushapp },
	{ XK_Return, Mod1Mask, keypress_fullscreen },
	{ XK_space, Mod1Mask, keypress_sticky },
};
//@+node:caminhante.20240208150816.1: *3* struct client
struct client {
	struct listener listener;
	List winstack;
	Window window;
	Colormap colormap;

	struct geometry geometry;

	XWMHints *wmhints;
	XSizeHints *wmnormalhints;
	Atom *wmprotocols;
	int wmprotocolscount;
	Window wmtransientfor;

	// WM_NAME property in current locale encoding
	char *wmname;

	// _NET_WM_NAME property in UTF-8 encoding
	char *netwmname;

	// Application id of this client
	XID app;

	struct frame *frame;

	Desk desk;

	/*
	 * If this counter is zero when an UnmapNotify event
	 * is received, the client is considered withdrawn.
	 */
	int ignoreunmapcount;

	Bool ismapped;
	Bool hasfocus;
	Bool isfull;
	Bool isdock;
	Bool skiptaskbar;
	Bool isundecorated;
	Bool followdesk;
	Bool initialized;
};
//@+node:caminhante.20240208150738.1: ** setndesk
void setndesk(Desk val)
{
	if (val == 0 || val >= 0xffffffffUL)
		return;

	Desk oldval = ndesk;
	ndesk = val;

	if (val >= oldval)
		ewmh_notifyndesk(val);

	if (curdesk >= val)
		gotodesk(val - 1);

	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = n - 1; i >= 0; i--)
		if (v[i]->desk != DESK_ALL && v[i]->desk >= val)
			csetdesk(v[i], val - 1);
	free(v);

	if (val < oldval)
		ewmh_notifyndesk(val);
}
//@+node:caminhante.20240208150731.1: ** gotodesk
void gotodesk(Desk d)
{
	if (d == curdesk || d >= ndesk || d == DESK_ALL)
		return;

	curdesk = d;

	/*
	 * Minimize the number of window exposures by first mapping
	 * windows from the new desk top-down, and then unmapping
	 * the windows from the old desk bottom-up.
	 */
	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = n - 1; i >= 0; i--)
		if (cisvisible(v[i]))
			cmap(v[i]);
	for (int i = 0; i < n; i++)
		if (v[i]->followdesk && v[i]->desk != DESK_ALL)
			csetdesk(v[i], curdesk);
		else if (!cisvisible(v[i]))
			cunmap(v[i]);
	free(v);

	ewmh_notifycurdesk(curdesk);
}
//@+node:caminhante.20240208150717.1: ** csetappdesk
void csetappdesk(struct client *c, Desk d)
{
	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = 0; i < n; i++)
		if (v[i]->app == c->app)
			csetdesk(v[i], d);
	free(v);
}
//@+node:caminhante.20240208150710.1: ** csetdesk
void csetdesk(struct client *c, Desk d)
{
	if (d >= ndesk && d != DESK_ALL)
		d = ndesk - 1;

	c->desk = d;
	ewmh_notifyclientdesktop(c->window, d);

	if (cisvisible(c))
		cmap(c);
	else
		cunmap(c);

	// May have become sticky
	if (c->frame != NULL)
		fupdate(c->frame);
}
//@+node:caminhante.20240208150705.1: ** cgetdesk
Desk cgetdesk(struct client *c)
{
	return c->desk;
}
//@+node:caminhante.20240208150701.1: ** csetdock
void csetdock(struct client *c, Bool isdock)
{
	c->isdock = isdock;
	creframe(c);
}
//@+node:caminhante.20240208150654.1: ** csetfull
void csetfull(struct client *c, Bool enabled)
{
	if (enabled && !c->isfull) {
		Bool f = c->hasfocus;
		cunmap(c);
		if (c->frame != NULL) {
			fdestroy(c->frame);
			c->frame = NULL;
		}
		c->isfull = True;
		XMoveResizeWindow(dpy, c->window,
				-c->geometry.borderwidth,
				-c->geometry.borderwidth,
				DisplayWidth(dpy, scr),
				DisplayHeight(dpy, scr));
		if (cisvisible(c))
			cmap(c);
		if (f)
			cfocus(c, CurrentTime);
		ewmh_notifyfull(c->window, True);
	} else if (!enabled && c->isfull) {
		assert(c->frame == NULL);
		Bool f = c->hasfocus;
		cunmap(c);
		c->isfull = False;
		XMoveResizeWindow(dpy, c->window,
				c->geometry.x, c->geometry.y,
				c->geometry.width, c->geometry.height);
		if (cisframed(c))
			c->frame = fcreate(c);
		if (cisvisible(c))
			cmap(c);
		if (f)
			cfocus(c, CurrentTime);
		ewmh_notifyfull(c->window, False);
	}
}
//@+node:caminhante.20240208150647.1: ** csetundecorated
void csetundecorated(struct client *c, Bool enabled)
{
	c->isundecorated = enabled;
	creframe(c);
}
//@+node:caminhante.20240208150642.1: ** csetappfollowdesk
void csetappfollowdesk(struct client *c, Bool enabled)
{
	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = 0; i < n; i++)
		if (v[i]->app == c->app)
			v[i]->followdesk = enabled;
	free(v);
}
//@+node:caminhante.20240208150637.1: ** getwindowstack
/*
 * Return client window stack, from bottom (first) to top (last).
 * Caller deallocates using free(3).
 */
void getwindowstack(Window **vp, size_t *np)
{
	List *lp;

	size_t n = 0;
	LIST_FOREACH(lp, &winstack)
		n++;

	Window *v = xmalloc(n * sizeof v[0]);
	size_t i = 0;
	LIST_FOREACH(lp, &winstack) {
		struct client *c = LIST_ITEM(lp, struct client, winstack);
		v[i++] = c->window;
	}

	*vp = v;
	*np = n;
}
//@+node:caminhante.20240208150631.1: ** cpushapp
void cpushapp(struct client *c)
{
	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = n - 1; i >= 0; i--)
		if (v[i]->app == c->app)
			cpush(v[i]);
	free(v);
}
//@+node:caminhante.20240208150625.1: ** cpopapp
void cpopapp(struct client *c)
{
	struct client **v;
	int n;
	getclientstack(&v, &n);

	for (int i = 0; i < n; i++)
		if (v[i]->app == c->app)
			cpop(v[i]);

	if (c->wmtransientfor != None) {
		for (int i = 0; i < n; i++)
			if (v[i]->window == c->wmtransientfor) {
				cpop(v[i]);
				break;
			}
		for (int i = 0; i < n; i++)
			if (v[i]->wmtransientfor == c->wmtransientfor)
				cpop(v[i]);
		cpop(c);
	} else {
		cpop(c);
		for (int i = 0; i < n; i++)
			if (v[i]->wmtransientfor == c->window)
				cpop(v[i]);
	}

	free(v);
}
//@+node:caminhante.20240208150619.1: ** getclientstack
/*
 * Return client stack, from bottom (first) to top (last).
 * Caller deallocates using free(3).
 */
static void getclientstack(struct client ***vp, int *np)
{
	int n = 0;
	List *lp;
	LIST_FOREACH(lp, &winstack)
		n++;
	struct client **v = xmalloc(n * sizeof *v);
	struct client **p = v;
	LIST_FOREACH(lp, &winstack)
		*p++ = LIST_ITEM(lp, struct client, winstack);
	*vp = v;
	*np = n;
}
//@+node:caminhante.20240208150613.1: ** cpop
static void cpop(struct client *c)
{
	if (LIST_TAIL(&winstack) != &c->winstack) {
		LIST_REMOVE(&c->winstack);
		LIST_INSERT_TAIL(&winstack, &c->winstack);
		needrestack = True;
	}
}
//@+node:caminhante.20240208150610.1: ** cpush
static void cpush(struct client *c)
{
	if (LIST_HEAD(&winstack) != &c->winstack) {
		LIST_REMOVE(&c->winstack);
		LIST_INSERT_HEAD(&winstack, &c->winstack);
		needrestack = True;
	}
}
//@+node:caminhante.20240208150604.1: ** restack
void restack(void)
{
	if (!needrestack)
		return;
	int n = 1;
	List *lp;
	LIST_FOREACH(lp, &winstack)
		n++;
	Window *v = xmalloc(n * sizeof *v);
	int i = 0;
	assert(stacktop != None);
	v[i++] = stacktop;
	LIST_FOREACH_REV(lp, &winstack) {
		struct client *c = LIST_ITEM(lp, struct client, winstack);
		v[i++] = c->frame == NULL ? c->window : fgetwin(c->frame);
	}
	assert(i == n);
	XRestackWindows(dpy, v, n);
	free(v);
	needrestack = False;
	ewmh_notifyrestack();
}
//@+node:caminhante.20240208150559.1: ** reloadwmtransientfor
static void reloadwmtransientfor(struct client *c)
{
	c->wmtransientfor = None;
	XGetTransientForHint(dpy, c->window, &c->wmtransientfor);

	if (c->wmtransientfor != None) {
		c->app = c->wmtransientfor;
		XWMHints *h = XGetWMHints(dpy, c->wmtransientfor);
		if (h != NULL) {
			if (h->flags & WindowGroupHint)
				c->app = h->window_group;
			XFree(h);
		}
		cupdatedesk(c);
	}
}
//@+node:caminhante.20240208150554.1: ** reloadwmhints
static void reloadwmhints(struct client *c)
{
	if (c->wmhints != NULL)
		XFree(c->wmhints);
	c->wmhints = XGetWMHints(dpy, c->window);

	if (c->wmtransientfor == None) {
		c->app = c->window;
		if (c->wmhints != NULL) {
			if (c->wmhints->flags & WindowGroupHint)
				c->app = c->wmhints->window_group;
		}
		cupdatedesk(c);
	}

	if (cisurgent(c) && c->initialized) {
		XBell(dpy, 0);
		cpopapp(c);
		gotodesk(c->desk);
		cfocus(c, CurrentTime);
	}
}
//@+node:caminhante.20240208150549.1: ** reloadwmnormalhints
static void reloadwmnormalhints(struct client *c)
{
	if (c->wmnormalhints == NULL)
		c->wmnormalhints = XAllocSizeHints();
	if (c->wmnormalhints != NULL) {
		c->wmnormalhints->flags = 0;
		long dummy;
		XGetWMNormalHints(dpy, c->window, c->wmnormalhints, &dummy);
	}
}
//@+node:caminhante.20240208150541.1: ** reloadwmname
static void reloadwmname(struct client *c)
{
	free(c->wmname);
	c->wmname = NULL;

	XTextProperty p;
	if (XGetWMName(dpy, c->window, &p) != 0) {
		c->wmname = decodetextproperty(&p);
		if (p.value != NULL)
			XFree(p.value);
	}

	if (c->frame != NULL)
		fupdate(c->frame);
}
//@+node:caminhante.20240208150535.1: ** reloadwmprotocols
static void reloadwmprotocols(struct client *c)
{
	if (c->wmprotocols != NULL) {
		XFree(c->wmprotocols);
		c->wmprotocols = NULL;
	}
	c->wmprotocolscount = 0;
	XGetWMProtocols(dpy, c->window,
			&c->wmprotocols, &c->wmprotocolscount);

	if (c->frame != NULL)
		fupdate(c->frame);
}
//@+node:caminhante.20240208150531.1: ** cupdatedesk
static void cupdatedesk(struct client *c)
{
	Desk d = c->desk;
	struct client **v;
	int n;
	getclientstack(&v, &n);
	if (c->wmtransientfor != None) {
		for (int i = n - 1; i >= 0; i--)
			if (v[i]->window == c->wmtransientfor) {
				d = v[i]->desk;
				break;
			}
	} else if (c->app != c->window) {
		for (int i = n - 1; i >= 0; i--)
			if (v[i]->app == c->app && v[i] != c) {
				d = v[i]->desk;
				break;
			}
	}
	free(v);
	if (d != c->desk)
		csetdesk(c, d);
}
//@+node:caminhante.20240208150525.1: ** cbuttonpress
static void cbuttonpress(struct client *c, XButtonEvent *e)
{
	cpopapp(c);
	cfocus(c, e->time);
	XAllowEvents(dpy, ReplayPointer, e->time);
}
//@+node:caminhante.20240208150451.1: ** keypress
static void keypress(struct client *c, XKeyEvent *e)
{
	for (size_t i = 0; i < NELEM(keymap); i++)
		if (XKeysymToKeycode(dpy, keymap[i].keysym) == e->keycode)
			keymap[i].function(c, e->state, e->time);
}
//@+node:caminhante.20240208150447.1: ** keypress_delete
static void keypress_delete(struct client *c, unsigned state, Time time)
{
      (void) state;
	if (!c->isdock) cdelete(c, time);
}
//@+node:caminhante.20240208150443.1: ** keypress_pushapp
static void keypress_pushapp(struct client *c, unsigned state, Time time)
{
      (void) state;
	cpushapp(c);
	refocus(time);
}
//@+node:caminhante.20240208150438.1: ** keypress_fullscreen
static void keypress_fullscreen(struct client *c, unsigned state, Time time)
{
      (void) state; (void) time;
	if (!c->isdock) csetfull(c, !c->isfull);
}
//@+node:caminhante.20240208150432.1: ** keypress_sticky
static void keypress_sticky(struct client *c, unsigned state, Time time)
{
      (void) state; (void) time;
	if (c->isdock)
		return;

	if (cgetdesk(c) == DESK_ALL)
		csetappdesk(c, curdesk);
	else {
		csetappdesk(c, DESK_ALL);

		// Make sure we are still on top when switching desks.
		cpopapp(c);
	}
}
//@+node:caminhante.20240208150413.1: ** focusin
static void focusin(struct client *c, XFocusChangeEvent *e)
{
	if (e->mode == NotifyUngrab || e->detail == NotifyPointerRoot ||
			e->detail == NotifyPointer)
		return;

	// This shouldn't happen.
	if (c->hasfocus || !c->ismapped) {
		return; }

	c->hasfocus = True;

	ungrabbutton(AnyButton, AnyModifier, c->window);

	cinstallcolormaps(c);

	if (c->frame != NULL)
		fupdate(c->frame);

	ewmh_notifyfocus(None, c->window);
}
//@+node:caminhante.20240208150407.1: ** focusout
static void focusout(struct client *c, XFocusChangeEvent *e)
{
	if (e->mode == NotifyGrab)
		return;

	if (e->detail == NotifyPointerRoot ||
			e->detail == NotifyPointer ||
			e->detail == NotifyInferior)
		return;

	// This shouldn't happen.
	if (!c->hasfocus) {
		return; }

	c->hasfocus = False;

	grabbutton(AnyButton, AnyModifier, c->window, True, 0,
			GrabModeSync, GrabModeAsync, None, None);

	if (c->frame != NULL)
		fupdate(c->frame);

	ewmh_notifyfocus(c->window, None);
}
//@+node:caminhante.20240208150402.1: ** configurerequest
static void configurerequest(struct client *c, XConfigureRequestEvent *e)
{
	if (c->frame != NULL) {
		/*
		 * If this happens, we are processing an event that
		 * was sent before we created the frame. We need to
		 * redirect the event manually. Note that this should
		 * only happen immediately after creating a frame.
		 *
		 * XMMS is one program that triggers this particularly
		 * often, and so is the "Save As" dialog of Firefox.
		 */
		redirect((XEvent *)e, fgetwin(c->frame));
		return;
	}

	if (c->isfull) {
		// Deny fullscreen windows to reconfigure themselves.
		csendconf(c);
		return;
	}

	unsigned long mask = e->value_mask &
			(CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

	if (mask & CWX)
		c->geometry.x = e->x;
	if (mask & CWY)
		c->geometry.y = e->y;
	if (mask & CWWidth)
		c->geometry.width = e->width;
	if (mask & CWHeight)
		c->geometry.height = e->height;
	if (mask & CWBorderWidth)
		c->geometry.borderwidth = e->border_width;

	XConfigureWindow(dpy, c->window, mask,
			&(XWindowChanges){
				.x = c->geometry.x,
				.y = c->geometry.y,
				.width = c->geometry.width,
				.height = c->geometry.height,
				.border_width = c->geometry.borderwidth });
}
//@+node:caminhante.20240208150355.1: ** propertynotify
static void propertynotify(struct client *c, XPropertyEvent *e)
{
	switch (e->atom) {
	case XA_WM_NAME:
		reloadwmname(c);
		break;
	case XA_WM_HINTS:
		reloadwmhints(c);
		break;
	case XA_WM_NORMAL_HINTS:
		reloadwmnormalhints(c);
		break;
	case XA_WM_TRANSIENT_FOR:
		reloadwmtransientfor(c);
		break;
	default:
		if (e->atom == WM_PROTOCOLS)
			reloadwmprotocols(c);
		break;
	}

	ewmh_propertynotify(c, e);
	mwm_propertynotify(c, e);
}
//@+node:caminhante.20240208150348.1: ** maprequest
/*
 * We don't listen to this event ourselves, but get it redirected to
 * us from the root listener and from the frame listener.
 */
static void maprequest(struct client *c, XMapRequestEvent *e)
{
      (void) e;
	ewmh_maprequest(c);
	cpopapp(c);
	if (cisvisible(c)) {
		cmap(c);
		cfocus(c, CurrentTime);
	}
}
//@+node:caminhante.20240208150342.1: ** cunmapnotify
static void cunmapnotify(struct client *c, XUnmapEvent *e)
{
      (void) e;
	if (c->ignoreunmapcount > 0)
		c->ignoreunmapcount--;
	else
		cwithdraw(c);
}
//@+node:caminhante.20240208150338.1: ** destroynotify
static void destroynotify(struct client *c, XDestroyWindowEvent *e)
{
      (void) e;
	cwithdraw(c);
}
//@+node:caminhante.20240208150332.1: ** clientmessage
static void clientmessage(struct client *c, XClientMessageEvent *e)
{
	if (e->message_type == WM_CHANGE_STATE && e->format == 32 &&
			e->data.l[0] == IconicState) {
		/*
		 * Wind doesn't allow hidden windows, so just push it.
		 */
		cpushapp(c);
		if (chasfocus(c))
			refocus(CurrentTime);
	}

	ewmh_clientmessage(c, e);
}
//@+node:caminhante.20240208150327.1: ** colormapnotify
static void colormapnotify(struct client *c, XColormapEvent *e)
{
	if (e->new) {
		c->colormap = e->colormap;
		if (c->hasfocus)
			cinstallcolormaps(c);
	}
}
//@+node:caminhante.20240208150322.1: ** cevent
static void cevent(void *self, XEvent *e)
{
	switch (e->type) {
	case ButtonPress:
		cbuttonpress(self, &e->xbutton);
		break;
	case KeyPress:
		keypress(self, &e->xkey);
		break;
	case FocusIn:
		focusin(self, &e->xfocus);
		break;
	case FocusOut:
		focusout(self, &e->xfocus);
		break;
	case ConfigureRequest:
		configurerequest(self, &e->xconfigurerequest);
		break;
	case PropertyNotify:
		propertynotify(self, &e->xproperty);
		break;
	case MapRequest:
		// We get this redirected to us from the root listener
		// and from the frame listener.
		maprequest(self, &e->xmaprequest);
		break;
	case UnmapNotify:
		unmapnotify(self, &e->xunmap);
		break;
	case DestroyNotify:
		destroynotify(self, &e->xdestroywindow);
		break;
	case ClientMessage:
		clientmessage(self, &e->xclient);
		break;
	case ColormapNotify:
		colormapnotify(self, &e->xcolormap);
		break;
	}
}
//@+node:caminhante.20240208150315.1: ** cinstallcolormaps
static void cinstallcolormaps(struct client *c)
{
	XInstallColormap(dpy, c->colormap == None ?
			DefaultColormap(dpy, scr) : c->colormap);
}
//@+node:caminhante.20240208150311.1: ** cisvisible
/*
 * Returns true if the window is, or should be, visible.
 */
Bool cisvisible(struct client *c)
{
	return c->desk == curdesk || c->desk == DESK_ALL;
}
//@+node:caminhante.20240208150305.1: ** expectsfocus
/*
 * Returns true if the window expects keyboard input focus.
 * A window that does not specify an input hint is considered
 * to expect focus.
 */
static Bool expectsfocus(struct client *c)
{
	return c->wmhints == NULL ||
			!(c->wmhints->flags & InputHint) ||
			c->wmhints->input;
}
//@+node:caminhante.20240208150258.1: ** cistask
/*
 * Returns true if the window is a task, i.e. if it appears in taskbars.
 */
Bool cistask(struct client *c)
{
	return !c->skiptaskbar && c->wmtransientfor == None;
}
//@+node:caminhante.20240208150250.1: ** manage
struct client *manage(Window window)
{
	XWindowAttributes attr;
	if (!XGetWindowAttributes(dpy, window, &attr))
		return NULL;
	if (attr.override_redirect)
		return NULL;

	long wmstate = getwmstate(window);
	if (wmstate == WithdrawnState) {
		XWMHints *h = XGetWMHints(dpy, window);
		if (h == NULL)
			wmstate = NormalState;
		else {
			if (h->flags & StateHint)
				wmstate = h->initial_state;
			else
				wmstate = NormalState;
			XFree(h);
		}
	}
	if (wmstate == WithdrawnState)
		return NULL;

	struct client *c = xmalloc(sizeof *c);

	LIST_INIT(&c->winstack);
	LIST_INSERT_TAIL(&winstack, &c->winstack);
	needrestack = True;

	c->desk = curdesk;
	c->frame = NULL;
	c->wmname = NULL;
	c->netwmname = NULL;
	c->wmhints = NULL;
	c->wmnormalhints = NULL;
	c->wmprotocols = NULL;
	c->wmprotocolscount = 0;
	c->wmtransientfor = None;

	c->window = window;
	c->app = window;
	c->colormap = attr.colormap;

	c->ignoreunmapcount = 0;

	c->hasfocus = False;
	c->isfull = False;
	c->isdock = False;
	c->skiptaskbar = False;
	c->isundecorated = False;
	c->followdesk = False;
	c->initialized = False;

	csetgeom(c, (struct geometry){
			attr.x,
			attr.y,
			attr.width,
			attr.height,
			attr.border_width });

	XAddToSaveSet(dpy, c->window);

	/*
	 * Everything initialized to default values. Register
	 * for events and THEN (re)load all attributes and
	 * properties. This avoids losing update events.
	 */
	c->listener.function = cevent;
	c->listener.pointer = c;
	setlistener(c->window, &c->listener);
	XSelectInput(dpy, c->window,
			StructureNotifyMask |
			PropertyChangeMask |
			ColormapChangeMask |
			FocusChangeMask);

	XSync(dpy, False);

	/*
	 * Done registering for events. What we read now is
	 * safe to use, since any updates will be notified
	 * to our event listener.
	 */

	c->ismapped = ismapped(c->window);

	if (!XGetWindowAttributes(dpy, c->window, &attr)) {
		// The window disappeared.
		crelease(c, True);
		return NULL;
	}

	csetgeom(c, (struct geometry){
			attr.x,
			attr.y,
			attr.width,
			attr.height,
			attr.border_width });

	c->colormap = attr.colormap;

	reloadwmname(c);
	reloadwmhints(c);
	reloadwmnormalhints(c);
	reloadwmprotocols(c);
	reloadwmtransientfor(c);

	/*
	 * Let the hints create the frame, if there should be one.
	 */
	ewmh_manage(c);
	mwm_manage(c);

	grabbutton(AnyButton, AnyModifier, c->window, True, 0,
			GrabModeSync, GrabModeAsync, None, None);

	for (size_t i = 0; i < NELEM(keymap); i++)
		grabkey(XKeysymToKeycode(dpy, keymap[i].keysym),
				keymap[i].modifiers, c->window, True,
				GrabModeAsync, GrabModeAsync);

	if (c->geometry.width == DisplayWidth(dpy, scr) &&
			c->geometry.height == DisplayHeight(dpy, scr))
		csetfull(c, True);

	if (!cisframed(c))
		ewmh_notifyframeextents(c->window, (struct extents){
				.top = 0,
				.bottom = 0,
				.left = 0,
				.right = 0 });

	XSizeHints *h = c->wmnormalhints;
	if (runlevel != RL_STARTUP && (h == NULL ||
			(h->flags & (USPosition | PPosition)) == 0))
		smartpos(c);

	/*
	 * Make sure WM_STATE is always initiated. We can't trust
	 * the first call to cmap/cunmap.
	 */
	setwmstate(c->window, cisvisible(c) ? NormalState : IconicState);

	c->initialized = True;

	if (wmstate == IconicState && runlevel == RL_NORMAL) {
		// Closest thing to iconic state
		cpush(c);
		if (cisvisible(c))
			cmap(c);
	} else {
		cpopapp(c);
		if (cisurgent(c) && runlevel == RL_NORMAL) {
			XBell(dpy, 0);
			gotodesk(c->desk);
		}
		if (cisvisible(c)) {
			cmap(c);
			if (runlevel == RL_NORMAL)
				cfocus(c, CurrentTime);
		}
	}

	return c;
}
//@+node:caminhante.20240208150238.1: ** manageall
void manageall(void)
{
	assert(stacktop == None);
	stacktop = XCreateWindow(dpy, root, 0, 0, 100, 100, 0, CopyFromParent,
			InputOnly, CopyFromParent, 0, NULL);
	Window r, p, *stack;
	unsigned n;
	if (XQueryTree(dpy, root, &r, &p, &stack, &n) != 0) {
		for (size_t i = 0; i < n; i++) {
			if (ismapped(stack[i]))
				manage(stack[i]);
		}
		if (stack != NULL)
			XFree(stack);
	}
	restack();
}
//@+node:caminhante.20240208150232.1: ** cmap
static void cmap(struct client *c)
{
	assert(c->desk == curdesk || c->desk == DESK_ALL);

	// Prevent premature mapping
	if (!c->initialized)
		return;

	if (!c->ismapped) {
		/*
		 * Make sure stacking order is correct before
		 * mapping the window.
		 */
		restack();

		setwmstate(c->window, NormalState);
		if (c->frame != NULL) {
			Window f = fgetwin(c->frame);
			XMapSubwindows(dpy, f);
			XMapWindow(dpy, f);
		} else
			XMapWindow(dpy, c->window);
		c->ismapped = True;
	}
}
//@+node:caminhante.20240208150223.1: ** cunmap
static void cunmap(struct client *c)
{
	if (c->ismapped) {
		setwmstate(c->window, IconicState);
		if (c->frame != NULL) {
			Window f = fgetwin(c->frame);
			XUnmapWindow(dpy, f);
			XUnmapSubwindows(dpy, f);
		} else
			XUnmapWindow(dpy, c->window);
		c->ignoreunmapcount++;
		c->ismapped = False;
	}
}
//@+node:caminhante.20240208150218.1: ** crelease
static void crelease(struct client *c, Bool clientrequested)
{
	// Unset this or fdestroy() will refocus the window.
	c->hasfocus = False;

	if (c->frame != NULL) {
		fdestroy(c->frame);
		c->frame = NULL;
	}

	LIST_REMOVE(&c->winstack);
	needrestack = True;

	ungrabkey(AnyKey, AnyModifier, c->window);

	XSelectInput(dpy, c->window, 0);
	setlistener(c->window, NULL);
	if (!clientrequested)
		XMapWindow(dpy, c->window);
	XRemoveFromSaveSet(dpy, c->window);

	if (c->wmnormalhints != NULL)
		XFree(c->wmnormalhints);
	if (c->wmhints != NULL)
		XFree(c->wmhints);
	if (c->wmprotocols != NULL)
		XFree(c->wmprotocols);
	free(c->wmname);
	free(c->netwmname);
	free(c);

	if (getfocus() == NULL)
		refocus(CurrentTime);
}
//@+node:caminhante.20240208150212.1: ** cwithdraw
static void cwithdraw(struct client *c)
{
	ewmh_withdraw(c);
	setwmstate(c->window, WithdrawnState);
	crelease(c, True);
}
//@+node:caminhante.20240208150206.1: ** cunmanage
static void cunmanage(struct client *c)
{
	ewmh_unmanage(c);
	setwmstate(c->window, NormalState);
	crelease(c, False);
}
//@+node:caminhante.20240208150200.1: ** cdelete
void cdelete(struct client *c, Time time)
{
	if (chaswmproto(c, WM_DELETE_WINDOW))
		csendwmproto(c, WM_DELETE_WINDOW, time);
}
//@+node:caminhante.20240208150154.1: ** unmanageall
void unmanageall(void)
{
	struct client **v;
	int n;
	getclientstack(&v, &n);
	for (int i = n - 1; i >= 0; i--)
		cunmanage(v[i]);
	free(v);

	if (stacktop != None) {
		XDestroyWindow(dpy, stacktop);
		stacktop = None;
	}
}
//@+node:caminhante.20240208150147.1: ** getfocus
struct client *getfocus(void)
{
	List *lp;
	LIST_FOREACH_REV(lp, &winstack) {
		struct client *c = LIST_ITEM(lp, struct client, winstack);
		if (c->hasfocus) {
			assert(c->desk == curdesk || c->desk == DESK_ALL);
			assert(c->ismapped);
			return c;
		}
	}
	return NULL;
}
//@+node:caminhante.20240208150138.1: ** getfronttask
static struct client *getfronttask(void)
{
	List *lp;
	LIST_FOREACH_REV(lp, &winstack) {
		struct client *c = LIST_ITEM(lp, struct client, winstack);
		if (cisvisible(c) && cistask(c))
			return c;
	}
	return NULL;
}
//@+node:caminhante.20240208150130.1: ** cfocusapp
static void cfocusapp(struct client *c, Time time)
{
	struct client *topmost = NULL;
	struct client *focus = NULL;

	// Find a window of the application that expects focus.
	List *lp;
	LIST_FOREACH_REV(lp, &winstack) {
		struct client *x = LIST_ITEM(
				lp, struct client, winstack);
		if (x->app == c->app && cisvisible(x)) {
			if (topmost == NULL)
				topmost = x;
			if (focus == NULL && expectsfocus(x))
				focus = x;
		}
	}

	if (focus == NULL)
		focus = topmost;

	assert(focus != NULL);

	cfocus(focus, time);
}
//@+node:caminhante.20240208150116.1: ** cfocus
/*
 * Change input focus to the specified client, which must be
 * mapped on the current desktop.
 *
 * This function ignores input hints and the WM_TAKE_FOCUS protocol.
 */
void cfocus(struct client *c, Time time)
{
	if (!cismapped(c))
		return;

	XSetInputFocus(dpy, c->window, RevertToPointerRoot, time);
}
//@+node:caminhante.20240208150109.1: ** client
/*
 * Focus the front window and return it.
 */
struct client *refocus(Time time)
{
	struct client *c = getfronttask();
	if (c != NULL)
		cfocusapp(c, time);
	return c;
}
//@+node:caminhante.20240208150103.1: ** namewidth
int namewidth(struct font *font, struct client *c)
{
	if (c->netwmname != NULL)
		return fttextwidth_utf8(font, c->netwmname);
	else if (c->wmname != NULL)
		return fttextwidth(font, c->wmname);
	else
		return 0;
}
//@+node:caminhante.20240208150057.1: ** drawname
void drawname(Drawable d, struct font *font, struct fontcolor *color,
		int x, int y, struct client *c)
{
	if (c->netwmname != NULL)
		ftdrawstring_utf8(d, font, color, x, y, c->netwmname);
	else if (c->wmname != NULL)
		ftdrawstring(d, font, color, x, y, c->wmname);
}
//@+node:caminhante.20240208150050.1: ** chaswmproto
Bool chaswmproto(struct client *c, Atom protocol)
{
	for (int i = 0; i < c->wmprotocolscount; i++)
		if (c->wmprotocols[i] == protocol)
			return True;
	return False;
}
//@+node:caminhante.20240208150042.1: ** csendwmproto
static void csendwmproto(struct client *c, Atom protocol, Time time)
{
	XEvent e;

	memset(&e, 0, sizeof e);
	e.xclient.type = ClientMessage;
	e.xclient.window = c->window;
	e.xclient.message_type = WM_PROTOCOLS;
	e.xclient.format = 32;
	e.xclient.data.l[0] = protocol;
	e.xclient.data.l[1] = time;

	XSendEvent(dpy, c->window, False, 0L, &e);
}
//@+node:caminhante.20240208150035.1: ** cgetgrav
int cgetgrav(struct client *c)
{
	if (c->wmnormalhints != NULL &&
			(c->wmnormalhints->flags & PWinGravity) != 0)
		return c->wmnormalhints->win_gravity;
	else
		return NorthWestGravity;
}
//@+node:caminhante.20240208150028.1: ** cgetgeom
struct geometry cgetgeom(struct client *c)
{
	if (c->isfull)
		return (struct geometry){
				.x = -c->geometry.borderwidth,
				.y = -c->geometry.borderwidth,
				.width = DisplayWidth(dpy, scr),
				.height = DisplayHeight(dpy, scr),
				.borderwidth = c->geometry.borderwidth };
	else
		return c->geometry;
}
//@+node:caminhante.20240208150020.1: ** csetgeom
void csetgeom(struct client *c, struct geometry g)
{
	c->geometry = g;
}
//@+node:caminhante.20240208150014.1: ** chintsize
void chintsize(struct client *c, int width, int height,
		int *rwidth, int *rheight)
{
	width = MAX(1, width);
	height = MAX(1, height);

	XSizeHints *h = c->wmnormalhints;
	if (h == NULL) {
		*rwidth = width;
		*rheight = height;
		return;
	}

	/*
	 * Aspect ratio hints, with the following meaning:
	 *
	 *       min_aspect.x    width     max_aspect.x
	 *   0 < ------------ <= ------ <= ------------ < inf
	 *       min_aspect.y    height    max_aspect.y
	 *
	 * Ignore the hints if the values are negative, zero, or
	 * otherwise out of range. This also avoids division by zero.
	 *
	 * FIXME: We should avoid screwing up these limits further
	 * down when adjusting for size increments and min/max size.
	 */
	if ((h->flags & PAspect) != 0 &&
			h->min_aspect.x > 0 && h->min_aspect.y > 0 &&
			h->max_aspect.x > 0 && h->max_aspect.y > 0 &&
			h->min_aspect.x * h->max_aspect.y <=
				h->max_aspect.x * h->min_aspect.y) {
		int minwidth = height * h->min_aspect.x / h->min_aspect.y;
		if (width < minwidth)
			width = minwidth;
		int minheight = width * h->max_aspect.y / h->max_aspect.x;
		if (height < minheight)
			height = minheight;
	}

	int basewidth;
	int baseheight;

	if (h->flags & PBaseSize) {
		basewidth = h->base_width;
		baseheight = h->base_height;
	} else if (h->flags & PMinSize) {
		basewidth = h->min_width;
		baseheight = h->min_height;
	} else {
		basewidth = 0;
		baseheight = 0;
	}

	// Cannot be less than the base (or minimum)
	width = MAX(basewidth, width);
	height = MAX(baseheight, height);

	if (h->flags & PResizeInc) {
		if (h->width_inc != 0)
			width -= (width - basewidth) % h->width_inc;
		if (h->height_inc != 0)
			height -= (height - baseheight) % h->height_inc;
	}

	if (h->flags & PMaxSize) {
		width = MIN(h->max_width, width);
		height = MIN(h->max_height, height);
	}

	width = MAX(1, width);
	height = MAX(1, height);

	*rwidth = width;
	*rheight = height;
}
//@+node:caminhante.20240208150003.1: ** csendconf
void csendconf(struct client *c)
{
	struct geometry g = cgetgeom(c);
	XSendEvent(dpy, c->window, False, StructureNotifyMask,
			(XEvent *)&(XConfigureEvent){
				.type = ConfigureNotify,
				.event = c->window,
				.window = c->window,
				.x = g.x,
				.y = g.y,
				.width = g.width,
				.height = g.height,
				.border_width = g.borderwidth,
				.above = None,
				.override_redirect = False });
}
//@+node:caminhante.20240208145950.1: ** chasfocus
Bool chasfocus(struct client *c)
{
	return c->hasfocus;
}
//@+node:caminhante.20240208145943.1: ** cgetwin
Window cgetwin(struct client *c)
{
	return c->window;
}
//@+node:caminhante.20240208145936.1: ** csetnetwmname
void csetnetwmname(struct client *c, const char *name)
{
	free(c->netwmname);
	c->netwmname = (name == NULL) ? NULL : xstrdup(name);

	if (c->frame != NULL)
		fupdate(c->frame);
}
//@+node:caminhante.20240208145931.1: ** cignoreunmap
void cignoreunmap(struct client *c)
{
	assert(c->ismapped);
	c->ignoreunmapcount++;
}
//@+node:caminhante.20240208145923.1: ** cismapped
Bool cismapped(struct client *c)
{
	return c->ismapped;
}
//@+node:caminhante.20240208145918.1: ** cisurgent
Bool cisurgent(struct client *c)
{
	return c->wmhints != NULL && (c->wmhints->flags & XUrgencyHint) != 0;
}
//@+node:caminhante.20240208145912.1: ** creframe
static void creframe(struct client *c)
{
	if (cisframed(c)) {
		if (c->frame == NULL)
			c->frame = fcreate(c);
	} else if (c->frame != NULL) {
		fdestroy(c->frame);
		c->frame = NULL;
	}
}
//@+node:caminhante.20240208145906.1: ** cisframed
static Bool cisframed(struct client *c)
{
	return !c->isfull && !c->isdock && !c->isundecorated;
}
//@+node:caminhante.20240208145859.1: ** csetskiptaskbar
void csetskiptaskbar(struct client *c, Bool skiptaskbar)
{
	c->skiptaskbar = skiptaskbar;
}
//@+node:caminhante.20240208145851.1: ** smartpos
/*
 * Find a good location for the specified client and move it there.
 *
 * A 'good' location is found by testing lots of random locations and
 * picking the one with the lowest 'badness' score. Overlapping another
 * window is very bad. Being far from screen edges is pretty bad, as
 * that tends to break up free areas.
 *
 * Window placement is about the only intelligent task of a window
 * manager, and it's worth to spend some extra CPU time here in order
 * to find a really good location.
 */
static void smartpos(struct client *c)
{
	struct geometry g = c->frame == NULL ?
			cgetgeom(c) : fgetgeom(c->frame);

	struct client **v;
	int n;
	getclientstack(&v, &n);

	// Exclude the window itself, and clients on other desks
	for (int i = 0; i < n; i++)
		if (v[i] == c || !samedesk(v[i], c))
			v[i--] = v[--n];

	unsigned long min = ~0;
	struct geometry best = g;
	for (int k = 0; min != 0 && k < 100; k++) {
		randpos(&g);
		unsigned long badness = 0;
		unsigned overlaps = 0;

		// Compute overlapping area.
		for (int i = 0; i < n; i++) {
			struct geometry g2 = v[i]->frame == NULL ?
					cgetgeom(v[i]) : fgetgeom(v[i]->frame);
			unsigned long area = overlaparea(g, g2);
			if (area > 0) {
				badness += area;
				overlaps++;
			}
		}
		// The more overlapping windows the worse.
		badness *= overlaps * overlaps;

		// Prefer to position a window near the edges of the display.
		unsigned x2 = DisplayWidth(dpy, scr) - (g.x + g.width);
		badness += MIN((unsigned)g.x, x2);
		unsigned y2 = DisplayHeight(dpy, scr) - (g.y + g.height);
		badness += MIN((unsigned)g.y, y2);

		if (badness < min) {
			min = badness;
			best = g;
		}
	}

	move(c, best.x, best.y);

	free(v);
}
//@+node:caminhante.20240208145834.1: ** randpos
/*
 * Find a random location for the specified geometry.
 */
static void randpos(struct geometry *g)
{
	int maxx = DisplayWidth(dpy, scr) - (g->width + 2 * g->borderwidth);
	int maxy = DisplayHeight(dpy, scr) - (g->height + 2 * g->borderwidth);
	g->x = maxx > 0 ? rand() % maxx : 0;
	g->y = maxy > 0 ? rand() % maxy : 0;
}
//@+node:caminhante.20240208145826.1: ** samedesk
/*
 * Return true if, and only if, the two clients are visible on the same desk.
 */
static Bool samedesk(struct client *c1, struct client *c2)
{
	return c1->desk == c2->desk ||
			c1->desk == DESK_ALL || c2->desk == DESK_ALL;
}
//@+node:caminhante.20240208145817.1: ** overlaparea
static unsigned long overlaparea(struct geometry g1, struct geometry g2)
{
	int x1 = g1.x;
	int x2 = g2.x;
	int x3 = g1.x + g1.width + 2 * g1.borderwidth;
	int x4 = g2.x + g2.width + 2 * g2.borderwidth;
	int y1 = g1.y;
	int y2 = g2.y;
	int y3 = g1.y + g1.height + 2 * g1.borderwidth;
	int y4 = g2.y + g2.height + 2 * g2.borderwidth;

	if (x1 < x4 && x2 < x3 && y1 < y4 && y2 < y3) {
		unsigned long x = MIN(x4 - x1, x3 - x2);
		unsigned long y = MIN(y4 - y1, y3 - y2);
		unsigned long area = x * y;
		return area;
	} else
		return 0;
}
//@+node:caminhante.20240208145808.1: ** move
/*
 * XXX: We move a window by simulating a ConfigureRequest from
 *      the client.
 */
static void move(struct client *c, int x, int y)
{
	Window parent = c->frame == NULL ? root : fgetwin(c->frame);
	XEvent e;
	e.xconfigurerequest = (XConfigureRequestEvent){
			.type = ConfigureRequest,
			.serial = 0,
			.send_event = True,
			.display = dpy,
			.parent = parent,
			.window = c->window,
			.x = x,
			.y = y,
			.value_mask = CWX | CWY };
	redirect(&e, parent);
}
//@-others
//@-leo
