#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef ALLEGRO
    #include <allegro5/allegro5.h>
    #include <allegro5/allegro_primitives.h>
    #include <allegro5/allegro_font.h>
    #include <allegro5/allegro_ttf.h>
#elif SDL2
    #include <SDL2/SDL.h>
#endif

#include "util.h"