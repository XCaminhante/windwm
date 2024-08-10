CFLAGS := -Wall -Wextra -Werror -Wfatal-errors \
    -Wno-unused-function -Wno-ignored-qualifiers \
    -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing \
    --std=gnu2x -O0 -g3

PROGFLAGS := $(shell pkg-config x11 --cflags --libs) \
    $(shell pkg-config xft --cflags --libs) \
    $(shell pkg-config freetype2 --cflags --libs)

all: windwm
	DISPLAY=:0 ./windwm

windwm: windwm.c Makefile *.c *.h
	gcc "$<" $(CFLAGS) $(PROGFLAGS) -o "$@"
