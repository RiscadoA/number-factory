#ifndef PIPE_H
#define PIPE_H

#include "utils.h"

/// A set of pipe cells, ordered from start to end.
typedef struct {
  Vector2i *positions;
  int length;

  // Same length as positions, but indices don't match up.
  // values[first_value] is the cell at positions[0].
  //
  // Moving the pipe forward just adjusts first_value.
  int *values;
  int first_value;

  // Inner-cell movement progress from 0.0 to 1.0, incremented by dt each
  // frame. When it reaches 1.0, the pipe moves forward.
  float progress;
} Pipe;

void pipe_init(Pipe *pipe, Vector2i pos);

void pipe_free(Pipe *pipe);

// Must be adjacent to either the start or end of the pipe.
void pipe_extend(Pipe *pipe, Vector2i pos);

// Returns 1 if a value can be fed into the pipe at the given position, 0
// otherwise.
int pipe_can_feed(Pipe *pipe, Vector2i pos);

// Feeds a value into the pipe at the given position, if possible.
int pipe_feed(Pipe *pipe, Vector2i pos, int value);

// Should only be called when there's space for the pipe to output values into.
// Returns the value that was just output, or 0 if none.
int pipe_update(Pipe *pipe, float dt);

#endif
