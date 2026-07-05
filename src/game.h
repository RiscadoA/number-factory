#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_state.h"

enum {
  GAME_WIDTH = 800,
  GAME_HEIGHT = 600,
};

struct NumberFactory {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  int ttf_initialized;
  GameStateStack states;
};

#endif
