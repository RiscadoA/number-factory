#ifndef UTILS_H
#define UTILS_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define DEQUE(type) struct { type *buffer; int count; int capacity; int begin; }

typedef enum {
  ORIENTATION_ZERO,

  NORTH = ORIENTATION_ZERO,
  EAST,
  SOUTH,
  WEST,

  ORIENTATION_COUNT,
} Orientation;

typedef struct {
  float x;
  float y;
} Vector2f;

typedef struct {
  int x;
  int y;
} Vector2i;

Orientation orientation_opposite(Orientation orientation);

Vector2i orientation_vector(Orientation orientation);

Orientation vector_orientation(Vector2i vector);

Vector2i vector_subtract(Vector2i a, Vector2i b);

Vector2i vector_add(Vector2i a, Vector2i b);

int is_position_adjacent(Vector2i pos1, Vector2i pos2);

#endif
