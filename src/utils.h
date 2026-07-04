#ifndef UTILS_H
#define UTILS_H

typedef enum {
  NORTH,
  EAST,
  SOUTH,
  WEST,
} Orientation;

typedef struct {
  float x;
  float y;
} Vector2f;

typedef struct {
  int x;
  int y;
} Vector2i;

int is_position_adjacent(Vector2i pos1, Vector2i pos2);

#endif
