#!/bin/sh
gcc -pedantic -Wall -lSDL -lSDL_gfx -O2 -ffast-math cslime.c cslime_ui.c cslime_ai.c vector.c -o cslime
