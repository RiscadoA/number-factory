#include "splitter.h"

#include <stdio.h>

void splitter_init(Splitter *splitter) {
  splitter->progress = 0.0f;
  for (int i = 0; i < 4; i++) {
    splitter->outputs[i] = 0;
  }
  splitter->current_output = 0;
}

void splitter_enable_output(Splitter *splitter, Orientation output) {
  splitter->outputs[output] = 1;
}

void splitter_disable_output(Splitter *splitter, Orientation output) {
  splitter->outputs[output] = 0;
}

Orientation splitter_current_output(Splitter *splitter) {
  return (Orientation)splitter->current_output;
}

int splitter_feed(Splitter *splitter, int value) {
  if (splitter->value) {
    return 0;
  }

  splitter->value = value;
  splitter->progress = 0.0f;
  return 1;
}

int splitter_update(Splitter *splitter, float dt) {
  if (!splitter->value) {
    return 0;
  }

  for (int i = 0; i < 4 && !splitter->outputs[splitter->current_output]; i++) {
    splitter->current_output = (splitter->current_output + 1) % 4;
  }
  if (!splitter->outputs[splitter->current_output]) {
    fprintf(stderr, "Spliter has no active output\n");
    return 0;
  }

  splitter->progress += dt;
  if (splitter->progress >= 1.0f) {
    splitter->progress = 0.0f;
    splitter->current_output = (splitter->current_output + 1) % 4;
  }
  return 1;
}
