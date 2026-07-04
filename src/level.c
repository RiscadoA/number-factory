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

  // Collect input neighbors and output neighbor
  Entity *input_entities[4];
  EntityId input_ids[4];
  Vector2i input_positions[4];
  int input_count = 0;
  Entity *output_entity = NULL;
  EntityId output_id = ENTITY_NONE;
  Vector2i output_position;
  for (Orientation i = ORIENTATION_ZERO; i < ORIENTATION_COUNT; i++) {
    input_entities[i] = NULL;
    input_ids[i] = ENTITY_NONE;

    // Check if the position is valid at all
    Vector2i position = vector_add(pos, orientation_vector(i));
    if (!BOARD_VALID(&level->board, position.x, position.y)) {
      continue;
    }

    // Check if there's a pipe entity at the neighbor position
    EntityId id = BOARD_AT(&level->board, position.x, position.y);
    Entity *ent = ENTITY_AT(&level->entity_pool, id);
    if (ent->type != ENTITY_PIPE) {
      continue;
    }

    // Get the orientation of the neighbor pipe
    Orientation neighbor_orientation = pipe_orientation(&ent->pipe, position);
    if (neighbor_orientation < 0) {
      fprintf(stderr,
              "level_place_pipe: failed to get neighbor pipe orientation\n");
      continue;
    }

    // If we're pointing at the pipe and it isn't pointing to us, it is our
    // output
    if (i == orientation &&
        neighbor_orientation != orientation_opposite(orientation)) {
      output_entity = ent;
      output_id = id;
      output_position = position;
      continue;
    }

    // If we're not pointing at the pipe and it is pointing to us, it is an
    // input
    if (i != orientation && neighbor_orientation == orientation_opposite(i)) {
      input_entities[i] = ent;
      input_ids[i] = id;
      input_positions[i] = position;
      input_count++;
      continue;
    }
  }

  // Points to the entity that the cell was added to, if any
  Entity *current_entity = NULL;

  // If we have an output entity, start by extending it with the current
  // position
  if (output_entity) {
    pipe_debug(&output_entity->pipe);

    int is_start = pipe_is_start_cell(&output_entity->pipe, output_position);
    int is_aligned =
        orientation == pipe_orientation(&output_entity->pipe, output_position);

    // If the output position is a turn cell, disconnect it from any existing
    // inputs
    if (pipe_is_turn_cell(&output_entity->pipe, output_position)) {
      fprintf(stderr, "Disconnecting turn cell at (%d, %d)\n", pos.x, pos.y);

      Pipe old_input_pipe, new_pipe;
      pipe_split(&output_entity->pipe, output_position, &old_input_pipe,
                 &new_pipe);
      pipe_free(&output_entity->pipe);
      output_entity->pipe = new_pipe;

      // Create a new entity for the old input pipe
      // Replace all cells in the board with this entity's ID
      EntityId id =
          entity_create(&level->entity_pool,
                        (Entity){.type = ENTITY_PIPE, .pipe = old_input_pipe});
      for (int i = 0; i < old_input_pipe.cells.size; ++i) {
        Vector2i cell = DEQUE_AT(old_input_pipe.cells, i).pos;
        BOARD_AT(&level->board, cell.x, cell.y) = id;
      }
    }

    // If we're the output position is the start cell of its pipe, or we're
    // aligned with its orientation, extend the pipe
    if (is_start || is_aligned) {
      fprintf(stderr, "Extending existing pipe at (%d, %d)\n", pos.x, pos.y);

      pipe_extend_input(&output_entity->pipe, pos, orientation);
      current_entity = output_entity;
      BOARD_AT(&level->board, pos.x, pos.y) = output_id;
    }
  }

  // If we have an aligned input pipe
  if (input_entities[orientation_opposite(orientation)]) {
    // TODO:  connect it to the output pipe
    fprintf(stderr, "Unimplemented\n");
    return 0;
  } else if (input_count == 1) {
    // TODO: If we have a single input pipe, connect it to the output pipe
    fprintf(stderr, "Unimplemented\n");
    return 0;
  }

  // If we weren't able to connect into an existing pipe, create a new one
  if (!current_entity) {
    fprintf(stderr, "Creating new pipe at (%d, %d)\n", pos.x, pos.y);
    Pipe pipe;
    pipe_init(&pipe, 1);
    pipe_extend_input(&pipe, pos, orientation);
    EntityId id = entity_create(&level->entity_pool, (Entity){
                                                         .type = ENTITY_PIPE,
                                                         .pipe = pipe,
                                                     });
    BOARD_AT(&level->board, pos.x, pos.y) = id;
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

  return 1;
}

int level_remove(Level *level, Vector2i pos) {
  fprintf(stderr, "Unimplemented\n");
  return 0;
}
