#ifndef PIPE_H
#define PIPE_H

#include "utils.h"
#include "item.h"

typedef struct {
  Vector2i pos;
  Orientation orientation;
} PipeCell;

#define DEQUE_ELEMENT_TYPE PipeCell
#define DEQUE_FUNCTION_PREFIX pipe_cell_deque
#include "deque.h"

typedef struct {
  PipeCellDeque cells;
  ItemDeque items;
} Pipe;

void pipe_init(Pipe *pipe, int capacity);

void pipe_free(Pipe *pipe);

int pipe_extend_input(Pipe *pipe, Vector2i pos, Orientation orientation);

int pipe_extend_output(Pipe *pipe, Vector2i pos, Orientation orientation);

int pipe_contract_input(Pipe *pipe);

// Connects two pipes together.
// Pipes must be adjacent to each other, input pipe end targeting output pipe
// start.
int pipe_merge(Pipe *input, Pipe *output, Pipe *result);

// Splits a pipe into two pipes at the given position, such that the resulting
// output pipe starts at the given position.
int pipe_split(Pipe *pipe, Vector2i start_pos, Pipe *result_input,
               Pipe *result_output);

void pipe_update(Pipe *pipe, int (*callback)(void *user, int value), void *user,
                 float dt);

// Gets the orientation of the pipe at the given position.
// If the pipe has a single cell, or the operation fails, returns -1.
Orientation pipe_orientation(Pipe *pipe, Vector2i pos);

// Checks if the given cell is the beginning or end of the pipe.
int pipe_is_start_cell(Pipe *pipe, Vector2i pos);

// Checks if the given cell has an aligned input cell.
int pipe_is_turn_cell(Pipe *pipe, Vector2i pos);

Vector2i pipe_output_position(Pipe *pipe);

Orientation pipe_output_orientation(Pipe *pipe);

int pipe_add_item(Pipe *pipe, Vector2i pos, Orientation source_orientation,
                  int value);

Vector2f pipe_item_position(Pipe *pipe, int index);

void pipe_debug(Pipe *pipe);

#endif
