//@+leo-ver=5-thin
//@+node:caminhante.20240208143459.7: * @file root.c
//@@tabwidth -2
//@@language c
//@+others
//@+node:caminhante.20240208145241.1: ** /copyright notice
/*
 * Copyright 2010 Johan Veenhuizen
 * Copyleft 2024 X Caminhante
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
//@+node:caminhante.20240208161456.1: ** /includes
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "wind.h"
//@+node:caminhante.20240208161452.1: ** /protótipos
static void fnkey(KeySym, unsigned, Time, int);
static void rrun_command(KeySym, unsigned, Time, int);
static void rconfigurerequest(XConfigureRequestEvent *);
static void rmaprequest(XMapRequestEvent *);
static void rkeypress(XKeyEvent *);
static void rclientmessage(XClientMessageEvent *);
static void runmapnotify(XUnmapEvent *);
static void renternotify(XCrossingEvent *);
static void rleavenotify(XCrossingEvent *);
static void revent(void *, XEvent *);
//@+node:caminhante.20240208161447.1: ** /variáveis
// True if the pointer is on this screen
static Bool pointerhere;

static const struct listener listener = {
  .function = revent,
  .pointer = NULL,
};
//@+node:caminhante.20240208161429.1: ** struct keybind rkeymap[]
static struct keybind {
  KeySym keysym;
  unsigned modifiers;
  void (*function)(KeySym key, unsigned state, Time t, int arg);
  int arg;
  KeyCode keycode;
} rkeymap[] = {
  { XK_F1, Mod1Mask, fnkey, 1, 0 },
  { XK_F2, Mod1Mask, fnkey, 2, 0 },
  { XK_F3, Mod1Mask, fnkey, 3, 0 },
  { XK_F4, Mod1Mask, fnkey, 4, 0 },
  { XK_F5, Mod1Mask, fnkey, 5, 0 },
  { XK_F6, Mod1Mask, fnkey, 6, 0 },
  { XK_F7, Mod1Mask, fnkey, 7, 0 },
  { XK_F8, Mod1Mask, fnkey, 8, 0 },
  { XK_F9, Mod1Mask, fnkey, 9, 0 },
  { XK_F10, Mod1Mask, fnkey, 10, 0 },
  { XK_F11, Mod1Mask, fnkey, 11, 0 },
  { XK_F12, Mod1Mask, fnkey, 12, 0 },

  { XK_F1, ShiftMask | Mod1Mask, fnkey, 1, 0 },
  { XK_F2, ShiftMask | Mod1Mask, fnkey, 2, 0 },
  { XK_F3, ShiftMask | Mod1Mask, fnkey, 3, 0 },
  { XK_F4, ShiftMask | Mod1Mask, fnkey, 4, 0 },
  { XK_F5, ShiftMask | Mod1Mask, fnkey, 5, 0 },
  { XK_F6, ShiftMask | Mod1Mask, fnkey, 6, 0 },
  { XK_F7, ShiftMask | Mod1Mask, fnkey, 7, 0 },
  { XK_F8, ShiftMask | Mod1Mask, fnkey, 8, 0 },
  { XK_F9, ShiftMask | Mod1Mask, fnkey, 9, 0 },
  { XK_F10, ShiftMask | Mod1Mask, fnkey, 10, 0 },
  { XK_F11, ShiftMask | Mod1Mask, fnkey, 11, 0 },
  { XK_F12, ShiftMask | Mod1Mask, fnkey, 12, 0 },

  { XK_Tab, Mod1Mask, rrun_command, 0, 0 },
};
//@+node:caminhante.20240208161407.1: ** fnkey
static void fnkey (KeySym keysym, unsigned state, Time time, int arg) {
  (void) keysym;
  if (state & ShiftMask) {
    setndesk(arg); }
  gotodesk(arg - 1);
  refocus(time);
}
//@+node:caminhante.20240210171913.1: ** rrun_command
static void rrun_command (KeySym keysym, unsigned state, Time time, int arg) {
  (void) keysym; (void) state; (void) time; (void) arg;
  if (call_argc == 0) {
    puts("rrun_command: no command was specified as alt+tab run command");
    return; }

  printf("keypress_run: running");
  for (size_t argc=0; argc<call_argc; argc++) {
    printf(" %s", call_argv[argc]); }
  putchar(10);

  int pid = fork();
  switch (pid) {
  case -1:
    puts("fork() error");
    break;
  case 0:
    execvp(call_argv[0], call_argv);
  default:
    break;
  }
  refocus(time);
}
//@+node:caminhante.20240208161402.1: ** rconfigurerequest
static void rconfigurerequest (XConfigureRequestEvent *e) {
  // First try to redirect the event.
  if (redirect((XEvent *)e, e->window) == 0) { return; }

  // Nobody listens to this window so we'll just
  // do whatever it wants us to do.

  // Ignore stacking requests for now.
  e->value_mask &= ~(CWSibling | CWStackMode);

  XConfigureWindow(dpy, e->window, e->value_mask,
      &(XWindowChanges){
        .x = e->x,
        .y = e->y,
        .width = e->width,
        .height = e->height,
        .border_width = e->border_width,
        .sibling = e->above,
        .stack_mode = e->detail });
}
//@+node:caminhante.20240208161355.1: ** rmaprequest
static void rmaprequest (XMapRequestEvent *e) {
  // Already managed?
  if (redirect((XEvent *)e, e->window) == 0) { return; }

  // Try to manage it, otherwise just map it.
  if (manage(e->window) == NULL) {
    XMapWindow(dpy, e->window); }
}
//@+node:caminhante.20240208161347.1: ** rkeypress
static void rkeypress (XKeyEvent *e) {
  for (size_t i = 0; i < NELEM(rkeymap); i++) {
    if (rkeymap[i].keycode == e->keycode) {
      rkeymap[i].function(rkeymap[i].keysym, e->state,
          e->time, rkeymap[i].arg);
      break;
  }}
}
//@+node:caminhante.20240208161343.1: ** rclientmessage
static void rclientmessage (XClientMessageEvent *e) {
  ewmh_rootclientmessage(e);
}
//@+node:caminhante.20240208161337.1: ** runmapnotify
/*
 * Refer to the ICCCM section 4.1.4, "Changing Window State",
 * for information on the synthetic UnmapNotify event sent
 * by clients to the root window on withdrawal.
 */
