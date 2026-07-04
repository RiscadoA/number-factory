#ifndef LEVEL_H
#define LEVEL_H

#include "board.h"
#include "entity.h"

typedef struct {
  Board board;
  EntityPool entity_pool;
} Level;

void level_init(Level *level);

void level_free(Level *level);

void level_update(Level *level, float dt);

#endif
