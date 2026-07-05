#include "gameplay_state.h"

#include <SDL3/SDL.h>
#include <stdlib.h>

#include "game.h"
#include "level.h"
#include "pipe.h"

typedef struct {
  GameState base;
  Level level;
  float cell_size;
  float board_offset_x;
  float board_offset_y;
  Vector2i hovered_cell;
  int has_hovered_cell;
  Orientation orientation;
} GameplayState;

static int screen_to_board_cell(GameplayState *state, float x, float y,
                                Vector2i *cell) {
  float board_x = x - state->board_offset_x;
  float board_y = y - state->board_offset_y;
  float board_width = state->cell_size * state->level.board.width;
  float board_height = state->cell_size * state->level.board.height;

  if (board_x < 0.0f || board_x >= board_width || board_y < 0.0f ||
      board_y >= board_height) {
    return 0;
  }

  cell->x = (int)(board_x / state->cell_size);
  cell->y = (int)(board_y / state->cell_size);
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
  Vector2i blocks[] = {
      {.x = 3, .y = 0}, {.x = 2, .y = -1}, {.x = 2, .y = 0},
      {.x = 2, .y = 1}, {.x = 1, .y = -2}, {.x = 1, .y = -1},
      {.x = 1, .y = 0}, {.x = 1, .y = 1},  {.x = 1, .y = 2},
      {.x = 0, .y = 0}, {.x = -1, .y = 0}, {.x = -2, .y = 0},
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

static void gameplay_state_destroy(GameState *base, NumberFactory *game) {
  (void)game;
  GameplayState *state = (GameplayState *)base;
  level_free(&state->level);
  free(state);
}

static SDL_AppResult gameplay_state_event(GameState *base, NumberFactory *game,
                                          SDL_Event *event) {
  (void)game;
  GameplayState *state = (GameplayState *)base;

  if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_R &&
      !event->key.repeat) {
    state->orientation =
        (Orientation)((state->orientation + 1) % ORIENTATION_COUNT);
  }

  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    state->has_hovered_cell = screen_to_board_cell(
        state, event->motion.x, event->motion.y, &state->hovered_cell);
  } else if (event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
    state->has_hovered_cell = 0;
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      (event->button.button == SDL_BUTTON_LEFT ||
       event->button.button == SDL_BUTTON_RIGHT)) {
    Vector2i cell;
    if (screen_to_board_cell(state, event->button.x, event->button.y, &cell)) {
      if (event->button.button == SDL_BUTTON_LEFT) {
        level_place_pipe(&state->level, cell, state->orientation);
      } else {
        level_remove(&state->level, cell);
      }
    }
  }

  return SDL_APP_CONTINUE;
}

static SDL_AppResult gameplay_state_update(GameState *base,
                                           NumberFactory *game) {
  (void)base;
  (void)game;
  return SDL_APP_CONTINUE;
}

static void gameplay_state_render(GameState *base, NumberFactory *game) {
  GameplayState *state = (GameplayState *)base;

  SDL_SetRenderDrawColor(game->renderer, 220, 190, 200, 255);
  SDL_RenderClear(game->renderer);

  for (int x = 0; x < state->level.board.width; x++) {
    for (int y = 0; y < state->level.board.height; y++) {
      EntityId id = BOARD_AT(&state->level.board, x, y);
      if (id == 0) continue;
      Entity *entity = ENTITY_AT(&state->level.entity_pool, id);
      switch (entity->type) {
      case ENTITY_PIPE: {
        Pipe *pipe = &entity->pipe;
        if (pipe_is_start_cell(pipe, (Vector2i){x, y}) != 1) break;
        if (pipe->cells.size == 0) break;

        static const uint8_t colors[][3] = {
            {255, 80, 80},  {80, 255, 80},  {80, 80, 255},  {255, 255, 80},
            {255, 80, 255}, {80, 255, 255}, {255, 160, 80}, {160, 80, 255},
        };
        uint8_t r = colors[id % 8][0];
        uint8_t g = colors[id % 8][1];
        uint8_t b = colors[id % 8][2];
        float half_cell = state->cell_size * 0.5f;
        float cs = state->cell_size;

        SDL_SetRenderDrawColor(game->renderer, r, g, b, 255);
        float cell_center_pad = cs * 0.3f;
        for (int i = 0; i < pipe->cells.size; i++) {
          PipeCell cell = DEQUE_AT(pipe->cells, i);
          float cx = state->board_offset_x + cell.pos.x * cs + cell_center_pad;
          float cy = state->board_offset_y + cell.pos.y * cs + cell_center_pad;
          float size = cs - cell_center_pad * 2.0f;
          SDL_FRect fill = {cx, cy, size, size};
          SDL_RenderFillRect(game->renderer, &fill);
        }

        for (int i = 0; i < pipe->cells.size - 1; i++) {
          PipeCell a = DEQUE_AT(pipe->cells, i);
          PipeCell b = DEQUE_AT(pipe->cells, i + 1);
          float ax = state->board_offset_x + a.pos.x * cs + half_cell;
          float ay = state->board_offset_y + a.pos.y * cs + half_cell;
          float bx = state->board_offset_x + b.pos.x * cs + half_cell;
          float by = state->board_offset_y + b.pos.y * cs + half_cell;
          SDL_RenderLine(game->renderer, ax, ay, bx, by);
        }

        float tick_len = state->cell_size * 0.3f;
        for (int i = 0; i < pipe->cells.size; i++) {
          PipeCell cell = DEQUE_AT(pipe->cells, i);
          float cx = state->board_offset_x + cell.pos.x * cs + half_cell;
          float cy = state->board_offset_y + cell.pos.y * cs + half_cell;
          Vector2i dir = orientation_vector(cell.orientation);
          SDL_RenderLine(game->renderer, cx, cy, cx + dir.x * tick_len,
                         cy + dir.y * tick_len);
        }
        break;
      }
      case ENTITY_NONE:
        break;
      }
    }
  }

  if (state->has_hovered_cell) {
    SDL_FRect preview = {
        .x = state->board_offset_x + state->hovered_cell.x * state->cell_size,
        .y = state->board_offset_y + state->hovered_cell.y * state->cell_size,
        .w = state->cell_size,
        .h = state->cell_size,
    };
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 48);
    SDL_RenderFillRect(game->renderer, &preview);
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 192);
    draw_orientation_arrow(game->renderer, preview, state->orientation);
  }
}

GameState *gameplay_state_create(NumberFactory *game) {
  GameplayState *state = calloc(1, sizeof(*state));
  if (!state) return NULL;

  state->base.destroy = gameplay_state_destroy;
  state->base.event = gameplay_state_event;
  state->base.update = gameplay_state_update;
  state->base.render = gameplay_state_render;

  level_init(&state->level);
  state->orientation = NORTH;
  float cell_width = (float)GAME_WIDTH / state->level.board.width;
  float cell_height = (float)GAME_HEIGHT / state->level.board.height;
  state->cell_size = cell_width < cell_height ? cell_width : cell_height;
  state->board_offset_x =
      (GAME_WIDTH - state->cell_size * state->level.board.width) / 2.0f;
  state->board_offset_y =
      (GAME_HEIGHT - state->cell_size * state->level.board.height) / 2.0f;

  (void)game;
  return &state->base;
}
