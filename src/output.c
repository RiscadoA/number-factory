#include "output.h"

#include <SDL3/SDL.h>

void output_init(Output *output, Vector2i position,
                 Orientation input_orientation, int target_value) {
  output->position = position;
  output->input_orientation = input_orientation;
  output->target_value = target_value;
  output->received_count = 0;
}

void output_free(Output *output) {
  (void)output;
}

int output_accept(Output *output, Orientation orientation, int value) {
  if (orientation != output->input_orientation ||
      value != output->target_value) {
    return 0;
  }

  output->received_count++;
  SDL_Log("Output accepted %d (count: %d)", value, output->received_count);
  return 1;
}
