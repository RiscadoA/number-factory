#include "addition.h"
#include "constants.h"
#include "utils.h"

void addition_init(Addition *addition, Vector2i position,
                   Orientation orientation) {
  addition->position = position;
  addition->orientation = orientation;
  item_deque_init(&addition->input_items[0], 4);
  item_deque_init(&addition->input_items[1], 4);
  item_deque_init(&addition->output_items, 4);
}

void addition_free(Addition *addition) {
  item_deque_free(&addition->input_items[0]);
  item_deque_free(&addition->input_items[1]);
  item_deque_free(&addition->output_items);
}

int addition_add_item(Addition *addition, Vector2i pos, int value) {
  Vector2i primary = addition->position;
  Vector2i secondary =
      vector_add(primary, orientation_vector(addition->orientation));

  if (vector_equal(pos, primary)) {
    return items_add(
        &addition->input_items[0],
        (Item){.value = value,
               .distance_from_start = 0.0f,
               .source_orientation = addition->orientation});
  }

  if (vector_equal(pos, secondary)) {
    return items_add(
        &addition->input_items[1],
        (Item){.value = value,
               .distance_from_start = 0.0f,
               .source_orientation =
                   orientation_opposite(
                       orientation_clockwise(addition->orientation))});
  }

  return 0;
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

static int dummy_callback(void *user, int value) {
  (void)user;
  (void)value;
  return 0;
}

void addition_update(Addition *addition,
                     int (*callback)(void *user, Orientation orientation,
                                     int value),
                     void *user, float dt) {
  // Update output queue
  OutputUpdateArg arg = {
      .callback = callback,
      .user = user,
      .orientation = addition->orientation,
  };
  items_update(&addition->output_items, 0.5F, output_update_callback, &arg,
               dt);

  // Advance input queues toward the center (0.5)
  // The dummy callback returns 0, which causes items to pile up at 0.5
  items_update(&addition->input_items[0], 0.5F, dummy_callback, addition, dt);
  items_update(&addition->input_items[1], 0.5F, dummy_callback, addition, dt);

  // Check if both inputs have items waiting at the boundary
  int ready[2] = {0, 0};
  int values[2] = {0, 0};
  for (int i = 0; i < 2; i++) {
    if (addition->input_items[i].size > 0) {
      Item item =
          DEQUE_AT(addition->input_items[i], addition->input_items[i].size - 1);
      if (item.distance_from_start >= 0.5F - 0.0001F) {
        ready[i] = 1;
        values[i] = item.value;
      }
    }
  }

  if (ready[0] && ready[1]) {
    // Pop both items
    item_deque_pop_back(&addition->input_items[0]);
    item_deque_pop_back(&addition->input_items[1]);

    // Create summed output item
    items_add(&addition->output_items,
              (Item){.value = values[0] + values[1],
                     .distance_from_start = 0.0f,
                     .source_orientation =
                         orientation_opposite(addition->orientation)});
  }
}

void addition_for_each_item_position(Addition *addition,
                                     void (*callback)(void *user, int value,
                                                      Vector2f position),
                                     void *user) {
  // Input 0 items: rendered in primary cell
  for (int i = 0; i < addition->input_items[0].size; i++) {
    Item item = DEQUE_AT(addition->input_items[0], i);
    callback(user, item.value,
             item_position(addition->position, item.distance_from_start,
                           addition->orientation, addition->orientation));
  }

  // Input 1 items: rendered in secondary cell, coming from side direction
  Vector2i secondary =
      vector_add(addition->position, orientation_vector(addition->orientation));
  Orientation input1_orientation =
      orientation_opposite(orientation_clockwise(addition->orientation));
  for (int i = 0; i < addition->input_items[1].size; i++) {
    Item item = DEQUE_AT(addition->input_items[1], i);
    callback(user, item.value,
             item_position(secondary, item.distance_from_start,
                           input1_orientation, input1_orientation));
  }

  // Output items: rendered in secondary cell, moving toward output
  for (int i = 0; i < addition->output_items.size; i++) {
    Item item = DEQUE_AT(addition->output_items, i);
    callback(user, item.value,
             item_position(secondary, 0.5F + item.distance_from_start,
                           orientation_opposite(addition->orientation),
                           addition->orientation));
  }
}
