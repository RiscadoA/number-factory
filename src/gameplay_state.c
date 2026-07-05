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
  HUD_HEIGHT = 44,
};

typedef enum {
  PLACEMENT_PIPE,
  PLACEMENT_FILTER,
} PlacementMode;

typedef struct {
  SDL_Texture *textures[NUMBER_GLYPH_COUNT];
  float widths[NUMBER_GLYPH_COUNT];
  float heights[NUMBER_GLYPH_COUNT];
} NumberRenderer;

typedef struct {
  SDL_Texture *texture;
  float width;
  float height;
} TextTexture;

typedef struct {
  TTF_Font *font;
  TextTexture pipe_label;
  TextTexture filter_label;
  TextTexture rotate_label;
} GameplayHud;

typedef struct {
  GameState base;
  Level level;
  NumberRenderer number_renderer;
  GameplayHud hud;
  PlacementMode placement_mode;
  float cell_size;
  float board_offset_x;
  float board_offset_y;
  int viewport_width;
  int viewport_height;
  Vector2i hovered_cell;
  int has_hovered_cell;
  Orientation orientation;
  Uint64 last_ticks;
} GameplayState;

static void gameplay_state_layout(GameplayState *state, int width, int height) {
  state->viewport_width = width;
  state->viewport_height = height;

  float cell_width = (float)width / state->level.board.width;
  float cell_height = (float)(height - HUD_HEIGHT) / state->level.board.height;
  state->cell_size = MIN(cell_width, cell_height);
  state->board_offset_x =
      (width - state->cell_size * state->level.board.width) / 2.0f;
  state->board_offset_y =
      HUD_HEIGHT +
      (height - HUD_HEIGHT - state->cell_size * state->level.board.height) /
          2.0f;
}

static void text_texture_free(TextTexture *text) {
  if (text->texture) SDL_DestroyTexture(text->texture);
  text->texture = NULL;
  text->width = 0.0f;
  text->height = 0.0f;
}

static int text_texture_init(TextTexture *text, NumberFactory *game,
                             TTF_Font *font, const char *value) {
  SDL_Color color = {.r = 40, .g = 30, .b = 40, .a = 255};
  SDL_Surface *surface = TTF_RenderText_Blended(font, value, 0, color);
  if (!surface) return 0;

  text->texture = SDL_CreateTextureFromSurface(game->renderer, surface);
  SDL_DestroySurface(surface);
  if (!text->texture ||
      !SDL_GetTextureSize(text->texture, &text->width, &text->height)) {
    text_texture_free(text);
    return 0;
  }

  SDL_SetTextureScaleMode(text->texture, SDL_SCALEMODE_NEAREST);
  return 1;
}

static void gameplay_hud_free(GameplayHud *hud) {
  text_texture_free(&hud->pipe_label);
  text_texture_free(&hud->filter_label);
  text_texture_free(&hud->rotate_label);
  if (hud->font) TTF_CloseFont(hud->font);
  hud->font = NULL;
}

static int gameplay_hud_init(GameplayHud *hud, NumberFactory *game) {
  hud->font = TTF_OpenFont("assets/fonts/PressStart2P-Regular.ttf", 14.0f);
  if (!hud->font ||
      !text_texture_init(&hud->pipe_label, game, hud->font, "1 PIPE") ||
      !text_texture_init(&hud->filter_label, game, hud->font, "2 FILTER") ||
      !text_texture_init(&hud->rotate_label, game, hud->font,
                         "R / WHEEL ROTATE")) {
    SDL_Log("Failed to create gameplay HUD: %s", SDL_GetError());
    gameplay_hud_free(hud);
    return 0;
  }
  return 1;
}

static void gameplay_hud_draw_button(NumberFactory *game, TextTexture *label,
                                     SDL_FRect bounds, int selected) {
  if (selected) {
    SDL_SetRenderDrawColor(game->renderer, 255, 220, 100, 255);
  } else {
    SDL_SetRenderDrawColor(game->renderer, 180, 155, 165, 255);
  }
  SDL_RenderFillRect(game->renderer, &bounds);
  SDL_SetRenderDrawColor(game->renderer, 40, 30, 40, 255);
  SDL_RenderRect(game->renderer, &bounds);

  float scale = MIN(1.0f, MIN((bounds.w - 20.0f) / label->width,
                              (bounds.h - 14.0f) / label->height));
  SDL_FRect destination = {
      .x = bounds.x + (bounds.w - label->width * scale) / 2.0f,
      .y = bounds.y + (bounds.h - label->height * scale) / 2.0f,
      .w = label->width * scale,
      .h = label->height * scale,
  };
  SDL_RenderTexture(game->renderer, label->texture, NULL, &destination);
}

