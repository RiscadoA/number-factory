#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#include "game.h"
#include "start_menu_state.h"

SDL_AppResult SDL_AppInit(void **state, int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  *state = NULL;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  NumberFactory *game = calloc(1, sizeof(*game));
  if (!game) {
    SDL_Log("Failed to allocate game state");
    return SDL_APP_FAILURE;
  }

  *state = game;
  game_state_stack_init(&game->states);

  if (!SDL_CreateWindowAndRenderer("Number Factory", GAME_WIDTH, GAME_HEIGHT, 0,
                                   &game->window, &game->renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);

  if (!TTF_Init()) {
    SDL_Log("TTF_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  game->ttf_initialized = 1;

  game->font = TTF_OpenFont("assets/fonts/PressStart2P-Regular.ttf", 64.0f);
  if (!game->font) {
    SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  GameState *menu = start_menu_state_create(game);
  if (!menu) return SDL_APP_FAILURE;

  if (!game_state_stack_push(&game->states, menu)) {
    menu->destroy(menu, game);
    SDL_Log("Failed to push start menu state");
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *state) {
  NumberFactory *game = state;

  SDL_AppResult result = game_state_stack_update(&game->states, game);
  if (result != SDL_APP_CONTINUE) return result;

  game_state_stack_render(&game->states, game);
  SDL_RenderPresent(game->renderer);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *state, SDL_Event *event) {
  NumberFactory *game = state;

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  return game_state_stack_event(&game->states, game, event);
}

void SDL_AppQuit(void *state, SDL_AppResult result) {
  (void)result;
  NumberFactory *game = state;
  if (!game) return;

  game_state_stack_free(&game->states, game);
  if (game->font) TTF_CloseFont(game->font);
  if (game->ttf_initialized) TTF_Quit();
  if (game->renderer) SDL_DestroyRenderer(game->renderer);
  if (game->window) SDL_DestroyWindow(game->window);
  free(game);
}
