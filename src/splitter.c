#include "splitter.h"
#include "item.h"
#include "utils.h"

#include <stdio.h>

void splitter_init(Splitter *splitter, Vector2i position,
                   Orientation orientation) {
  splitter->position = position;
  splitter->orientation = orientation;
  splitter->current_output = 0;
  item_deque_init(&splitter->input_items, 4);
  for (int i = 0; i < 4; i++) {
    item_deque_init(&splitter->output_items[i], 4);
  }
}

int splitter_add_item(Splitter *splitter, int value) {
  return items_add(&splitter->input_items,
                   (Item){.value = value,
                          .distance_from_start = 0.0f,
                          .source_orientation = splitter->orientation});
}

typedef struct {
  int (*callback)(void *user, Orientation orientation, int value);
  void *user;
  Orientation orientation;
} OutputUpdateArg;

static int output_update_callback(void *user, int value) {
  OutputUpdateArg *arg = (OutputUpdateArg *)user;
  return arg->callback(arg->user, arg->orientation, value);
}

static int input_update_callback(void *user, int value) {
  Splitter *splitter = (Splitter *)user;

  for (int try = 0; try < ORIENTATION_COUNT; try++) {
    // Find an output orientation that isn't the same as the input orientation
    splitter->current_output =
        (splitter->current_output + 1) % ORIENTATION_COUNT;
    if (splitter->current_output == orientation_opposite(splitter->orientation)) {
      continue;
    }

    // Try outputting into our internal output queue
    if (items_add(&splitter->output_items[splitter->current_output],
                  (Item){
                      .distance_from_start = 0.0f,
                      .source_orientation = orientation_opposite(splitter->orientation),
                      .value = value,
                  })) {
      return 1;
    }
  }

  return 0;
}

void splitter_update(Splitter *splitter,
                     int (*callback)(void *user, Orientation orientation,
                                     int value),
                     void *user, float dt) {
  for (Orientation i = ORIENTATION_ZERO; i < ORIENTATION_COUNT; i++) {
    OutputUpdateArg arg = {
        .callback = callback,
        .user = user,
        .orientation = i,
    };
    items_update(&splitter->output_items[i], 0.5F, output_update_callback, &arg,
                 dt);
  }

  items_update(&splitter->input_items, 0.5F, input_update_callback, splitter,
               dt);
}

void splitter_for_each_item_position(Splitter *splitter,
                                     void (*callback)(void *user, int value,
                                                      Vector2f position),
                                     void *user) {
  for (int i = 0; i < splitter->input_items.size; i++) {
    Item item = DEQUE_AT(splitter->input_items, i);
    callback(user, item.value,
             item_position(splitter->position, item.distance_from_start,
                           item.source_orientation, item.source_orientation));
  }
  for (Orientation o = ORIENTATION_ZERO; o < ORIENTATION_COUNT; o++) {
    for (int i = 0; i < splitter->output_items[o].size; i++) {
      Item item = DEQUE_AT(splitter->output_items[o], i);
      callback(user, item.value,
               item_position(splitter->position,
                             0.5F + item.distance_from_start,
                             item.source_orientation, o));
    }
  }
}