static void gameplay_hud_render(GameplayState *state, NumberFactory *game) {
  SDL_SetRenderDrawColor(game->renderer, 40, 30, 40, 255);
  SDL_FRect background = {0.0f, 0.0f, state->viewport_width, HUD_HEIGHT};
  SDL_RenderFillRect(game->renderer, &background);

  SDL_FRect pipe_button = {8.0f, 6.0f, 110.0f, 32.0f};
  SDL_FRect filter_button = {126.0f, 6.0f, 140.0f, 32.0f};
  SDL_FRect rotate_hint = {274.0f, 6.0f, 250.0f, 32.0f};
  gameplay_hud_draw_button(game, &state->hud.pipe_label, pipe_button,
                           state->placement_mode == PLACEMENT_PIPE);
  gameplay_hud_draw_button(game, &state->hud.filter_label, filter_button,
                           state->placement_mode == PLACEMENT_FILTER);
  gameplay_hud_draw_button(game, &state->hud.rotate_label, rotate_hint, 0);
}

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

static void draw_pipe_half_segment(SDL_Renderer *renderer, float center_x,
                                   float center_y, Vector2i direction,
                                   float length, float width) {
  SDL_FRect segment;
  if (direction.x < 0) {
    segment =
        (SDL_FRect){center_x - length, center_y - width / 2.0f, length, width};
  } else if (direction.x > 0) {
    segment = (SDL_FRect){center_x, center_y - width / 2.0f, length, width};
  } else if (direction.y < 0) {
    segment =
        (SDL_FRect){center_x - width / 2.0f, center_y - length, width, length};
  } else {
    segment = (SDL_FRect){center_x - width / 2.0f, center_y, width, length};
  }
  SDL_RenderFillRect(renderer, &segment);
}

static void draw_pipe_layer(SDL_Renderer *renderer, GameplayState *state,
                            Pipe *pipe, float width) {
  float half_cell = state->cell_size * 0.5f;

  for (int i = 0; i < pipe->cells.size; i++) {
    PipeCell cell = DEQUE_AT(pipe->cells, i);
    Vector2i output = orientation_vector(cell.orientation);
    Vector2i input;
    if (i == 0) {
      input = orientation_vector(orientation_opposite(cell.orientation));
    } else {
      PipeCell previous = DEQUE_AT(pipe->cells, i - 1);
      Vector2i previous_output = orientation_vector(previous.orientation);
      input = (Vector2i){-previous_output.x, -previous_output.y};
    }

    float center_x =
        state->board_offset_x + (cell.pos.x + 0.5f) * state->cell_size;
    float center_y =
        state->board_offset_y + (cell.pos.y + 0.5f) * state->cell_size;
    draw_pipe_half_segment(renderer, center_x, center_y, input, half_cell,
                           width);
    draw_pipe_half_segment(renderer, center_x, center_y, output, half_cell,
                           width);

    SDL_FRect joint = {
        center_x - width / 2.0f,
        center_y - width / 2.0f,
        width,
        width,
    };
    SDL_RenderFillRect(renderer, &joint);
  }
}

static void draw_pipe_direction_markers(SDL_Renderer *renderer,
                                        GameplayState *state, Pipe *pipe) {
  float block_size = state->cell_size * 0.055f;
  float forward = state->cell_size * 0.1f;
  float sideways = state->cell_size * 0.075f;

  for (int i = 0; i < pipe->cells.size; i++) {
    PipeCell cell = DEQUE_AT(pipe->cells, i);
    Vector2i direction = orientation_vector(cell.orientation);
    Vector2i perpendicular = {-direction.y, direction.x};
    float center_x =
        state->board_offset_x + (cell.pos.x + 0.5f) * state->cell_size;
    float center_y =
        state->board_offset_y + (cell.pos.y + 0.5f) * state->cell_size;

    Vector2f offsets[] = {
        {direction.x * forward, direction.y * forward},
        {-direction.x * forward * 0.35f + perpendicular.x * sideways,
         -direction.y * forward * 0.35f + perpendicular.y * sideways},
        {-direction.x * forward * 0.35f - perpendicular.x * sideways,
         -direction.y * forward * 0.35f - perpendicular.y * sideways},
    };
    for (int j = 0; j < 3; j++) {
      SDL_FRect block = {
          center_x + offsets[j].x - block_size / 2.0f,
          center_y + offsets[j].y - block_size / 2.0f,
          block_size,
          block_size,
      };
      SDL_RenderFillRect(renderer, &block);
    }
  }
}

