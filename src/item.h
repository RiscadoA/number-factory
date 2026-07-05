#ifndef ITEM_H
#define ITEM_H

#include "utils.h"

typedef struct {
  int value;
  float distance_from_start;
  Orientation source_orientation;
} Item;

#define DEQUE_ELEMENT_TYPE Item
#define DEQUE_FUNCTION_PREFIX item_deque
#include "deque.h"

// Tries to add an item into an item deque (sorted by distance)
// If it is not possible such that the item gap is preserved, returns 0
int items_add(ItemDeque *items, Item item);

void items_update(ItemDeque *items, float output_distance,
                  int (*on_output)(void *user, int value), void *user,
                  float dt);

Vector2f item_position(Vector2i cell, float progress, Orientation from, Orientation to);

#endif
