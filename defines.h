//@+leo-ver=5-thin
//@+node:caminhante.20240208144745.1: * @file defines.h
#define PACKAGE_STRING "WindWM"
#define PACKAGE_NAME "windwm"
#define PACKAGE_BUGREPORT "<xcaminhante@i2pmail.org>"

// #include "x11font.c"
#include "xftfont.c"

#define STRING(EXPR) #EXPR
#define DEBUG_EXPR(PRINTFFLAG, EXPR) printf ("* Debug probe, %s, %s:%d, '%s' = " PRINTFFLAG "\n", __FILE__, __PRETTY_FUNCTION__, __LINE__, STRING(EXPR), EXPR);
//@-leo
