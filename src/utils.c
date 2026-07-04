#include "utils.h"

#include <stdlib.h>

int is_position_adjacent(Vector2i pos1, Vector2i pos2) {
  return (pos1.x == pos2.x && abs(pos1.y - pos2.y) == 1) ||
         (pos1.y == pos2.y && abs(pos1.x - pos2.x) == 1);
}
