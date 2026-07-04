#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "level.h"

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  Level level;
  float cell_size;
  float board_offset_x;
  float board_offset_y;
  Vector2i hovered_cell;
  int has_hovered_cell;
  Orientation orientation;
} NumberFactory;

const int width = 800;
const int height = 600;

static int screen_to_board_cell(NumberFactory *game, float x, float y,
                                Vector2i *cell) {
  float board_x = x - game->board_offset_x;
  float board_y = y - game->board_offset_y;
  float board_width = game->cell_size * game->level.board.width;
  float board_height = game->cell_size * game->level.board.height;

  if (board_x < 0.0f || board_x >= board_width || board_y < 0.0f ||
      board_y >= board_height) {
    return 0;
  }

  cell->x = (int)(board_x / game->cell_size);
  cell->y = (int)(board_y / game->cell_size);
  return 1;
}

static void draw_orientation_arrow(SDL_Renderer *renderer, SDL_FRect cell,
                                   Orientation orientation) {
  Vector2i direction = orientation_vector(orientation);
  Vector2i perpendicular = {.x = -direction.y, .y = direction.x};
  float block_size = cell.w / 9.0f;
  float center_x = cell.x + cell.w / 2.0f - block_size / 2.0f -
                   direction.x * block_size / 2.0f;
  float center_y = cell.y + cell.h / 2.0f - block_size / 2.0f -
                   direction.y * block_size / 2.0f;
  // x is distance forward and y is distance sideways from the shaft.
  Vector2i blocks[] = {
      {.x = 3, .y = 0},
      {.x = 2, .y = -1},
      {.x = 2, .y = 0},
      {.x = 2, .y = 1},
      {.x = 1, .y = -2},
      {.x = 1, .y = -1},
      {.x = 1, .y = 0},
      {.x = 1, .y = 1},
      {.x = 1, .y = 2},
      {.x = 0, .y = 0},
      {.x = -1, .y = 0},
      {.x = -2, .y = 0},
  };

  for (int i = 0; i < 12; i++) {
    int x = blocks[i].x * direction.x + blocks[i].y * perpendicular.x;
    int y = blocks[i].x * direction.y + blocks[i].y * perpendicular.y;
    SDL_FRect block = {
        .x = center_x + x * block_size,
        .y = center_y + y * block_size,
        .w = block_size,
        .h = block_size,
    };
    SDL_RenderFillRect(renderer, &block);
  }
}

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
  game->orientation = NORTH;
  float cell_width = (float)width / game->level.board.width;
  float cell_height = (float)height / game->level.board.height;
  game->cell_size = cell_width < cell_height ? cell_width : cell_height;
  game->board_offset_x =
      (width - game->cell_size * game->level.board.width) / 2.0f;
  game->board_offset_y =
      (height - game->cell_size * game->level.board.height) / 2.0f;

  if (!SDL_CreateWindowAndRenderer("jam", width, height, 0, &game->window,
                                   &game->renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);

  if (!TTF_Init()) {
    SDL_Log("TTF_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  game->font = TTF_OpenFont("assets/fonts/PressStart2P-Regular.ttf", 64.0f);
  if (!game->font) {
    SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
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

  if (game->has_hovered_cell) {
    SDL_FRect preview = {
        .x = game->board_offset_x + game->hovered_cell.x * game->cell_size,
        .y = game->board_offset_y + game->hovered_cell.y * game->cell_size,
        .w = game->cell_size,
        .h = game->cell_size,
    };
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 48);
    SDL_RenderFillRect(game->renderer, &preview);
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 192);
    draw_orientation_arrow(game->renderer, preview, game->orientation);
  }

  SDL_RenderPresent(game->renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *state, SDL_Event *event) {
  NumberFactory *game = state;

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_R &&
      !event->key.repeat) {
    game->orientation =
        (Orientation)((game->orientation + 1) % ORIENTATION_COUNT);
  }

  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    game->has_hovered_cell = screen_to_board_cell(
        game, event->motion.x, event->motion.y, &game->hovered_cell);
  } else if (event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
    game->has_hovered_cell = 0;
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      (event->button.button == SDL_BUTTON_LEFT ||
       event->button.button == SDL_BUTTON_RIGHT)) {
    Vector2i cell;
    if (screen_to_board_cell(game, event->button.x, event->button.y, &cell)) {
      if (event->button.button == SDL_BUTTON_LEFT) {
        level_place_pipe(&game->level, cell, game->orientation);
      } else {
        level_remove(&game->level, cell);
      }
    }
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *state, SDL_AppResult result) {
  (void)result;
  NumberFactory *game = state;
  level_free(&game->level);
  if (game->font) TTF_CloseFont(game->font);
  TTF_Quit();
  if (game->renderer) SDL_DestroyRenderer(game->renderer);
  if (game->window) SDL_DestroyWindow(game->window);
  SDL_free(game);
}
