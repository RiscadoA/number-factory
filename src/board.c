#include "board.h"

#include <stdlib.h>

void board_init(Board *board, int width, int height) {
  board->width = width;
  board->height = height;
  board->entities = calloc(width * height * sizeof(EntityId), 1);
}

void board_free(Board *board) { free(board->entities); }
