#include "pause_state.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#include "game.h"

typedef struct {
  GameState base;
  SDL_Texture *title;
  SDL_Texture *prompt;
} PauseState;

static SDL_Texture *create_text_texture(NumberFactory *game, const char *text) {
  SDL_Color color = {.r = 255, .g = 245, .b = 235, .a = 255};
  SDL_Surface *surface = TTF_RenderText_Blended(game->font, text, 0, color);
  if (!surface) return NULL;

  SDL_Texture *texture = SDL_CreateTextureFromSurface(game->renderer, surface);
  SDL_DestroySurface(surface);
  if (texture) SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  return texture;
}

static void render_centered(NumberFactory *game, SDL_Texture *texture,
                            float center_x, float center_y, float max_width,
                            float max_scale) {
  float width;
  float height;
  if (!SDL_GetTextureSize(texture, &width, &height)) return;

  float scale = max_scale;
  if (width * scale > max_width) scale = max_width / width;
  SDL_FRect destination = {
      .x = center_x - width * scale / 2.0f,
      .y = center_y - height * scale / 2.0f,
      .w = width * scale,
      .h = height * scale,
  };
  SDL_RenderTexture(game->renderer, texture, NULL, &destination);
}

static void pause_state_destroy(GameState *base, NumberFactory *game) {
  (void)game;
  PauseState *state = (PauseState *)base;
  if (state->prompt) SDL_DestroyTexture(state->prompt);
  if (state->title) SDL_DestroyTexture(state->title);
  free(state);
}

static SDL_AppResult pause_state_event(GameState *base, NumberFactory *game,
                                       SDL_Event *event) {
  (void)base;
  if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE &&
      !event->key.repeat) {
    game_state_stack_pop(&game->states, game);
  }
  return SDL_APP_CONTINUE;
}

static SDL_AppResult pause_state_update(GameState *base, NumberFactory *game) {
  (void)base;
  (void)game;
  return SDL_APP_CONTINUE;
}

static void pause_state_render(GameState *base, NumberFactory *game) {
  PauseState *state = (PauseState *)base;
  int width;
  int height;
  SDL_GetWindowSize(game->window, &width, &height);

  SDL_SetRenderDrawColor(game->renderer, 25, 18, 24, 205);
  SDL_FRect overlay = {0.0f, 0.0f, width, height};
  SDL_RenderFillRect(game->renderer, &overlay);

  render_centered(game, state->title, width / 2.0f, height * 0.44f,
                  width * 0.7f, 1.0f);
  render_centered(game, state->prompt, width / 2.0f, height * 0.57f,
                  width * 0.75f, 0.45f);
}

GameState *pause_state_create(NumberFactory *game) {
  PauseState *state = calloc(1, sizeof(*state));
  if (!state) return NULL;

  state->base.destroy = pause_state_destroy;
  state->base.event = pause_state_event;
  state->base.update = pause_state_update;
  state->base.render = pause_state_render;

  state->title = create_text_texture(game, "PAUSED");
  state->prompt = create_text_texture(game, "ESC TO UNPAUSE");
  if (!state->title || !state->prompt) {
    SDL_Log("Failed to create pause text: %s", SDL_GetError());
    pause_state_destroy(&state->base, game);
    return NULL;
  }

  return &state->base;
}