static void runmapnotify (XUnmapEvent *e) {
  if (e->send_event) {
    redirect((XEvent *)e, e->window); }
}
//@+node:caminhante.20240208161329.1: ** renternotify
/*
 * Refocus whenever the pointer enters our root window from
 * another screen.
 */
static void renternotify (XCrossingEvent *e) {
  if (e->detail == NotifyNonlinear ||
  e->detail == NotifyNonlinearVirtual) {
    pointerhere = True;
    refocus(e->time);
  }
}
//@+node:caminhante.20240208161323.1: ** rleavenotify
/*
 * Give up focus if the pointer leaves our screen.
 */
static void rleavenotify (XCrossingEvent *e) {
  if (e->detail == NotifyNonlinear ||
  e->detail == NotifyNonlinearVirtual) {
    pointerhere = False;
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, e->time);
  }
}
//@+node:caminhante.20240208161243.1: ** revent
static void revent (void *self, XEvent *e) {
      (void) self;
  switch (e->type) {
  case MapRequest:
    rmaprequest(&e->xmaprequest);
    break;
  case ConfigureRequest:
    rconfigurerequest(&e->xconfigurerequest);
    break;
  case KeyPress:
    rkeypress(&e->xkey);
    break;
  case ClientMessage:
    rclientmessage(&e->xclient);
    break;
  case UnmapNotify:
    runmapnotify(&e->xunmap);
    break;
  case EnterNotify:
    renternotify(&e->xcrossing);
    break;
  case LeaveNotify:
    rleavenotify(&e->xcrossing);
    break;
  }
}
//@+node:caminhante.20240208161227.1: ** initroot
void initroot (void) {
  setlistener(root, &listener);

  XSync(dpy, False);
  xerror = NULL;
  XSelectInput(dpy, root,
      EnterWindowMask |
      LeaveWindowMask |
      SubstructureRedirectMask |
      SubstructureNotifyMask);
  XSync(dpy, False);
  if (xerror != NULL) {
    errorf("display \"%s\" already has a window manager",
        XDisplayName(displayname));
    exit(1);
  }
      //NOTE keycode é inicializado aqui, por isso é definido como 0 em rkeymap
  for (size_t i = 0; i < NELEM(rkeymap); i++) {
    struct keybind *k = &rkeymap[i];
    k->keycode = XKeysymToKeycode(dpy, k->keysym);
    grabkey(k->keycode, k->modifiers, root, True,
        GrabModeAsync, GrabModeAsync);
  }

  Window r, c;
  int rx, ry, x, y;
  unsigned m;
  XQueryPointer(dpy, root, &r, &c, &rx, &ry, &x, &y, &m);
  pointerhere = (r == root);
}
//@-others
//@-leo
