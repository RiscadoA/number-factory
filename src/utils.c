#include "utils.h"

#include <stdlib.h>

Orientation orientation_opposite(Orientation orientation) {
  switch (orientation) {
  case NORTH:
    return SOUTH;
  case EAST:
    return WEST;
  case SOUTH:
    return NORTH;
  case WEST:
    return EAST;
  default:
    return -1;
  }
}

Vector2i orientation_vector(Orientation orientation) {
  switch (orientation) {
  case NORTH:
    return (Vector2i){0, -1};
  case EAST:
    return (Vector2i){1, 0};
  case SOUTH:
    return (Vector2i){0, 1};
  case WEST:
    return (Vector2i){-1, 0};
  default:
    return (Vector2i){0, 0};
  }
}

Orientation vector_orientation(Vector2i vector) {
  if (vector.x == 0 && vector.y == -1) return NORTH;
  if (vector.x == 1 && vector.y == 0) return EAST;
  if (vector.x == 0 && vector.y == 1) return SOUTH;
  if (vector.x == -1 && vector.y == 0) return WEST;
  return -1;
}

const char *orientation_to_string(Orientation orientation) {
  switch (orientation) {
  case NORTH:
    return "NORTH";
  case EAST:
    return "EAST";
  case SOUTH:
    return "SOUTH";
  case WEST:
    return "WEST";
  default:
    return "UNKNOWN";
  }
}

Vector2i vector_subtract(Vector2i a, Vector2i b) {
  return (Vector2i){a.x - b.x, a.y - b.y};
}

Vector2i vector_add(Vector2i a, Vector2i b) {
  return (Vector2i){a.x + b.x, a.y + b.y};
}

Vector2f vectorf_subtract(Vector2f a, Vector2f b) {
  return (Vector2f){a.x - b.x, a.y - b.y};
}

Vector2f vectorf_add(Vector2f a, Vector2f b) {
  return (Vector2f){a.x + b.x, a.y + b.y};
}

Vector2f vectorf_scale(Vector2f a, float scalar) {
  return (Vector2f){a.x * scalar, a.y * scalar};
}

Vector2f vectorf_lerp(Vector2f a, Vector2f b, float t) {
  return (Vector2f){a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
}

Vector2f vectori_to_f(Vector2i a) { return (Vector2f){(float)a.x, (float)a.y}; }

int is_position_adjacent(Vector2i pos1, Vector2i pos2) {
  return (pos1.x == pos2.x && abs(pos1.y - pos2.y) == 1) ||
         (pos1.y == pos2.y && abs(pos1.x - pos2.x) == 1);
}
