#include "gameplay_state.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "input.h"
#include "level.h"
#include "pause_state.h"
#include "pipe.h"
#include "splitter.h"

enum {
  NUMBER_GLYPH_COUNT = 11,
  NUMBER_MINUS_GLYPH = 10,
  HUD_HEIGHT = 44,
};

typedef enum {
  PLACEMENT_PIPE,
  PLACEMENT_SPLITTER,
  PLACEMENT_ADDITION,
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
  TextTexture splitter_label;
  TextTexture addition_label;
  TextTexture rotate_label;
  TextTexture pause_label;
  TextTexture health_label;
  TextTexture health_value;
  int rendered_health;
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

static void number_renderer_draw(NumberRenderer *renderer,
                                 SDL_Renderer *sdl_renderer, int value,
                                 SDL_FRect bounds);

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
  SDL_Surface *surface = TTF_RenderText_Solid(font, value, 0, color);
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
  text_texture_free(&hud->splitter_label);
  text_texture_free(&hud->addition_label);
  text_texture_free(&hud->rotate_label);
  text_texture_free(&hud->pause_label);
  text_texture_free(&hud->health_label);
  text_texture_free(&hud->health_value);
  if (hud->font) TTF_CloseFont(hud->font);
  hud->font = NULL;
}

static int gameplay_hud_init(GameplayHud *hud, NumberFactory *game) {
  hud->font = TTF_OpenFont("assets/fonts/PressStart2P-Regular.ttf", 10.0f);
  if (!hud->font ||
      !text_texture_init(&hud->pipe_label, game, hud->font, "1 PIPE") ||
      !text_texture_init(&hud->splitter_label, game, hud->font, "2 SPLITTER") ||
      !text_texture_init(&hud->addition_label, game, hud->font, "3 ADDITION") ||
      !text_texture_init(&hud->rotate_label, game, hud->font,
                         "R/WHEEL ROTATE") ||
      !text_texture_init(&hud->pause_label, game, hud->font, "ESC PAUSE") ||
      !text_texture_init(&hud->health_label, game, hud->font, "HP") ||
      !text_texture_init(&hud->health_value, game, hud->font, "100")) {
    SDL_Log("Failed to create gameplay HUD: %s", SDL_GetError());
    gameplay_hud_free(hud);
    return 0;
  }
  hud->rendered_health = 100;
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

  SDL_FRect destination = {
      .x = (float)(int)(bounds.x + (bounds.w - label->width) / 2.0f + 0.5f),
      .y = (float)(int)(bounds.y + (bounds.h - label->height) / 2.0f + 0.5f),
      .w = label->width,
      .h = label->height,
  };
  SDL_RenderTexture(game->renderer, label->texture, NULL, &destination);
}

