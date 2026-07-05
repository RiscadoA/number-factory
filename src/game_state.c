#include "game_state.h"

#include <stdlib.h>

void game_state_stack_init(GameStateStack *stack) {
  stack->items = NULL;
  stack->count = 0;
  stack->capacity = 0;
}

void game_state_stack_free(GameStateStack *stack, NumberFactory *game) {
  while (stack->count > 0) {
    game_state_stack_pop(stack, game);
  }
  free(stack->items);
  stack->items = NULL;
  stack->capacity = 0;
}

int game_state_stack_push(GameStateStack *stack, GameState *state) {
  if (stack->count == stack->capacity) {
    int capacity = stack->capacity == 0 ? 4 : stack->capacity * 2;
    GameState **items = realloc(stack->items, capacity * sizeof(*stack->items));
    if (!items) {
      return 0;
    }
    stack->items = items;
    stack->capacity = capacity;
  }

  stack->items[stack->count++] = state;
  return 1;
}

void game_state_stack_pop(GameStateStack *stack, NumberFactory *game) {
  if (stack->count == 0) return;

  GameState *state = stack->items[--stack->count];
  state->destroy(state, game);

  GameState *resumed = game_state_stack_top(stack);
  if (resumed && resumed->resume) resumed->resume(resumed, game);
}

GameState *game_state_stack_top(GameStateStack *stack) {
  if (stack->count == 0) return NULL;
  return stack->items[stack->count - 1];
}

SDL_AppResult game_state_stack_event(GameStateStack *stack, NumberFactory *game,
                                     SDL_Event *event) {
  GameState *state = game_state_stack_top(stack);
  if (!state) return SDL_APP_CONTINUE;
  return state->event(state, game, event);
}

SDL_AppResult game_state_stack_update(GameStateStack *stack,
                                      NumberFactory *game) {
  GameState *state = game_state_stack_top(stack);
  if (!state) return SDL_APP_CONTINUE;
  return state->update(state, game);
}

void game_state_stack_render(GameStateStack *stack, NumberFactory *game) {
  for (int i = 0; i < stack->count; i++) {
    stack->items[i]->render(stack->items[i], game);
  }
}
