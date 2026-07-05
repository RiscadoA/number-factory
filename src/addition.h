#ifndef ADDITION_H
#define ADDITION_H

#include "item.h"
#include "utils.h"

typedef struct {
  Vector2i position;
  Orientation orientation;
  ItemDeque input_items[2];
  ItemDeque output_items;
} Addition;

void addition_init(Addition *addition, Vector2i position,
                   Orientation orientation);

void addition_free(Addition *addition);

int addition_add_item(Addition *addition, Vector2i pos, int value);

void addition_update(Addition *addition,
                     int (*callback)(void *user, Orientation orientation,
                                     int value),
                     void *user, float dt);

void addition_for_each_item_position(Addition *addition,
                                     void (*callback)(void *user, int value,
                                                      Vector2f position),
                                     void *user);

#endif
