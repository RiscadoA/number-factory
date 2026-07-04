#include "pipe.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// returns -1 if the position is not found in the pipe
static int position_to_index(Pipe *pipe, Vector2i pos) {
  for (int i = 0; i < pipe->length; i++) {
    if (pipe->positions[i].x == pos.x && pipe->positions[i].y == pos.y) {
      return i;
    }
  }
  return -1;
}

void pipe_init(Pipe *pipe, Vector2i pos) {
  pipe->positions = malloc(sizeof(Vector2i));
  pipe->length = 1;
  pipe->positions[0] = pos;

  pipe->values = malloc(sizeof(int));
  pipe->values[0] = 0;
  pipe->first_value = 0;

  pipe->progress = 0.0f;
}

void pipe_free(Pipe *pipe) {
  free(pipe->positions);
  free(pipe->values);
}

void pipe_extend(Pipe *pipe, Vector2i pos) {
  int is_append;
  if (is_position_adjacent(pos, pipe->positions[0])) {
    is_append = 0;
  } else if (is_position_adjacent(pos, pipe->positions[pipe->length - 2])) {
    is_append = 1;
  } else {
    fprintf(stderr, "position %d,%d is not adjacent to a pipe end\n", pos.x,
            pos.y);
    return;
  }

  pipe->length += 1;
  pipe->positions = realloc(pipe->positions, pipe->length * sizeof(Vector2i));
  pipe->values = realloc(pipe->values, pipe->length * sizeof(int));

  // In both append/prepend, move values ahead in the values buffer.
  // The new cell will be at the current first_value position.
  // If appending, first_value is incremented, making the new value the last
  // cell.
  memmove(pipe->values + pipe->first_value,
          pipe->values + pipe->first_value + 1,
          (pipe->length - pipe->first_value) * sizeof(int));
  pipe->values[pipe->first_value] = 0;

  if (is_append) {
    pipe->positions[pipe->length - 1] = pos;
    pipe->values[pipe->first_value] = 0;
    pipe->first_value += 1;
  } else {
    memmove(pipe->positions + 1, pipe->positions,
            pipe->length - 1 * sizeof(Vector2i));
    pipe->positions[0] = pos;
  }
}

int pipe_can_feed(Pipe *pipe, Vector2i pos) {
  int index = position_to_index(pipe, pos);
  return index >= 0 && !pipe->values[index];
}

int pipe_feed(Pipe *pipe, Vector2i pos, int value) {
  int index = position_to_index(pipe, pos);
  if (index < 0 || pipe->values[index]) {
    return 0;
  }
  pipe->values[index] = value;
  return 1;
}

int pipe_update(Pipe *pipe, float dt) {
  pipe->progress += dt;
  if (pipe->progress < 1.0f) {
    return 0;
  }

  pipe->progress -= 1.0f;
  if (pipe->progress > 1.0f) {
    fprintf(stderr,
            "dt is too large, doing single pipe move per update anyway\n");
  }

  // Extract the value at the end of the pipe and clear it.
  int last_value = (pipe->first_value + pipe->length - 1) % pipe->length;
  int value = pipe->values[last_value];
  pipe->values[last_value] = 0;

  pipe->first_value = (pipe->first_value + 1) % pipe->length;

  return value;
}

Orientation pipe_orientation(Pipe *pipe, Vector2i pos) {
  int index = position_to_index(pipe, pos);
  if (index < 0) {
    fprintf(stderr, "pipe_orientation: position not found\n");
    return -1;
  }
  if (pipe->length == 1) {
    return -1;
  }

  if (index == pipe->length - 1) {
    index -= 1;
  }

  return vector_orientation(vector_subtract(pipe->positions[index + 1], pipe->positions[index]));
}