static void gameplay_hud_render(GameplayState *state, NumberFactory *game) {
  if (state->hud.rendered_health != state->level.health) {
    char value[16];
    snprintf(value, sizeof(value), "%d", state->level.health);
    TextTexture texture = {0};
    if (text_texture_init(&texture, game, state->hud.font, value)) {
      text_texture_free(&state->hud.health_value);
      state->hud.health_value = texture;
      state->hud.rendered_health = state->level.health;
    }
  }

  SDL_SetRenderDrawColor(game->renderer, 40, 30, 40, 255);
  SDL_FRect background = {0.0f, 0.0f, state->viewport_width, HUD_HEIGHT};
  SDL_RenderFillRect(game->renderer, &background);

  SDL_FRect pipe_button = {8.0f, 6.0f, 72.0f, 32.0f};
  SDL_FRect filter_button = {86.0f, 6.0f, 92.0f, 32.0f};
  SDL_FRect addition_button = {184.0f, 6.0f, 106.0f, 32.0f};
  SDL_FRect rotate_hint = {296.0f, 6.0f, 152.0f, 32.0f};
  SDL_FRect pause_hint = {454.0f, 6.0f, 102.0f, 32.0f};
  gameplay_hud_draw_button(game, &state->hud.pipe_label, pipe_button,
                           state->placement_mode == PLACEMENT_PIPE);
  gameplay_hud_draw_button(game, &state->hud.splitter_label, filter_button,
                           state->placement_mode == PLACEMENT_SPLITTER);
  gameplay_hud_draw_button(game, &state->hud.addition_label, addition_button,
                           state->placement_mode == PLACEMENT_ADDITION);
  gameplay_hud_draw_button(game, &state->hud.rotate_label, rotate_hint, 0);
  gameplay_hud_draw_button(game, &state->hud.pause_label, pause_hint, 0);

  SDL_FRect health_bar = {(float)state->viewport_width - 88.0f, 6.0f, 80.0f,
                          32.0f};
  SDL_SetRenderDrawColor(game->renderer, 75, 55, 65, 255);
  SDL_RenderFillRect(game->renderer, &health_bar);
  float health_ratio = (float)state->level.health / state->level.max_health;
  if (health_ratio > 0.5f) {
    SDL_SetRenderDrawColor(game->renderer, 105, 205, 120, 255);
  } else if (health_ratio > 0.25f) {
    SDL_SetRenderDrawColor(game->renderer, 235, 195, 80, 255);
  } else {
    SDL_SetRenderDrawColor(game->renderer, 225, 85, 85, 255);
  }
  SDL_FRect health_fill = {health_bar.x + 2.0f, health_bar.y + 2.0f,
                           (health_bar.w - 4.0f) * health_ratio,
                           health_bar.h - 4.0f};
  SDL_RenderFillRect(game->renderer, &health_fill);
  SDL_SetRenderDrawColor(game->renderer, 40, 30, 40, 255);
  SDL_RenderRect(game->renderer, &health_bar);

  TextTexture *health_label = &state->hud.health_label;
  SDL_FRect label_destination = {
      .x = health_bar.x + 5.0f,
      .y = (float)(int)(health_bar.y +
                        (health_bar.h - health_label->height) / 2.0f + 0.5f),
      .w = health_label->width,
      .h = health_label->height,
  };
  SDL_RenderTexture(game->renderer, health_label->texture, NULL,
                    &label_destination);
  TextTexture *health_value = &state->hud.health_value;
  SDL_FRect value_destination = {
      .x = (float)(int)(health_bar.x + 29.0f +
                        (health_bar.w - 34.0f - health_value->width) / 2.0f +
                        0.5f),
      .y = (float)(int)(health_bar.y +
                        (health_bar.h - health_value->height) / 2.0f + 0.5f),
      .w = health_value->width,
      .h = health_value->height,
  };
  SDL_RenderTexture(game->renderer, health_value->texture, NULL,
                    &value_destination);
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

static void draw_pipe_direction_arrow(SDL_Renderer *renderer, float center_x,
                                      float center_y, Orientation orientation,
                                      float cell_size) {
  Vector2i direction = orientation_vector(orientation);
  Vector2i perpendicular = {.x = -direction.y, .y = direction.x};
  float block_size = cell_size * 0.06f;
  float cx = center_x - block_size / 2.0f;
  float cy = center_y - block_size / 2.0f;

  Vector2i blocks[] = {
      {.x = 1, .y = 0},   // tip
      {.x = 0, .y = -1}, {.x = 0, .y = 0}, {.x = 0, .y = 1},
      {.x = -1, .y = -1}, {.x = -1, .y = 0}, {.x = -1, .y = 1},
  };

  for (int i = 0; i < 7; i++) {
    int x = blocks[i].x * direction.x + blocks[i].y * perpendicular.x;
    int y = blocks[i].x * direction.y + blocks[i].y * perpendicular.y;
    SDL_FRect block = {
        .x = cx + x * block_size,
        .y = cy + y * block_size,
        .w = block_size,
        .h = block_size,
    };
    SDL_RenderFillRect(renderer, &block);
  }
}

static void draw_item_scaled(NumberFactory *game, GameplayState *state,
                             Vector2f pos, int value, float scale) {
  float size = state->cell_size * 0.25f * scale;
  float x = state->board_offset_x + pos.x * state->cell_size - size / 2.0f;
  float y = state->board_offset_y + pos.y * state->cell_size - size / 2.0f;
  SDL_FRect rect = {x, y, size, size};
  SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(game->renderer, &rect);
  SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
  SDL_RenderRect(game->renderer, &rect);
  number_renderer_draw(&state->number_renderer, game->renderer, value, rect);
}

static void draw_item(NumberFactory *game, GameplayState *state, Vector2f pos,
                      int value) {
  draw_item_scaled(game, state, pos, value, 1.0f);
}

typedef struct {
  NumberFactory *game;
  GameplayState *state;
} DrawItemCallbackData;

static void draw_item_callback(void *user, int value, Vector2f position) {
  DrawItemCallbackData *data = user;
  draw_item(data->game, data->state, position, value);
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

static void draw_input(NumberFactory *game, SDL_FRect cell, Input *input) {
  float center_x = cell.x + cell.w / 2.0f;
  float center_y = cell.y + cell.h / 2.0f;
  Vector2i direction = orientation_vector(input->orientation);

  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  draw_pipe_half_segment(game->renderer, center_x, center_y, direction,
                         cell.w * 0.5f, cell.w * 0.38f);
  SDL_SetRenderDrawColor(game->renderer, 90, 175, 220, 255);
  draw_pipe_half_segment(game->renderer, center_x, center_y, direction,
                         cell.w * 0.5f, cell.w * 0.24f);

  float casing_padding = cell.w * 0.14f;
  SDL_FRect casing = {
      .x = cell.x + casing_padding,
      .y = cell.y + casing_padding,
      .w = cell.w - casing_padding * 2.0f,
      .h = cell.h - casing_padding * 2.0f,
  };
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  SDL_RenderFillRect(game->renderer, &casing);

  float panel_padding = cell.w * 0.07f;
  SDL_FRect panel = {
      .x = casing.x + panel_padding,
      .y = casing.y + panel_padding,
      .w = casing.w - panel_padding * 2.0f,
      .h = casing.h - panel_padding * 2.0f,
  };
  SDL_SetRenderDrawColor(game->renderer, 90, 175, 220, 255);
  SDL_RenderFillRect(game->renderer, &panel);

  SDL_SetRenderDrawColor(game->renderer, 145, 215, 240, 255);
  SDL_FRect highlight = {panel.x, panel.y, panel.w, cell.h * 0.055f};
  SDL_RenderFillRect(game->renderer, &highlight);

  float rivet_size = cell.w * 0.045f;
  float rivet_inset = cell.w * 0.035f;
  SDL_SetRenderDrawColor(game->renderer, 220, 235, 240, 255);
  SDL_FRect rivets[] = {
      {panel.x + rivet_inset, panel.y + rivet_inset, rivet_size, rivet_size},
      {panel.x + panel.w - rivet_inset - rivet_size, panel.y + rivet_inset,
       rivet_size, rivet_size},
      {panel.x + rivet_inset, panel.y + panel.h - rivet_inset - rivet_size,
       rivet_size, rivet_size},
      {panel.x + panel.w - rivet_inset - rivet_size,
       panel.y + panel.h - rivet_inset - rivet_size, rivet_size, rivet_size},
  };
  for (int i = 0; i < 4; i++) {
    SDL_RenderFillRect(game->renderer, &rivets[i]);
  }

  SDL_SetRenderDrawColor(game->renderer, 40, 75, 95, 255);
  draw_orientation_arrow(game->renderer, panel, input->orientation);
}

static void draw_output(NumberFactory *game, GameplayState *state,
                        SDL_FRect cell, Output *output) {
  float center_x = cell.x + cell.w / 2.0f;
  float center_y = cell.y + cell.h / 2.0f;
  Vector2i travel_direction = orientation_vector(output->input_orientation);
  Vector2i inlet_direction = {-travel_direction.x, -travel_direction.y};

  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  draw_pipe_half_segment(game->renderer, center_x, center_y, inlet_direction,
                         cell.w * 0.5f, cell.w * 0.38f);
  SDL_SetRenderDrawColor(game->renderer, 185, 175, 165, 255);
  draw_pipe_half_segment(game->renderer, center_x, center_y, inlet_direction,
                         cell.w * 0.5f, cell.w * 0.24f);

  float casing_padding = cell.w * 0.14f;
  SDL_FRect casing = {
      .x = cell.x + casing_padding,
      .y = cell.y + casing_padding,
      .w = cell.w - casing_padding * 2.0f,
      .h = cell.h - casing_padding * 2.0f,
  };
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  SDL_RenderFillRect(game->renderer, &casing);

  float panel_padding = cell.w * 0.07f;
  SDL_FRect panel = {
      .x = casing.x + panel_padding,
      .y = casing.y + panel_padding,
      .w = casing.w - panel_padding * 2.0f,
      .h = casing.h - panel_padding * 2.0f,
  };
  SDL_SetRenderDrawColor(game->renderer, 205, 105, 95, 255);
  SDL_RenderFillRect(game->renderer, &panel);

  SDL_SetRenderDrawColor(game->renderer, 235, 150, 125, 255);
  SDL_FRect highlight = {panel.x, panel.y, panel.w, cell.h * 0.055f};
  SDL_RenderFillRect(game->renderer, &highlight);

  float badge_size = cell.w * 0.3f;
  SDL_FRect target_badge = {
      .x = center_x - badge_size / 2.0f,
      .y = center_y - badge_size / 2.0f,
      .w = badge_size,
      .h = badge_size,
  };
  number_renderer_draw(&state->number_renderer, game->renderer,
                       output->target_value, target_badge);

  float marker_size = cell.w * 0.055f;
  float marker_distance = cell.w * 0.22f;
  SDL_FRect direction_marker = {
      .x = center_x - travel_direction.x * marker_distance - marker_size / 2.0f,
      .y = center_y - travel_direction.y * marker_distance - marker_size / 2.0f,
      .w = marker_size,
      .h = marker_size,
  };
  SDL_SetRenderDrawColor(game->renderer, 255, 225, 205, 255);
  SDL_RenderFillRect(game->renderer, &direction_marker);
}

static void draw_splitter(NumberFactory *game, SDL_FRect cell,
                          Splitter *splitter) {
  float center_x = cell.x + cell.w / 2.0f;
  float center_y = cell.y + cell.h / 2.0f;
  float half_cell = cell.w * 0.5f;
  float casing_width = cell.w * 0.38f;
  float channel_width = cell.w * 0.24f;

  // Draw dark casing arms in all four directions
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  for (Orientation o = ORIENTATION_ZERO; o < ORIENTATION_COUNT; o++) {
    Vector2i dir = orientation_vector(o);
    draw_pipe_half_segment(game->renderer, center_x, center_y, dir, half_cell,
                           casing_width);
  }

  // Draw teal channel arms in all four directions
  SDL_SetRenderDrawColor(game->renderer, 80, 220, 220, 255);
  for (Orientation o = ORIENTATION_ZERO; o < ORIENTATION_COUNT; o++) {
    Vector2i dir = orientation_vector(o);
    draw_pipe_half_segment(game->renderer, center_x, center_y, dir, half_cell,
                           channel_width);
  }

  // Central casing block
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  SDL_FRect joint = {
      center_x - casing_width / 2.0f,
      center_y - casing_width / 2.0f,
      casing_width,
      casing_width,
  };
  SDL_RenderFillRect(game->renderer, &joint);

  // Inner panel
  SDL_FRect panel = {
      center_x - channel_width / 2.0f,
      center_y - channel_width / 2.0f,
      channel_width,
      channel_width,
  };
  SDL_SetRenderDrawColor(game->renderer, 60, 200, 200, 255);
  SDL_RenderFillRect(game->renderer, &panel);

  // Highlight strip
  SDL_SetRenderDrawColor(game->renderer, 120, 240, 240, 255);
  SDL_FRect highlight = {panel.x, panel.y, panel.w, cell.h * 0.055f};
  SDL_RenderFillRect(game->renderer, &highlight);

  // Indicate the input orientation
  SDL_SetRenderDrawColor(game->renderer, 20, 60, 60, 255);
  draw_orientation_arrow(game->renderer, cell, splitter->orientation);
}

static int entity_has_output_to(Entity *entity, Vector2i entity_pos,
                                Vector2i target_pos) {
  Vector2i diff = vector_subtract(target_pos, entity_pos);
  Orientation dir = vector_orientation(diff);
  if (dir < 0)
    return 0;

  switch (entity->type) {
  case ENTITY_PIPE: {
    Orientation o = pipe_orientation(&entity->pipe, entity_pos);
    return o == dir;
  }
  case ENTITY_INPUT:
    return entity->input.orientation == dir;
  case ENTITY_SPLITTER:
    return dir != orientation_opposite(entity->splitter.orientation);
  case ENTITY_ADDITION: {
    Vector2i secondary = vector_add(entity->addition.position,
                                    orientation_vector(entity->addition.orientation));
    if (vector_equal(entity_pos, secondary)) {
      Vector2i output_pos =
          vector_add(secondary, orientation_vector(entity->addition.orientation));
      if (vector_equal(target_pos, output_pos)) {
        return entity->addition.orientation == dir;
      }
    }
    return 0;
  }
  case ENTITY_OUTPUT:
    return 0;
  default:
    return 0;
  }
}

static void draw_pipe_cell(NumberFactory *game, GameplayState *state,
                           Vector2i pos, Pipe *pipe) {
  float casing_width = state->cell_size * 0.38f;
  float channel_width = state->cell_size * 0.24f;
  float half_cell = state->cell_size * 0.5f;

  Orientation orientation = pipe_orientation(pipe, pos);
  if (orientation < 0)
    return;

  float center_x =
      state->board_offset_x + (pos.x + 0.5f) * state->cell_size;
  float center_y =
      state->board_offset_y + (pos.y + 0.5f) * state->cell_size;

  int active[ORIENTATION_COUNT] = {0};
  active[orientation] = 1;
  for (Orientation d = ORIENTATION_ZERO; d < ORIENTATION_COUNT; d++) {
    if (d == orientation)
      continue;
    Vector2i neighbor_pos = vector_add(pos, orientation_vector(d));
    if (BOARD_VALID(&state->level.board, neighbor_pos.x, neighbor_pos.y)) {
      EntityId nid =
          BOARD_AT(&state->level.board, neighbor_pos.x, neighbor_pos.y);
      if (nid != ENTITY_NONE) {
        Entity *neighbor = ENTITY_AT(&state->level.entity_pool, nid);
        if (entity_has_output_to(neighbor, neighbor_pos, pos)) {
          active[d] = 1;
        }
      }
    }
  }

  // Draw casing arms
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  for (Orientation d = ORIENTATION_ZERO; d < ORIENTATION_COUNT; d++) {
    if (active[d]) {
      draw_pipe_half_segment(game->renderer, center_x, center_y,
                             orientation_vector(d), half_cell, casing_width);
    }
  }

  // Draw channel arms
  SDL_SetRenderDrawColor(game->renderer, 185, 175, 165, 255);
  for (Orientation d = ORIENTATION_ZERO; d < ORIENTATION_COUNT; d++) {
    if (active[d]) {
      draw_pipe_half_segment(game->renderer, center_x, center_y,
                             orientation_vector(d), half_cell, channel_width);
    }
  }

  // At L-corners the casing arms overlap in the inner corner, creating a dark
  // joint. Fill that overlap with channel color so the bend looks clean.
  struct {
    Orientation a, b;
    float x, y;
  } inner[4] = {
      {NORTH, EAST,  center_x, center_y - casing_width / 2.0f},
      {EAST,  SOUTH, center_x, center_y},
      {SOUTH, WEST,  center_x - casing_width / 2.0f, center_y},
      {WEST,  NORTH, center_x - casing_width / 2.0f, center_y - casing_width / 2.0f},
  };
  float inner_size = casing_width / 2.0f;
  SDL_SetRenderDrawColor(game->renderer, 185, 175, 165, 255);
  for (int i = 0; i < 4; i++) {
    if (active[inner[i].a] && active[inner[i].b]) {
      SDL_FRect fill = {inner[i].x, inner[i].y, inner_size, inner_size};
      SDL_RenderFillRect(game->renderer, &fill);
    }
  }

  // At L-corners the half-segments are thin strips from center to two edges.
  // They leave the diagonally-opposite cell corner uncovered. Patch it.
  struct {
    Orientation a, b;
    float x, y;
  } corners[4] = {
      // NORTH+EAST → top-right is outer
      {NORTH, EAST,  center_x + casing_width / 2.0f, center_y - half_cell},
      // EAST+SOUTH → bottom-right is outer
      {EAST,  SOUTH, center_x + casing_width / 2.0f, center_y + casing_width / 2.0f},
      // SOUTH+WEST → bottom-left is outer
      {SOUTH, WEST,  center_x - half_cell, center_y + casing_width / 2.0f},
      // WEST+NORTH → top-left is outer
      {WEST,  NORTH, center_x - half_cell, center_y - half_cell},
  };

  float casing_patch = half_cell - casing_width / 2.0f;
  SDL_SetRenderDrawColor(game->renderer, 55, 40, 50, 255);
  for (int i = 0; i < 4; i++) {
    if (active[corners[i].a] && active[corners[i].b]) {
      SDL_FRect patch = {corners[i].x, corners[i].y, casing_patch, casing_patch};
      SDL_RenderFillRect(game->renderer, &patch);
    }
  }

  struct {
    Orientation a, b;
    float x, y;
  } ch_corners[4] = {
      {NORTH, EAST,  center_x + channel_width / 2.0f, center_y - half_cell},
      {EAST,  SOUTH, center_x + channel_width / 2.0f, center_y + channel_width / 2.0f},
      {SOUTH, WEST,  center_x - half_cell, center_y + channel_width / 2.0f},
      {WEST,  NORTH, center_x - half_cell, center_y - half_cell},
  };

  float channel_patch = half_cell - channel_width / 2.0f;
  SDL_SetRenderDrawColor(game->renderer, 185, 175, 165, 255);
  for (int i = 0; i < 4; i++) {
    if (active[ch_corners[i].a] && active[ch_corners[i].b]) {
      SDL_FRect patch = {ch_corners[i].x, ch_corners[i].y,
                         channel_patch, channel_patch};
      SDL_RenderFillRect(game->renderer, &patch);
    }
  }

  // Direction arrow
  SDL_SetRenderDrawColor(game->renderer, 255, 245, 235, 255);
  draw_pipe_direction_arrow(game->renderer, center_x, center_y, orientation,
                            state->cell_size);
}

static void draw_addition(NumberFactory *game, SDL_FRect rect,
                          Addition *addition, Vector2i cell_pos) {
  SDL_SetRenderDrawColor(game->renderer, 100, 150, 200, 255);
  SDL_RenderFillRect(game->renderer, &rect);

  SDL_SetRenderDrawColor(game->renderer, 60, 90, 120, 255);
  SDL_RenderRect(game->renderer, &rect);

  Vector2i primary = addition->position;
  Vector2i secondary =
      vector_add(primary, orientation_vector(addition->orientation));

  if (vector_equal(cell_pos, primary)) {
    draw_orientation_arrow(game->renderer, rect, addition->orientation);
  }

  if (vector_equal(cell_pos, secondary)) {
    draw_orientation_arrow(
        game->renderer, rect,
        orientation_opposite(orientation_clockwise(addition->orientation)));
    draw_orientation_arrow(game->renderer, rect, addition->orientation);
  }

  float cx = rect.x + rect.w * 0.5f;
  float cy = rect.y + rect.h * 0.5f;
  float r = rect.w * 0.12f;
  SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
  SDL_FRect center = {cx - r, cy - r, r * 2, r * 2};
  SDL_RenderFillRect(game->renderer, &center);
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

static SDL_AppResult pause_game(NumberFactory *game) {
  GameState *pause = pause_state_create(game);
  if (!pause) return SDL_APP_FAILURE;

  if (!game_state_stack_push(&game->states, pause)) {
    pause->destroy(pause, game);
    SDL_Log("Failed to push pause state");
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

static SDL_AppResult gameplay_state_event(GameState *base, NumberFactory *game,
                                          SDL_Event *event) {
  GameplayState *state = (GameplayState *)base;

  if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
    switch (event->key.key) {
    case SDLK_1:
    case SDLK_KP_1:
      state->placement_mode = PLACEMENT_PIPE;
      break;
    case SDLK_2:
    case SDLK_KP_2:
      state->placement_mode = PLACEMENT_SPLITTER;
      break;
    case SDLK_3:
    case SDLK_KP_3:
      state->placement_mode = PLACEMENT_ADDITION;
      break;
    case SDLK_R:
      rotate_orientation(state, 1);
      break;
    case SDLK_ESCAPE:
      return pause_game(game);
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
        switch (state->placement_mode) {
          case PLACEMENT_PIPE:
            level_place_pipe(&state->level, cell, state->orientation);
            break;
          case PLACEMENT_SPLITTER:
            level_place_splitter(&state->level, cell, state->orientation);
            break;
          case PLACEMENT_ADDITION:
            level_place_addition(&state->level, cell, state->orientation);
            break;
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
  GameplayState *state = (GameplayState *)base;

  Uint64 now = SDL_GetTicks();
  float dt = (now - state->last_ticks) / 1000.0f;
  state->last_ticks = now;

  level_update(&state->level, dt);
  if (state->level.health <= 0 && state->level.missed_item_count == 0) {
    // TODO: Replace this with a dedicated game-over state.
    game_state_stack_pop(&game->states, game);
  }
  return SDL_APP_CONTINUE;
}

static void gameplay_state_resume(GameState *base, NumberFactory *game) {
  (void)game;
  GameplayState *state = (GameplayState *)base;
  state->last_ticks = SDL_GetTicks();
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
        draw_pipe_cell(game, state, (Vector2i){x, y}, pipe);
        break;
      }
      case ENTITY_NONE:
        break;
      case ENTITY_INPUT: {
        Input *input = &entity->input;
        draw_input(game, rect, input);
        break;
      }
      case ENTITY_OUTPUT: {
        Output *output = &entity->output;
        draw_output(game, state, rect, output);
        break;
      }
      case ENTITY_SPLITTER: {
        Splitter *splitter = &entity->splitter;
        draw_splitter(game, rect, splitter);
        break;
      }
      case ENTITY_ADDITION: {
        Addition *addition = &entity->addition;
        draw_addition(game, rect, addition, (Vector2i){x, y});
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

        DrawItemCallbackData data = {game, state};
        pipe_for_each_item_position(pipe, draw_item_callback, &data);
        break;
      }
      case ENTITY_INPUT: {
        Input *input = &entity->input;
        DrawItemCallbackData data = {game, state};
        input_for_each_item_position(input, draw_item_callback, &data);
        break;
      }
      case ENTITY_SPLITTER: {
        Splitter *splitter = &entity->splitter;
        DrawItemCallbackData data = {game, state};
        splitter_for_each_item_position(splitter, draw_item_callback, &data);
        break;
      }
      case ENTITY_ADDITION: {
        Addition *addition = &entity->addition;
        DrawItemCallbackData data = {game, state};
        addition_for_each_item_position(addition, draw_item_callback, &data);
        break;
      }
      case ENTITY_NONE:
        break;
      case ENTITY_OUTPUT:
        break;
      }
    }
  }

  for (int i = 0; i < state->level.missed_item_count; i++) {
    MissedItem *item = &state->level.missed_items[i];
    Vector2f pos = level_missed_item_position(item);
    float t = MIN(1.0f, MAX(0.0f, item->progress));
    float scale = 1.0f + 4.0f * t * (1.0f - t);
    draw_item_scaled(game, state, pos, item->value, scale);
  }

  if (state->has_hovered_cell) {
    SDL_FRect preview = {
        .x = state->board_offset_x + state->hovered_cell.x * state->cell_size,
        .y = state->board_offset_y + state->hovered_cell.y * state->cell_size,
        .w = state->cell_size,
        .h = state->cell_size,
    };
    if (state->placement_mode == PLACEMENT_ADDITION) {
      SDL_SetRenderDrawColor(game->renderer, 100, 150, 200, 48);
    } else if (state->placement_mode == PLACEMENT_SPLITTER) {
      SDL_SetRenderDrawColor(game->renderer, 80, 255, 255, 48);
    } else {
      SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 48);
    }
    SDL_RenderFillRect(game->renderer, &preview);

    // Draw secondary cell preview for 2-cell machines
    if (state->placement_mode == PLACEMENT_ADDITION) {
      Vector2i secondary = vector_add(state->hovered_cell,
                                      orientation_vector(state->orientation));
      if (BOARD_VALID(&state->level.board, secondary.x, secondary.y)) {
        SDL_FRect preview2 = {
            .x = state->board_offset_x + secondary.x * state->cell_size,
            .y = state->board_offset_y + secondary.y * state->cell_size,
            .w = state->cell_size,
            .h = state->cell_size,
        };
        SDL_SetRenderDrawColor(game->renderer, 100, 150, 200, 48);
        SDL_RenderFillRect(game->renderer, &preview2);
        SDL_SetRenderDrawColor(game->renderer, 100, 150, 200, 192);
        SDL_RenderRect(game->renderer, &preview2);
      }
    }

    if (state->placement_mode == PLACEMENT_ADDITION) {
      SDL_SetRenderDrawColor(game->renderer, 100, 150, 200, 192);
    } else if (state->placement_mode == PLACEMENT_SPLITTER) {
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
  state->base.resume = gameplay_state_resume;

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
