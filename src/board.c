#include "board.h"

#include <stdlib.h>

void board_init(Board *board, int width, int height) {
  board->width = width;
  board->height = height;
  board->entities = malloc(width * height * sizeof(EntityId));
  for (int i = 0; i < width * height; i++) {
    board->entities[i] = ENTITY_NONE;
  }
}

void board_free(Board *board) { free(board->entities); }
