//@+leo-ver=5-thin
//@+node:caminhante.20240208144745.1: * @file defines.h
//@@tabwidth -2
#define PACKAGE_STRING "WindWM"
#define PACKAGE_NAME "windwm"
#define PACKAGE_BUGREPORT "<xcaminhante@i2pmail.org>"

// #include "x11font.c"
#include "xftfont.c"

#define STRING(EXPR) #EXPR
#define DEBUG_EXPR(PRINTFFLAG, EXPR) \
  printf("* At %s:%d, function %s, expression '%s' = " PRINTFFLAG "\n", \
  __FILE__, __LINE__, __PRETTY_FUNCTION__, STRING(EXPR), EXPR);

#include <limits.h>
#if __SIZEOF_POINTER__ == 4
  #define BIT32
#elif __SIZEOF_POINTER__ == 8
  #define BIT64
#else
  #error "System must be either 32-bits or 64-bits."
#endif
//@-leo
