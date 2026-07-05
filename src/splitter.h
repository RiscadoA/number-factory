#ifndef SPLITTER_H
#define SPLITTER_H

#include "item.h"
#include "utils.h"

typedef struct {
  Vector2i position;
  Orientation orientation;
  ItemDeque input_items;
  ItemDeque output_items[4];
  Orientation current_output;
} Splitter;

void splitter_init(Splitter *splitter, Vector2i position,
                   Orientation orientation);

int splitter_add_item(Splitter *splitter, int value);

void splitter_update(Splitter *splitter,
                     int (*callback)(void *user, Orientation orientation,
                                     int value),
                     void *user, float dt);

void splitter_for_each_item_position(Splitter *splitter,
                                     void (*callback)(void *user, int value,
                                                      Vector2f position),
                                     void *user);

#endif
