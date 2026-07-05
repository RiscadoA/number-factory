#include "item.h"
#include "constants.h"

#define DEQUE_ELEMENT_TYPE Item
#define DEQUE_FUNCTION_PREFIX item_deque
#define DEQUE_IMPLEMENT
#include "deque.h"

#include <math.h>

int items_add(ItemDeque *items, Item item) {
  int insert_i;
  for (insert_i = 0; insert_i < items->size; ++insert_i) {
    Item existing = DEQUE_AT(*items, insert_i);
    if (existing.distance_from_start >= item.distance_from_start - ITEM_GAP &&
        existing.distance_from_start <= item.distance_from_start + ITEM_GAP) {
      return 0;
    }

    if (existing.distance_from_start > item.distance_from_start) {
      break;
    }
  }

  item_deque_insert(items, insert_i, item);
  return 1;
}

void items_update(ItemDeque *items, float output_distance,
                  int (*on_output)(void *user, int value), void *user,
                  float dt) {
  float max_distance = INFINITY;
  int return_value = 0;

  for (int i = items->size - 1; i >= 0; --i) {
    Item *item = &DEQUE_AT(*items, i);
    item->distance_from_start += dt * ITEM_SPEED;
    item->distance_from_start = fminf(item->distance_from_start, max_distance);

    if (item->distance_from_start > output_distance) {
      if (on_output(user, item->value)) {
        item_deque_pop_back(items);
        max_distance = output_distance;
      } else {
        item->distance_from_start = output_distance;
        max_distance = item->distance_from_start - ITEM_GAP;
      }
    } else {
      max_distance = item->distance_from_start - ITEM_GAP;
    }
  }
}

Vector2f item_position(Vector2i cell, float progress, Orientation from,
                       Orientation to) {
  // Compute the cell input and output positions
  Vector2f cell_center = (Vector2f){cell.x + 0.5f, cell.y + 0.5f};
  Vector2f cell_output_dir = vectori_to_f(orientation_vector(to));
  Vector2f cell_output =
      vectorf_add(cell_center, vectorf_scale(cell_output_dir, 0.5f));
  Vector2f cell_input_dir = vectori_to_f(orientation_vector(from));
  Vector2f cell_input =
      vectorf_subtract(cell_center, vectorf_scale(cell_input_dir, 0.5f));

  // Compute the actual item position
  Vector2f out;
  if (progress < 0.5f) {
    out = vectorf_lerp(cell_input, cell_center, progress / 0.5f);
  } else {
    out = vectorf_lerp(cell_center, cell_output, (progress - 0.5f) / 0.5f);
  }
  return out;
}
