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

int input_update(Input *input, float dt, int can_output) {
  input->time_accumulator += dt;

  if (input->progress > 0.0f) {
    input->progress += dt * ITEM_SPEED;

    if (input->progress >= 1.0f) {
      if (can_output) {
        input->progress = 0.0f;
        return input->value;
      }
      input->progress = 1.0f;
    }

    return 0;
  }

  if (input->time_accumulator >= input->time_per_value) {
    input->time_accumulator -= input->time_per_value;
    input->progress = 0.00001f;
  }

  return 0;
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
