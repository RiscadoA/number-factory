#ifndef OUTPUT_H
#define OUTPUT_H

#include "utils.h"

typedef struct {
  Vector2i position;
  Orientation input_orientation;
  int target_value;
  int received_count;
} Output;

void output_init(Output *output, Vector2i position,
                 Orientation input_orientation, int target_value);

void output_free(Output *output);

int output_accept(Output *output, Orientation orientation, int value);

#endif
