#include "level.h"

void level_init(Level *level) {
  board_init(&level->board, 12, 8);
  entity_pool_init(&level->entity_pool, 12 * 8);
}

void level_free(Level *level) {
  board_free(&level->board);
  entity_pool_free(&level->entity_pool);
}
