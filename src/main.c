#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "level.h"

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  Level level;
  float cell_size;
  float board_offset_x;
  float board_offset_y;
} NumberFactory;

const int width = 800;
const int height = 600;

SDL_AppResult SDL_AppInit(void **state, int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  *state = NULL;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  NumberFactory *game = SDL_calloc(1, sizeof(*game));
  if (!game) {
    SDL_Log("Failed to allocate game state");
    return SDL_APP_FAILURE;
  }
  *state = game;

  level_init(&game->level);
  float cell_width = (float)width / game->level.board.width;
  float cell_height = (float)height / game->level.board.height;
  game->cell_size = cell_width < cell_height ? cell_width : cell_height;
  game->board_offset_x =
      (width - game->cell_size * game->level.board.width) / 2.0f;
  game->board_offset_y =
      (height - game->cell_size * game->level.board.height) / 2.0f;

  // TODO: remove this temp pipe
  Vector2i pipe_position = {.x = 1, .y = 1};
  EntityId pipe_id =
      entity_create(&game->level.entity_pool, (Entity){.type = ENTITY_PIPE});
  pipe_init(&ENTITY_AT(&game->level.entity_pool, pipe_id)->pipe, pipe_position);
  BOARD_AT(&game->level.board, pipe_position.x, pipe_position.y) = pipe_id;

  if (!SDL_CreateWindowAndRenderer("jam", width, height, 0, &game->window,
                                   &game->renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *state) {
  NumberFactory *game = state;

  SDL_SetRenderDrawColor(game->renderer, 220, 190, 200, 255);
  SDL_RenderClear(game->renderer);

  for (int x = 0; x < game->level.board.width; x++) {
    for (int y = 0; y < game->level.board.height; y++) {
      EntityId id = BOARD_AT(&game->level.board, x, y);
      if (id == 0) continue;
      Entity *entity = ENTITY_AT(&game->level.entity_pool, id);
      SDL_FRect rect = {
          .x = game->board_offset_x + x * game->cell_size,
          .y = game->board_offset_y + y * game->cell_size,
          .w = game->cell_size,
          .h = game->cell_size,
      };

      switch (entity->type) {
      case ENTITY_PIPE:
        SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(game->renderer, &rect);
        break;
      case ENTITY_NONE:
        break;
      }
    }
  }

  SDL_RenderPresent(game->renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *state, SDL_Event *event) {
  (void)state;

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *state, SDL_AppResult result) {
  (void)result;
  NumberFactory *game = state;
  level_free(&game->level);
  if (game->renderer) SDL_DestroyRenderer(game->renderer);
  if (game->window) SDL_DestroyWindow(game->window);
  SDL_free(game);
}
