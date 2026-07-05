#include "input.h"
#include "constants.h"
#include "utils.h"

void input_init(Input *input, Vector2i position, Orientation orientation,
                int value, float time_per_value) {
  input->position = position;
  input->orientation = orientation;
  input->value = value;
  input->time_per_value = time_per_value;
  input->time_accumulator = 0.0f;
  item_deque_init(&input->items, 4);
}

void input_free(Input *input) { item_deque_free(&input->items); }

Vector2i input_output_position(Input *input) {
  return vector_add(input->position, orientation_vector(input->orientation));
}

int input_update(Input *input, int (*callback)(void *user, int value),
                 void *user, float dt) {
  input->time_accumulator += dt;

  items_update(&input->items, ITEM_GAP, callback, user, dt);

  if (input->time_accumulator >= input->time_per_value) {
    input->time_accumulator -= input->time_per_value;
    if (!items_add(&input->items,
                   (Item){.value = input->value,
                          .distance_from_start = ITEM_GAP,
                          .source_orientation =
                              orientation_opposite(input->orientation)})) {
      return 1;
    }
  }
  return 0;
}

void input_for_each_item_position(Input *input,
                                  void (*callback)(void *user, int value,
                                                   Vector2f position),
                                  void *user) {
  for (int i = 0; i < input->items.size; i++) {
    Item item = DEQUE_AT(input->items, i);
    callback(user, item.value,
             item_position(input->position,
                           1.0F - ITEM_GAP + item.distance_from_start,
                           item.source_orientation, input->orientation));
  }
}
