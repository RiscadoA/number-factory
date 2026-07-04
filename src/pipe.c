#include "pipe.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEQUE_ELEMENT_TYPE PipeItem
#define DEQUE_FUNCTION_PREFIX pipe_item_deque
#define DEQUE_IMPLEMENT
#include "deque.h"

#define DEQUE_ELEMENT_TYPE PipeCell
#define DEQUE_FUNCTION_PREFIX pipe_cell_deque
#define DEQUE_IMPLEMENT
#include "deque.h"

static const float ITEM_GAP = 1.0f;
static const float ITEM_SPEED = 1.0f;

// returns -1 if the position is not found in the pipe
static int position_to_index(Pipe *pipe, Vector2i pos) {
  for (int i = 0; i < pipe->cells.size; i++) {
    if (DEQUE_AT(pipe->cells, i).pos.x == pos.x &&
        DEQUE_AT(pipe->cells, i).pos.y == pos.y) {
      return i;
    }
  }
  return -1;
}

void pipe_init(Pipe *pipe, int capacity) {
  pipe_cell_deque_init(&pipe->cells, capacity);
  pipe_item_deque_init(&pipe->items, capacity);
}

void pipe_free(Pipe *pipe) {
  pipe_cell_deque_free(&pipe->cells);
  pipe_item_deque_free(&pipe->items);
}

int pipe_extend_input(Pipe *pipe, Vector2i pos, Orientation orientation) {
  if (pipe->cells.size != 0 &&
      !is_position_adjacent(DEQUE_AT(pipe->cells, 0).pos, pos)) {
    fprintf(stderr,
            "pipe_extend_input: position %d,%d is not adjacent to first cell\n",
            pos.x, pos.y);
    return 0;
  }

  pipe_cell_deque_push_front(&pipe->cells, (PipeCell){
                                               .pos = pos,
                                               .orientation = orientation,
                                           });
  return 1;
}

int pipe_extend_output(Pipe *pipe, Vector2i pos, Orientation orientation) {
  if (pipe->cells.size != 0 &&
      !is_position_adjacent(DEQUE_AT(pipe->cells, pipe->cells.size - 1).pos,
                            pos)) {
    fprintf(stderr,
            "pipe_extend_output: position %d,%d is not adjacent to last cell\n",
            pos.x, pos.y);
    return 0;
  }

  pipe_cell_deque_push_back(&pipe->cells, (PipeCell){
                                              .pos = pos,
                                              .orientation = orientation,
                                          });
  return 1;
}

int pipe_contract_input(Pipe *pipe) {
  if (pipe->cells.size == 0) {
    return 0;
  }
  pipe_cell_deque_pop_front(&pipe->cells);
  for (int i = 0; i < pipe->items.size; i++) {
    DEQUE_AT(pipe->items, i).distance_from_start -= 1.0f;
  }
  return 1;
}

int pipe_merge(Pipe *input, Pipe *output, Pipe *result) {
  // Input pipe needs to point into the output pipe's first cell.
  if (!is_position_adjacent(DEQUE_AT(input->cells, input->cells.size - 1).pos,
                            DEQUE_AT(output->cells, 0).pos)) {
    return 0;
  }

  pipe_init(result, input->cells.capacity + output->cells.capacity);

  // Merge cells
  for (int i = 0; i < input->cells.size; i++) {
    pipe_cell_deque_push_back(&result->cells, DEQUE_AT(input->cells, i));
  }
  for (int i = 0; i < output->cells.size; i++) {
    pipe_cell_deque_push_back(&result->cells, DEQUE_AT(output->cells, i));
  }

  // Merge items
  for (int i = 0; i < input->items.size; i++) {
    pipe_item_deque_push_back(&result->items, DEQUE_AT(input->items, i));
  }
  for (int i = 0; i < output->items.size; i++) {
    PipeItem item = DEQUE_AT(output->items, i);
    item.distance_from_start += (float)input->cells.size;
    pipe_item_deque_push_back(&result->items, item);
  }

  return 1;
}

int pipe_split(Pipe *pipe, Vector2i start_pos, Pipe *result_input,
               Pipe *result_output) {
  int output_start_index = position_to_index(pipe, start_pos);
  if (output_start_index < 0) {
    fprintf(stderr, "Invalid start position: (%d, %d)\n", start_pos.x,
            start_pos.y);
    return 0;
  }

  pipe_init(result_input, output_start_index);
  pipe_init(result_output, pipe->cells.size - output_start_index);

  for (int i = 0; i < output_start_index; ++i) {
    pipe_cell_deque_push_back(&result_input->cells, DEQUE_AT(pipe->cells, i));
  }
  for (int i = output_start_index; i < pipe->cells.size; ++i) {
    pipe_cell_deque_push_back(&result_output->cells, DEQUE_AT(pipe->cells, i));
  }

  for (int i = 0; i < pipe->items.size; ++i) {
    PipeItem item = DEQUE_AT(pipe->items, i);
    if (item.distance_from_start <= (float)output_start_index) {
      pipe_item_deque_push_back(&result_input->items, item);
    } else {
      item.distance_from_start -= (float)output_start_index;
      pipe_item_deque_push_back(&result_output->items, item);
    }
  }

  return 1;
}

int pipe_update(Pipe *pipe, int can_output, float dt) {
  float max_distance = can_output ? INFINITY : (float)pipe->cells.size;
  int return_value = 0;

  for (int i = pipe->items.size - 1; i >= 0; --i) {
    PipeItem *item = &DEQUE_AT(pipe->items, i);
    item->distance_from_start += dt * ITEM_SPEED;
    item->distance_from_start = fminf(item->distance_from_start, max_distance);
    if (item->distance_from_start > (float)pipe->cells.size) {
      return_value = item->value;
      pipe_item_deque_pop_front(&pipe->items);
      max_distance = (float)pipe->cells.size;
    } else {
      max_distance = item->distance_from_start - ITEM_GAP;
    }
  }

  return return_value;
}

Orientation pipe_orientation(Pipe *pipe, Vector2i pos) {
  int index = position_to_index(pipe, pos);
  if (index < 0) {
    fprintf(stderr, "pipe_orientation: position not found\n");
    return -1;
  }
  return DEQUE_AT(pipe->cells, index).orientation;
}

int pipe_is_start_cell(Pipe *pipe, Vector2i pos) {
  int index = position_to_index(pipe, pos);
  if (index < 0) {
    fprintf(stderr, "pipe_is_start_cell: position not found\n");
    return -1;
  }
  return index == 0;
}

int pipe_is_turn_cell(Pipe *pipe, Vector2i pos) {
  int index = position_to_index(pipe, pos);
  if (index < 0) {
    fprintf(stderr, "pipe_is_turn_cell: position not found\n");
    return -1;
  }
  if (index == 0) {
    return 0;
  }

  return DEQUE_AT(pipe->cells, index).orientation !=
         DEQUE_AT(pipe->cells, index - 1).orientation;
}

void pipe_debug(Pipe *pipe) {
  for (int i = 0; i < pipe->cells.size; ++i) {
    PipeCell cell = DEQUE_AT(pipe->cells, i);
    fprintf(stderr, "| %s (%d, %d) ", orientation_to_string(cell.orientation),
            cell.pos.x, cell.pos.y);
  }
  fprintf(stderr, "\n");
}
