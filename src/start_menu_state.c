#include "start_menu_state.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#include "game.h"
#include "gameplay_state.h"

typedef struct {
  GameState base;
  SDL_Texture *title;
  SDL_Texture *prompt;
} StartMenuState;

static SDL_Texture *create_text_texture(NumberFactory *game, const char *text) {
  SDL_Color color = {.r = 40, .g = 30, .b = 40, .a = 255};
  SDL_Surface *surface = TTF_RenderText_Blended(game->font, text, 0, color);
  if (!surface) return NULL;

  SDL_Texture *texture = SDL_CreateTextureFromSurface(game->renderer, surface);
  SDL_DestroySurface(surface);
  if (texture) {
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  }
  return texture;
}

static void render_centered_texture(NumberFactory *game, SDL_Texture *texture,
                                    float center_y, float max_width,
                                    float max_scale) {
  float width;
  float height;
  if (!SDL_GetTextureSize(texture, &width, &height)) return;

  float scale = max_scale;
  if (width * scale > max_width) {
    scale = max_width / width;
  }

  SDL_FRect destination = {
      .x = (GAME_WIDTH - width * scale) / 2.0f,
      .y = center_y - height * scale / 2.0f,
      .w = width * scale,
      .h = height * scale,
  };
  SDL_RenderTexture(game->renderer, texture, NULL, &destination);
}

static void start_menu_state_destroy(GameState *base, NumberFactory *game) {
  (void)game;
  StartMenuState *state = (StartMenuState *)base;
  if (state->prompt) SDL_DestroyTexture(state->prompt);
  if (state->title) SDL_DestroyTexture(state->title);
  free(state);
}

static SDL_AppResult start_game(NumberFactory *game) {
  GameState *gameplay = gameplay_state_create(game);
  if (!gameplay) {
    SDL_Log("Failed to create gameplay state");
    return SDL_APP_FAILURE;
  }

  if (!game_state_stack_push(&game->states, gameplay)) {
    gameplay->destroy(gameplay, game);
    SDL_Log("Failed to push gameplay state");
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

static SDL_AppResult
start_menu_state_event(GameState *base, NumberFactory *game, SDL_Event *event) {
  (void)base;
  if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
      (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER)) {
    return start_game(game);
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event->button.button == SDL_BUTTON_LEFT) {
    return start_game(game);
  }

  return SDL_APP_CONTINUE;
}

static SDL_AppResult start_menu_state_update(GameState *base,
                                             NumberFactory *game) {
  (void)base;
  (void)game;
  return SDL_APP_CONTINUE;
}

static void start_menu_state_render(GameState *base, NumberFactory *game) {
  StartMenuState *state = (StartMenuState *)base;

  SDL_SetRenderDrawColor(game->renderer, 220, 190, 200, 255);
  SDL_RenderClear(game->renderer);

  render_centered_texture(game, state->title, 240.0f, GAME_WIDTH * 0.8f, 1.0f);
  render_centered_texture(game, state->prompt, 360.0f, GAME_WIDTH * 0.6f,
                          0.55f);
}

GameState *start_menu_state_create(NumberFactory *game) {
  StartMenuState *state = calloc(1, sizeof(*state));
  if (!state) return NULL;

  state->base.destroy = start_menu_state_destroy;
  state->base.event = start_menu_state_event;
  state->base.update = start_menu_state_update;
  state->base.render = start_menu_state_render;

  state->title = create_text_texture(game, "NUMBER FACTORY");
  if (!state->title) {
    SDL_Log("Failed to create menu title: %s", SDL_GetError());
    start_menu_state_destroy(&state->base, game);
    return NULL;
  }

  state->prompt = create_text_texture(game, "PRESS ENTER");
  if (!state->prompt) {
    SDL_Log("Failed to create menu prompt: %s", SDL_GetError());
    start_menu_state_destroy(&state->base, game);
    return NULL;
  }

  return &state->base;
}
