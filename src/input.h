#ifndef INPUT_H
#define INPUT_H

#include "utils.h"

typedef struct {
  Vector2i position;
  Orientation orientation;

  // Value to output.
  int value;

  // How much time must elapse before outputting the next value.
  float time_per_value;
  float time_accumulator;

  // Progress from 0.0 to 1.0 for outputting the current value.
  float progress;
} Input;

void input_init(Input *input, Vector2i position, Orientation orientation,
                int value, float time_per_value);

// Returns the value that was just output, or 0 if none.
int input_update(Input *input, float dt, int can_output);

Vector2i input_output_position(Input *input);

Vector2f input_item_position(Input *input);

#endif
