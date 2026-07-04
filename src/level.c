#include "level.h"
#include "board.h"
#include "entity.h"
#include "pipe.h"
#include "utils.h"

#include <stdio.h>

void level_init(Level *level) {
  board_init(&level->board, 12, 8);
  entity_pool_init(&level->entity_pool, 12 * 8);
}

void level_free(Level *level) {
  board_free(&level->board);
  entity_pool_free(&level->entity_pool);
}

int level_place_pipe(Level *level, Vector2i pos, Orientation orientation) {
  if (!BOARD_VALID(&level->board, pos.x, pos.y)) {
    return 0;
  }
  EntityId id = BOARD_AT(&level->board, pos.x, pos.y);
  if (id != 0) {
    fprintf(stderr, "Cannot place pipe in occupied cell\n");
    return 0;
  }

  // Fetch all neighboring pipes
  EntityId neighbors[4] = {};
  Orientation neighbor_orientation[4] = {};
  for (Orientation i = ORIENTATION_ZERO; i < ORIENTATION_COUNT; i++) {
    Vector2i offset = orientation_vector(i);
    Vector2i neighbor_pos = vector_add(pos, offset);
    if (!BOARD_VALID(&level->board, neighbor_pos.x, neighbor_pos.y)) {
      neighbors[i] = ENTITY_NONE;
      continue;
    }

    neighbors[i] = BOARD_AT(&level->board, neighbor_pos.x, neighbor_pos.y);
    Entity *ent = ENTITY_AT(&level->entity_pool, neighbors[i]);
    if (ent->type != ENTITY_PIPE) {
      neighbors[i] = ENTITY_NONE;
      continue;
    }

    neighbor_orientation[i] = pipe_orientation(&ent->pipe, neighbor_pos);
  }

  // TODO: do the hard part, this is difficult...
  // need to join pipes together possibly, or create a new one
}

int level_remove(Level *level, Vector2i pos) {
  // TODO: this is also hard
  // need to split pipes possibly, or just trim
}
