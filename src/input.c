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
  input->progress = 0.0f;
}

void input_update(Input *input, int (*callback)(void *user, int value),
                  void *user, float dt) {
  input->time_accumulator += dt;

  if (input->progress > 0.0f) {
    // Multiplied by 2.0f as progress maps to half the usual distance
    input->progress += dt * ITEM_SPEED * 2.0f;

    if (input->progress >= 1.0f) {
      if (callback(user, input->value)) {
        input->progress = 0.0f;
      } else {
        input->progress = 1.0f;
      }
    }
    return;
  }

  if (input->time_accumulator >= input->time_per_value) {
    input->time_accumulator -= input->time_per_value;
    input->progress = 0.00001f;
  }
}

Vector2i input_output_position(Input *input) {
  return vector_add(input->position, orientation_vector(input->orientation));
}

Vector2f input_item_position(Input *input) {
  Vector2f start = {input->position.x + 0.5f, input->position.y + 0.5f};
  Vector2i output = input_output_position(input);
  Vector2f end = {output.x + 0.5f, output.y + 0.5f};
  Vector2f dir = vectorf_subtract(end, start);
  return vectorf_add(start, vectorf_scale(dir, 0.5f * input->progress));
}
