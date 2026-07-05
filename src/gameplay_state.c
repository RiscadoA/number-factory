#include "gameplay_state.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "input.h"
#include "level.h"
#include "pipe.h"

enum {
  NUMBER_GLYPH_COUNT = 11,
  NUMBER_MINUS_GLYPH = 10,
};

typedef struct {
  SDL_Texture *textures[NUMBER_GLYPH_COUNT];
  float widths[NUMBER_GLYPH_COUNT];
  float heights[NUMBER_GLYPH_COUNT];
} NumberRenderer;

typedef struct {
  GameState base;
  Level level;
  NumberRenderer number_renderer;
  float cell_size;
  float board_offset_x;
  float board_offset_y;
  Vector2i hovered_cell;
  int has_hovered_cell;
  Orientation orientation;
  Uint64 last_ticks;
} GameplayState;

static void number_renderer_free(NumberRenderer *renderer) {
  for (int i = 0; i < NUMBER_GLYPH_COUNT; i++) {
    if (renderer->textures[i]) {
      SDL_DestroyTexture(renderer->textures[i]);
      renderer->textures[i] = NULL;
    }
  }
}

static int number_renderer_init(NumberRenderer *renderer, NumberFactory *game) {
  SDL_Color color = {.r = 0, .g = 0, .b = 0, .a = 255};

  for (int i = 0; i < NUMBER_GLYPH_COUNT; i++) {
    char text[2] = {i == NUMBER_MINUS_GLYPH ? '-' : (char)('0' + i), '\0'};
    SDL_Surface *surface = TTF_RenderText_Blended(game->font, text, 0, color);
    if (!surface) {
      SDL_Log("Failed to render number glyph '%c': %s", text[0],
              SDL_GetError());
      number_renderer_free(renderer);
      return 0;
    }

    SDL_Texture *texture =
        SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture || !SDL_GetTextureSize(texture, &renderer->widths[i],
                                        &renderer->heights[i])) {
      SDL_Log("Failed to create number glyph '%c': %s", text[0],
              SDL_GetError());
      if (texture) SDL_DestroyTexture(texture);
      number_renderer_free(renderer);
      return 0;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    renderer->textures[i] = texture;
  }

  return 1;
}

static void number_renderer_draw(NumberRenderer *renderer,
                                 SDL_Renderer *sdl_renderer, int value,
                                 SDL_FRect bounds) {
  char text[16];
  int length = snprintf(text, sizeof(text), "%d", value);
  if (length <= 0 || length >= (int)sizeof(text)) return;

  float unscaled_width = 0.0f;
  float unscaled_height = 0.0f;
  for (int i = 0; i < length; i++) {
    int glyph = text[i] == '-' ? NUMBER_MINUS_GLYPH : text[i] - '0';
    unscaled_width += renderer->widths[glyph];
    unscaled_height = MAX(unscaled_height, renderer->heights[glyph]);
  }

  float max_width = bounds.w * 0.75f;
  float max_height = bounds.h * 0.65f;
  float width_scale = max_width / unscaled_width;
  float height_scale = max_height / unscaled_height;
  float scale = MIN(width_scale, height_scale);
  float x = bounds.x + (bounds.w - unscaled_width * scale) / 2.0f;

  for (int i = 0; i < length; i++) {
    int glyph = text[i] == '-' ? NUMBER_MINUS_GLYPH : text[i] - '0';
    float width = renderer->widths[glyph] * scale;
    float height = renderer->heights[glyph] * scale;
    SDL_FRect destination = {
        .x = x,
        .y = bounds.y + (bounds.h - height) / 2.0f,
        .w = width,
        .h = height,
    };
    SDL_RenderTexture(sdl_renderer, renderer->textures[glyph], NULL,
                      &destination);
    x += width;
  }
}

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

static void draw_item(NumberFactory *game, GameplayState *state, Vector2f pos,
                      int value) {
  float size = state->cell_size * 0.25f;
  float x = state->board_offset_x + pos.x * state->cell_size - size / 2.0f;
  float y = state->board_offset_y + pos.y * state->cell_size - size / 2.0f;
  SDL_FRect rect = {x, y, size, size};
  SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(game->renderer, &rect);
  SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
  SDL_RenderRect(game->renderer, &rect);
  number_renderer_draw(&state->number_renderer, game->renderer, value, rect);
}

static void gameplay_state_destroy(GameState *base, NumberFactory *game) {
  (void)game;
  GameplayState *state = (GameplayState *)base;
  number_renderer_free(&state->number_renderer);
  level_free(&state->level);
  free(state);
}

static void rotate_orientation(GameplayState *state, int steps) {
  int orientation = state->orientation + steps % ORIENTATION_COUNT;
  if (orientation < 0) orientation += ORIENTATION_COUNT;
  state->orientation = (Orientation)(orientation % ORIENTATION_COUNT);
}

static SDL_AppResult gameplay_state_event(GameState *base, NumberFactory *game,
                                          SDL_Event *event) {
  (void)game;
  GameplayState *state = (GameplayState *)base;

  if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_R &&
      !event->key.repeat) {
    rotate_orientation(state, 1);
  }

  if (event->type == SDL_EVENT_MOUSE_WHEEL) {
    int steps = event->wheel.integer_y;
    if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) steps = -steps;
    rotate_orientation(state, steps);
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
  (void)game;
  GameplayState *state = (GameplayState *)base;

  Uint64 now = SDL_GetTicks();
  float dt = (now - state->last_ticks) / 1000.0f;
  state->last_ticks = now;

  level_update(&state->level, dt);
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
      SDL_FRect rect = {
          .x = state->board_offset_x + x * state->cell_size,
          .y = state->board_offset_y + y * state->cell_size,
          .w = state->cell_size,
          .h = state->cell_size,
      };
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
      case ENTITY_INPUT: {
        Input *input = &entity->input;
        SDL_SetRenderDrawColor(game->renderer, 80, 160, 220, 255);
        SDL_RenderFillRect(game->renderer, &rect);
        SDL_SetRenderDrawColor(game->renderer, 40, 80, 120, 255);
        SDL_RenderRect(game->renderer, &rect);
        SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
        draw_orientation_arrow(game->renderer, rect, input->orientation);
        break;
      }
      }
    }
  }

  // Second pass: render items on top of all entity geometry
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

        for (int i = 0; i < pipe->items.size; i++) {
          PipeItem item = DEQUE_AT(pipe->items, i);
          Vector2f pos = pipe_item_position(pipe, i);
          draw_item(game, state, pos, item.value);
        }
        break;
      }
      case ENTITY_INPUT: {
        Input *input = &entity->input;
        if (input->progress > 0.0f) {
          Vector2f pos = input_item_position(input);
          draw_item(game, state, pos, input->value);
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
  if (!number_renderer_init(&state->number_renderer, game)) {
    gameplay_state_destroy(&state->base, game);
    return NULL;
  }
  state->orientation = NORTH;
  state->last_ticks = SDL_GetTicks();
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
