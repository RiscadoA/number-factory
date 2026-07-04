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

  Orientation opposite_orientation = orientation_opposite(orientation);

  // Fetch all neighboring pipes
  Entity *neighbors[4] = {};
  Vector2i neighbor_positions[4] = {};
  Orientation neighbor_orientations[4] = {};
  for (Orientation i = ORIENTATION_ZERO; i < ORIENTATION_COUNT; i++) {
    Vector2i offset = orientation_vector(i);
    neighbor_positions[i] = vector_add(pos, offset);
    if (!BOARD_VALID(&level->board, neighbor_positions[i].x,
                     neighbor_positions[i].y)) {
      neighbors[i] = NULL;
      continue;
    }

    EntityId neighbor_id = BOARD_AT(&level->board, neighbor_positions[i].x,
                                    neighbor_positions[i].y);
    neighbors[i] = ENTITY_AT(&level->entity_pool, neighbor_id);
    if (neighbors[i]->type != ENTITY_PIPE) {
      neighbors[i] = NULL;
      continue;
    }

    neighbor_orientations[i] =
        pipe_orientation(&neighbors[i]->pipe, neighbor_positions[i]);
  }

  // ======== Connection patterns ========
  //
  // --------- Output direction ---------
  //
  // a) Pointing at a pipe with no inputs (not pointing to us) -> connect to it
  // b) Pointing at an aligned pipe -> split target from any existing inputs,
  // connect to it
  //
  // --------- Input direction ----------
  //
  // a) Pointed to by a single pipe -> connect to it
  // b) Pointed to by multiple pipes, one of which is aligned with this pipe ->
  // connect to aligned pipe

  Pipe *result_pipe = NULL;

  // If we're placing a pipe pointing at an existing pipe, which is not pointing
  // to us
  if (neighbors[orientation] &&
      neighbor_orientations[orientation] != opposite_orientation) {
    // Check if the existing pipe has inputs
    if (pipe_is_start_or_end(&neighbors[orientation]->pipe,
                             neighbor_positions[orientation])) {
      // In this case, the target pipe has no inputs, so we can just connect to
      // it
      pipe_extend(&neighbors[orientation]->pipe, pos, orientation);
      result_pipe = &neighbors[orientation]->pipe;
    } else if (orientation == neighbor_orientations[orientation]) {
      // In this case, the target pipe points in the same direction as us, but
      // is not a start/end We need to split the target pipe from its existing
      // input, and connect to the split
      Pipe input_pipe, output_pipe;
      pipe_split(&neighbors[orientation]->pipe, neighbor_positions[orientation],
                 &input_pipe, &output_pipe);
      pipe_extend(&output_pipe, pos, orientation);

      // The now disconnected pipe entity is replaced by the input pipe
      pipe_free(&neighbors[orientation]->pipe);
      neighbors[orientation]->pipe = input_pipe;

      // We need to create a new entity for the output pipe
      EntityId id = entity_create(&level->entity_pool, (Entity){
                                                           .type = ENTITY_PIPE,
                                                           .pipe = output_pipe,
                                                       });
      neighbors[orientation] = ENTITY_AT(&level->entity_pool, id);
      BOARD_AT(&level->board, pos.x, pos.y) = id;
      result_pipe = &neighbors[orientation]->pipe;
    }
  }

  // Filter out neighbors that are not input pipes
  int input_count = 0;
  for (Orientation i = ORIENTATION_ZERO; i < ORIENTATION_COUNT; i++) {
    if (neighbors[i] && neighbor_orientations[i] == orientation_opposite(i)) {
      input_count++;
    } else {
      neighbors[i] = NULL;
    }
  }

  // If we have an aligned input pipe
  if (neighbors[opposite_orientation] &&
      neighbor_orientations[opposite_orientation] == orientation) {
    // TODO:  connect it to the output pipe
    fprintf(stderr, "Unimplemented\n");
    return 0;
  } else if (input_count == 1) {
    // TODO: If we have a single input pipe, connect it to the output pipe
    fprintf(stderr, "Unimplemented\n");
    return 0;
  }

  // Create a new pipe if we weren't able to connect to an existing one
  if (!result_pipe) {
    Pipe pipe;
    pipe_init(&pipe, 1);
    pipe_extend(&pipe, pos, orientation);
    EntityId id = entity_create(&level->entity_pool, (Entity){
                                                         .type = ENTITY_PIPE,
                                                         .pipe = pipe,
                                                     });
    BOARD_AT(&level->board, pos.x, pos.y) = id;
  }

  return 1;
}

int level_remove(Level *level, Vector2i pos) {
  fprintf(stderr, "Unimplemented\n");
  return 0;
}
