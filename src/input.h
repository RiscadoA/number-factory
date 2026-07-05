#ifndef INPUT_H
#define INPUT_H

#include "item.h"
#include "utils.h"

typedef struct {
  Vector2i position;
  Orientation orientation;

  // Value to output.
  int value;

  // How much time must elapse before outputting the next value.
  float time_per_value;
  float time_accumulator;

  ItemDeque items;
} Input;

void input_init(Input *input, Vector2i position, Orientation orientation,
                int value, float time_per_value);

void input_free(Input *input);

Vector2i input_output_position(Input *input);

int input_update(Input *input, int (*callback)(void *user, int value),
                 void *user, float dt);

void input_for_each_item_position(Input *input,
                                  void (*callback)(void *user, int value,
                                                   Vector2f position),
                                  void *user);

#endif