static void draw_pipe(NumberFactory *game, GameplayState *state, Pipe *pipe) {
  float casing_width = state->cell_size * 0.38f;
  float channel_width = state->cell_size * 0.24f;

  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  draw_pipe_layer(game->renderer, state, pipe, casing_width);

  SDL_SetRenderDrawColor(game->renderer, 185, 175, 165, 255);
  draw_pipe_layer(game->renderer, state, pipe, channel_width);

  SDL_SetRenderDrawColor(game->renderer, 255, 245, 235, 210);
  draw_pipe_direction_markers(game->renderer, state, pipe);
}

static void gameplay_state_destroy(GameState *base, NumberFactory *game) {
  (void)game;
  GameplayState *state = (GameplayState *)base;
  gameplay_hud_free(&state->hud);
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

  if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
    switch (event->key.key) {
    case SDLK_1:
    case SDLK_KP_1:
      state->placement_mode = PLACEMENT_PIPE;
      break;
    case SDLK_2:
    case SDLK_KP_2:
      state->placement_mode = PLACEMENT_FILTER;
      break;
    case SDLK_R:
      rotate_orientation(state, 1);
      break;
    default:
      break;
    }
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
        if (state->placement_mode == PLACEMENT_PIPE) {
          level_place_pipe(&state->level, cell, state->orientation);
        } else if (BOARD_AT(&state->level.board, cell.x, cell.y) ==
                   ENTITY_NONE) {
          SDL_Log("Placing filter at (%d, %d), orientation %s", cell.x, cell.y,
                  orientation_to_string(state->orientation));
        }
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
  int width;
  int height;
  SDL_GetWindowSize(game->window, &width, &height);
  if (width != state->viewport_width || height != state->viewport_height) {
    gameplay_state_layout(state, width, height);
  }

  SDL_SetRenderDrawColor(game->renderer, 220, 190, 200, 255);
  SDL_RenderClear(game->renderer);
  gameplay_hud_render(state, game);

  SDL_FRect board_bounds = {
      .x = state->board_offset_x,
      .y = state->board_offset_y,
      .w = state->cell_size * state->level.board.width,
      .h = state->cell_size * state->level.board.height,
  };
  SDL_SetRenderDrawColor(game->renderer, 200, 165, 180, 255);
  SDL_RenderFillRect(game->renderer, &board_bounds);
  SDL_SetRenderDrawColor(game->renderer, 120, 90, 105, 255);
  SDL_RenderRect(game->renderer, &board_bounds);

  for (int x = 0; x < state->level.board.width; x++) {
    for (int y = 0; y < state->level.board.height; y++) {
      EntityId id = BOARD_AT(&state->level.board, x, y);
      if (id == ENTITY_NONE) continue;
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
        draw_pipe(game, state, pipe);
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
      if (id == ENTITY_NONE) continue;
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
    if (state->placement_mode == PLACEMENT_FILTER) {
      SDL_SetRenderDrawColor(game->renderer, 80, 255, 255, 48);
    } else {
      SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 48);
    }
    SDL_RenderFillRect(game->renderer, &preview);
    if (state->placement_mode == PLACEMENT_FILTER) {
      SDL_SetRenderDrawColor(game->renderer, 80, 255, 255, 192);
    } else {
      SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 192);
    }
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
  if (!gameplay_hud_init(&state->hud, game)) {
    gameplay_state_destroy(&state->base, game);
    return NULL;
  }
  state->placement_mode = PLACEMENT_PIPE;
  state->orientation = NORTH;
  state->last_ticks = SDL_GetTicks();
  int width;
  int height;
  SDL_GetWindowSize(game->window, &width, &height);
  gameplay_state_layout(state, width, height);

  (void)game;
  return &state->base;
}
