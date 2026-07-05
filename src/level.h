#ifndef LEVEL_H
#define LEVEL_H

#include "board.h"
#include "entity.h"
#include "utils.h"

typedef struct {
  int value;
  Vector2f start;
  Orientation orientation;
  float progress;
} MissedItem;

typedef struct {
  Board board;
  EntityPool entity_pool;
  int health;
  int max_health;
  MissedItem *missed_items;
  int missed_item_count;
  int missed_item_capacity;
} Level;

void level_init(Level *level);

void level_free(Level *level);

void level_update(Level *level, float dt);

Vector2f level_missed_item_position(const MissedItem *item);

// Places a pipe cell at the given position with the given orientation.
int level_place_pipe(Level *level, Vector2i pos, Orientation orientation);

int level_place_splitter(Level *level, Vector2i pos, Orientation orientation);

int level_place_addition(Level *level, Vector2i pos, Orientation orientation);

// Removes whatever is at the given position.
int level_remove(Level *level, Vector2i pos);

#endif
