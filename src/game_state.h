#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL3/SDL.h>

typedef struct NumberFactory NumberFactory;
typedef struct GameState GameState;

struct GameState {
  void (*destroy)(GameState *state, NumberFactory *game);
  SDL_AppResult (*event)(GameState *state, NumberFactory *game,
                         SDL_Event *event);
  SDL_AppResult (*update)(GameState *state, NumberFactory *game);
  void (*render)(GameState *state, NumberFactory *game);
};

typedef struct {
  GameState **items;
  int count;
  int capacity;
} GameStateStack;

void game_state_stack_init(GameStateStack *stack);

void game_state_stack_free(GameStateStack *stack, NumberFactory *game);

int game_state_stack_push(GameStateStack *stack, GameState *state);

void game_state_stack_pop(GameStateStack *stack, NumberFactory *game);

GameState *game_state_stack_top(GameStateStack *stack);

SDL_AppResult game_state_stack_event(GameStateStack *stack, NumberFactory *game,
                                     SDL_Event *event);

SDL_AppResult game_state_stack_update(GameStateStack *stack,
                                      NumberFactory *game);

void game_state_stack_render(GameStateStack *stack, NumberFactory *game);

#endif
