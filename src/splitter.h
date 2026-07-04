#ifndef SPLITTER_H
#define SPLITTER_H

#include "entity.h"
#include "utils.h"

typedef struct {
  // Which outputs are active (1) or inactive (0).
  int outputs[4];

  // Index of the next output to use.
  int current_output;

  // Current value in the splitter.
  int value;

  // Progress from 0.0 to 1.0 for outputting the current value.
  float progress;
} Splitter;

void splitter_init(Splitter *splitter);

void splitter_enable_output(Splitter *splitter, Orientation output);

void splitter_disable_output(Splitter *splitter, Orientation output);

Orientation splitter_current_output(Splitter *splitter);

int splitter_feed(Splitter *splitter, int value);

// Should only be called when the splitter's current output has space to output
// a value. Returns the value that was just output, or 0 if none.
int splitter_update(Splitter *splitter, float dt);

#endif
