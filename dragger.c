//@+leo-ver=5-thin
//@+node:caminhante.20240209172034.16: * @file dragger.c
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
//@+node:caminhante.20240210012928.1: ** /includes
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "wind.h"
//@+node:caminhante.20240210012923.1: ** struct dragger
struct dragger {
  struct listener listener;
  void (*dragnotify) (void *, int, int, unsigned long, Time);
  void *arg;
  unsigned long counter;
  Window window;
  int x0;
  int y0;
  int x;
  int y;
};
//@+node:caminhante.20240210012917.1: ** /protótipos
static void devent(void *, XEvent *);
static void dbuttonpress(struct dragger *, XButtonEvent *);
static void motionnotify(struct dragger *, XMotionEvent *);
//@+node:caminhante.20240210012911.1: ** dcreate
struct dragger* dcreate (
  Window parent, int x, int y,
  int width, int height, int gravity, Cursor cursor,
  void (*dragnotify) (void *, int, int, unsigned long, Time),
  void *arg
) {
  struct dragger *d = xmalloc(sizeof *d);
  d->window = XCreateWindow(dpy, parent, x, y, width, height, 0,
      CopyFromParent, InputOnly, CopyFromParent,
      CWWinGravity | CWCursor,
      &(XSetWindowAttributes){
        .win_gravity = gravity,
        .cursor = cursor });
  d->listener.function = devent;
  d->listener.pointer = d;
  setlistener(d->window, &d->listener);
  d->counter = 0;

  switch (gravity) {
  case NorthWestGravity:
  case WestGravity:
  case SouthWestGravity:
    d->x0 = 0;
    break;
  case NorthGravity:
  case CenterGravity:
  case SouthGravity:
    d->x0 = width / 2;
    break;
  case NorthEastGravity:
  case EastGravity:
  case SouthEastGravity:
    d->x0 = width - 1;
    break;
  default:
    d->x0 = 0;
    break;
  }

  switch (gravity) {
  case NorthWestGravity:
  case NorthGravity:
  case NorthEastGravity:
    d->y0 = 0;
    break;
  case WestGravity:
  case CenterGravity:
  case EastGravity:
    d->y0 = height / 2;
    break;
  case SouthWestGravity:
  case SouthGravity:
  case SouthEastGravity:
    d->y0 = height - 1;
    break;
  default:
    d->y0 = 0;
    break;
  }

  d->x = 0;
  d->y = 0;
  d->dragnotify = dragnotify;
  d->arg = arg;
  XGrabButton(dpy, Button1, AnyModifier, d->window, False,
      Button1MotionMask, GrabModeAsync, GrabModeAsync,
      None, cursor);
  XMapWindow(dpy, d->window);
  return d;
}
//@+node:caminhante.20240210012859.1: ** ddestroy
void ddestroy (struct dragger *d) {
  setlistener(d->window, NULL);
  XDestroyWindow(dpy, d->window);
  free(d);
}
//@+node:caminhante.20240210012855.1: ** devent
static void devent (void *self, XEvent *e) {
  switch (e->type) {
  case MotionNotify:
    motionnotify(self, &e->xmotion);
    break;
  case ButtonPress:
    dbuttonpress(self, &e->xbutton);
    break;
  }
}
//@+node:caminhante.20240210012823.1: ** dbuttonpress
static void dbuttonpress (struct dragger *d, XButtonEvent *e) {
  d->counter = 0;
  d->x = e->x - d->x0;
  d->y = e->y - d->y0;
  if (d->dragnotify != NULL) {
    d->dragnotify(d->arg,
        e->x_root - d->x,
        e->y_root - d->y,
        d->counter++, e->time); }
}
//@+node:caminhante.20240210012755.1: ** motionnotify
static void motionnotify (struct dragger *d, XMotionEvent *e) {
  if (d->dragnotify != NULL) {
    d->dragnotify(d->arg,
        e->x_root - d->x, e->y_root - d->y,
        d->counter++, e->time); }
}
//@-others
//@-leo
