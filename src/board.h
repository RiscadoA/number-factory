#ifndef BOARD_H
#define BOARD_H

#include "entity.h"

typedef struct {
  int width;
  int height;
  EntityId *entities;
} Board;

// Allocates and zeroes a board of the given width and height.
void board_init(Board *board, int width, int height);

void board_free(Board *board);

#define BOARD_AT(board, x, y) ((board)->entities[(y) * (board)->width + (x)])

#endif
