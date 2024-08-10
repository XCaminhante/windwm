//@+leo-ver=5-thin
//@+node:caminhante.20240208143459.33: * @file button.c
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
//@+node:caminhante.20240208145234.1: ** /includes
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "wind.h"
//@+node:caminhante.20240208145230.1: ** struct button
struct button {
  struct listener listener;
  void (*function)(void *, Time);
  void *arg;
  struct bitmap *bitmap;
  Pixmap pixmap;
  int width;
  int height;
  Window window;
  Bool pressed;
  Bool entered;
};
//@+node:caminhante.20240208145222.1: ** /protótipos
static void bupdate(struct button *);
static void buttonpress(struct button *, XButtonEvent *);
static void buttonrelease(struct button *, XButtonEvent *);
static void enternotify(struct button *, XCrossingEvent *);
static void leavenotify(struct button *, XCrossingEvent *);
static void expose(struct button *, XExposeEvent *);
static void unmapnotify(struct button *, XUnmapEvent *);
static void bevent(void *, XEvent *);
//@+node:caminhante.20240208145215.1: ** bupdate
static void bupdate (struct button *b) {
  //BUG por algum motivo o valor de b->bitmap recebido as vezes é um ponteiro inválido
  DEBUG_EXPR("%p", b->bitmap );
#ifdef BIT32
  uint32_t bitmap = (uint32_t)(b->bitmap) & 0xFFFFFF00;
  //BUG nesses casos, b->bitmap->width não está alocado
  // DEBUG_EXPR("%d", b->bitmap->width );
  if (bitmap >= 0x400000) { return; }
#elifdef BIT64
  uint64_t bitmap = (uint64_t)(b->bitmap) & 0xFFFFFFFFFFF00000;
  if (bitmap >= 0x400000 && bitmap < 0x555555500000) { return; }
#endif
  Bool invert = b->pressed && b->entered;
  GC fg = invert ? background : foreground;
  GC bg = invert ? foreground : background;

  XFillRectangle(dpy, b->pixmap, bg, 0, 0, b->width, b->height);

  drawbitmap(b->pixmap, fg, b->bitmap, (b->width - b->bitmap->width) / 2, (b->height - b->bitmap->height) / 2);

  if (!invert) {
    XSetLineAttributes(dpy, fg,
      b->entered ? 1 + 2 * halfleading : 0,
      LineSolid, CapButt, JoinMiter);
    XDrawRectangle(dpy, b->pixmap, fg,
      0, 0, b->width - 1, b->height - 1);
    XSetLineAttributes(dpy, fg, 0, LineSolid, CapButt, JoinMiter);
  }

  XCopyArea(dpy, b->pixmap, b->window, fg,
    0, 0, b->width, b->height, 0, 0);
}
//@+node:caminhante.20240208145210.1: ** buttonpress
static void buttonpress(struct button *b, XButtonEvent *e) {
  (void) e;
  b->pressed = True;
  // DEBUG_EXPR("%p", b->bitmap);
  bupdate(b);
}
//@+node:caminhante.20240208145204.1: ** buttonrelease
static void buttonrelease(struct button *b, XButtonEvent *e) {
  if (e->button == Button1) {
    if (b->pressed && b->entered) { b->function(b->arg, e->time); }
    b->pressed = False;
    // DEBUG_EXPR("%p", b->bitmap);
    bupdate(b);
  }
}
//@+node:caminhante.20240208145155.1: ** enternotify
static void enternotify (struct button *b, XCrossingEvent *e) {
  (void) e;
  b->entered = True;
  // DEBUG_EXPR("%p", b->bitmap);
  bupdate(b);
}
//@+node:caminhante.20240208145149.1: ** leavenotify
static void leavenotify(struct button *b, XCrossingEvent *e) {
  (void) e;
  if (b->entered) {
    b->entered = False;
    // DEBUG_EXPR("%p", b->bitmap);
    bupdate(b);
  }
}
//@+node:caminhante.20240208145142.1: ** unmapnotify
static void unmapnotify(struct button *b, XUnmapEvent *e) {
  (void) e;
  if (b->pressed) {
    b->pressed = False;
    // DEBUG_EXPR("%p", b->bitmap);
    bupdate(b);
  }
}
//@+node:caminhante.20240208145137.1: ** expose
static void expose(struct button *b, XExposeEvent *e) {
  XCopyArea(dpy, b->pixmap, b->window, foreground,
    e->x, e->y, e->width, e->height, e->x, e->y);
}
//@+node:caminhante.20240208145133.1: ** bevent
static void bevent(void *self, XEvent *e) {
  switch (e->type) {
  case Expose:
    expose(self, &e->xexpose); break;
  case EnterNotify:
    enternotify(self, &e->xcrossing); break;
  case LeaveNotify:
    leavenotify(self, &e->xcrossing); break;
  case ButtonPress:
    buttonpress(self, &e->xbutton); break;
  case ButtonRelease:
    buttonrelease(self, &e->xbutton); break;
  case UnmapNotify:
    unmapnotify(self, &e->xunmap); break;
  }
}
//@+node:caminhante.20240208145126.1: ** bcreate
struct button *bcreate(
  void (*function)(void *, Time),
  void *arg, struct bitmap *bitmap,
  Window parent, int x, int y, int width,
  int height, int gravity
) {
  struct button *b = xmalloc(sizeof *b);
  b->function = function;
  b->arg = arg;
  // DEBUG_EXPR("%p", bitmap);
  // DEBUG_EXPR("%d", bitmap->width);
  b->bitmap = bitmap;
  b->width = width;
  b->height = height;
  b->pixmap = XCreatePixmap(dpy, root, width, height,
    DefaultDepth(dpy, scr));
  b->pressed = False;
  b->entered = False;
  b->window = XCreateWindow(dpy, parent, x, y, width, height, 0,
    CopyFromParent, InputOutput, CopyFromParent,
    CWWinGravity,
    &(XSetWindowAttributes){
      .win_gravity = gravity });
  b->listener.function = bevent;
  b->listener.pointer = b;
  setlistener(b->window, &b->listener);
  XGrabButton(dpy, Button1, AnyModifier, b->window, False,
    EnterWindowMask | LeaveWindowMask | ButtonReleaseMask,
    GrabModeAsync, GrabModeAsync, None, None);
  XSelectInput(dpy, b->window,
    EnterWindowMask | LeaveWindowMask |
    StructureNotifyMask | ExposureMask);
  bupdate(b);
  XMapWindow(dpy, b->window);
  return b;
}
//@+node:caminhante.20240208145120.1: ** bdestroy
void bdestroy(struct button *b)
{
	setlistener(b->window, NULL);
	XFreePixmap(dpy, b->pixmap);
	XDestroyWindow(dpy, b->window);
	free(b);
}
//@-others
//@-leo
