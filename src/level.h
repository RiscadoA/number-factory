#ifndef LEVEL_H
#define LEVEL_H

#include "board.h"
#include "entity.h"
#include "utils.h"

typedef struct {
  Board board;
  EntityPool entity_pool;
} Level;

void level_init(Level *level);

void level_free(Level *level);

void level_update(Level *level, float dt);

// Places a pipe cell at the given position with the given orientation.
int level_place_pipe(Level *level, Vector2i pos, Orientation orientation);

// Removes whatever is at the given position.
int level_remove(Level *level, Vector2i pos);

#endif
