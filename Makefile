CFLAGS := -Wall -Wextra -Werror -Wfatal-errors \
-Wno-unused-function -Wno-ignored-qualifiers \
-Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing \
--std=gnu2x -O0 -g3

PROGFLAGS := $(shell pkg-config x11 --cflags --libs) \
	$(shell pkg-config xft --cflags --libs) \
	$(shell pkg-config freetype2 --cflags --libs)

all: windwm
	DISPLAY=:1 xinit ./windwm -c gmrun -- :1 vt1

.PHONY: test
test:
	gcc windwm.c $(CFLAGS) $(PROGFLAGS) -c -o /dev/null

windwm: windwm.c Makefile *.c *.h
	gcc windwm.c $(CFLAGS) $(PROGFLAGS) -o "$@"
