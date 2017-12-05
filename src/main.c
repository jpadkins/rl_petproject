///////////////////////////////////////////////////////////////////////////////
/// file:           main.c
/// author:         Jacob Adkins - jpadkins
/// description:    The roguelike application entry point.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glad.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////////
/// Static variables
///////////////////////////////////////////////////////////////////////////////

static FT_Face font;
static FT_Library freetype;
//static SDL_GLContext context;
static SDL_Window *window = NULL;
//static SDL_Renderer *renderer = NULL;
static const char *font_path = "res/unifont.ttf";

///////////////////////////////////////////////////////////////////////////////
/// Main
///////////////////////////////////////////////////////////////////////////////

int main(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        DEBUG(DEBUG_EXIT, "SDL2 Initialization failed");
    }

    if (!(window = SDL_CreateWindow(
        "SDL2 Application",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        0))) {
        DEBUG(DEBUG_EXIT, "Window creation failed");
    }

    if (FT_Init_FreeType(&freetype)) {
        DEBUG(DEBUG_EXIT, "FreeType initialization failed");
    }

    if (FT_New_Face(freetype, font_path, 0, &font)) {
        DEBUG(DEBUG_EXIT, "Font loading failed");
    }

    FT_Done_FreeType(freetype);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
