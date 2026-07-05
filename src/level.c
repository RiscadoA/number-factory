#include "level.h"
#include "board.h"
#include "entity.h"
#include "input.h"
#include "output.h"
#include "pipe.h"
#include "utils.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  INITIAL_HEALTH = 100,
  HEALTH_PER_ITEM = 10,
};

static const float MISSED_ITEM_DURATION = 1.0f;

void level_init(Level *level) {
  board_init(&level->board, 12, 8);
  entity_pool_init(&level->entity_pool, 12 * 8);
  level->health = INITIAL_HEALTH;
  level->max_health = INITIAL_HEALTH;
  level->missed_items = NULL;
  level->missed_item_count = 0;
  level->missed_item_capacity = 0;

  // Init input
  Input input;
  input_init(&input, (Vector2i){1, 4}, EAST, 5, 1.0f);
  EntityId id = entity_create(&level->entity_pool, (Entity){
                                                       .type = ENTITY_INPUT,
                                                       .input = input,
                                                   });
  BOARD_AT(&level->board, input.position.x, input.position.y) = id;

  Output output;
  output_init(&output, (Vector2i){10, 4}, EAST, 5);
  id = entity_create(&level->entity_pool, (Entity){
                                              .type = ENTITY_OUTPUT,
                                              .output = output,
                                          });
  BOARD_AT(&level->board, output.position.x, output.position.y) = id;
}

void level_free(Level *level) {
  free(level->missed_items);
  board_free(&level->board);
  entity_pool_free(&level->entity_pool);
}

static void update_missed_items(Level *level, float dt) {
  for (int i = level->missed_item_count - 1; i >= 0; i--) {
    MissedItem *item = &level->missed_items[i];
    item->progress += dt / MISSED_ITEM_DURATION;
    if (item->progress >= 1.0f) {
      level->missed_items[i] = level->missed_items[--level->missed_item_count];
    }
  }
}

static void add_missed_item(Level *level, Input *input) {
  if (level->missed_item_count == level->missed_item_capacity) {
    int capacity =
        level->missed_item_capacity == 0 ? 4 : level->missed_item_capacity * 2;
    MissedItem *items = realloc(level->missed_items, capacity * sizeof(*items));
    if (!items) {
      SDL_Log("Failed to allocate missed-number animation");
    } else {
      level->missed_items = items;
      level->missed_item_capacity = capacity;
    }
  }

  if (level->missed_item_count < level->missed_item_capacity) {
    Vector2i direction = orientation_vector(input->orientation);
    level->missed_items[level->missed_item_count++] = (MissedItem){
        .value = input->value,
        .start = {input->position.x + 0.5f + direction.x * 0.5f,
                  input->position.y + 0.5f + direction.y * 0.5f},
        .orientation = input->orientation,
        .progress = 0.0f,
    };
  }

  level->health = MAX(0, level->health - HEALTH_PER_ITEM);
}

Vector2f level_missed_item_position(const MissedItem *item) {
  float t = MIN(1.0f, MAX(0.0f, item->progress));
  Vector2f direction = vectori_to_f(orientation_vector(item->orientation));
  Vector2f position =
      vectorf_add(item->start, vectorf_scale(direction, 1.75f * t));
  position.y -= 1.1f * 4.0f * t * (1.0f - t);
  return position;
}

static int put_item_on(Level *level, Vector2i pos, Orientation orientation,
                       int value) {
  if (!BOARD_VALID(&level->board, pos.x, pos.y)) {
    return 0;
  }
  EntityId id = BOARD_AT(&level->board, pos.x, pos.y);
  if (id == ENTITY_NONE) {
    return 0;
  }

  Entity *ent = ENTITY_AT(&level->entity_pool, id);
  switch (ent->type) {
  case ENTITY_NONE:
    return 0;
  case ENTITY_PIPE: {
    return pipe_add_item(&ent->pipe, pos, orientation, value);
  }
  case ENTITY_INPUT:
    return 0;
  case ENTITY_OUTPUT:
    if (!output_accept(&ent->output, orientation, value)) return 0;
    level->health = MIN(level->max_health, level->health + HEALTH_PER_ITEM);
    return 1;
  }
}

typedef struct {
  Level *level;
  Vector2i output_pos;
  Orientation orientation;
} OutputItemArgs;

static int output_item_callback(void *user, int value) {
  OutputItemArgs *args = (OutputItemArgs *)user;
  return put_item_on(args->level, args->output_pos, args->orientation, value);
}

void level_update(Level *level, float dt) {
  update_missed_items(level, dt);
  if (level->health <= 0) return;

  for (int i = 0; i < level->entity_pool.capacity; ++i) {
    Entity *ent = ENTITY_AT(&level->entity_pool, i);
    switch (ent->type) {
    case ENTITY_NONE:
      break;
    case ENTITY_PIPE: {
      OutputItemArgs args = {.level = level,
                             .output_pos = pipe_output_position(&ent->pipe),
                             .orientation =
                                 pipe_output_orientation(&ent->pipe)};
      pipe_update(&ent->pipe, output_item_callback, &args, dt);
      break;
    }
    case ENTITY_INPUT: {
      OutputItemArgs args = {.level = level,
                             .output_pos = input_output_position(&ent->input),
                             .orientation = ent->input.orientation};
      if (input_update(&ent->input, output_item_callback, &args, dt)) {
        add_missed_item(level, &ent->input);
      }
      break;
    }
    case ENTITY_OUTPUT:
      break;
    }
    if (level->health <= 0) break;
  }
}

int level_place_pipe(Level *level, Vector2i pos, Orientation orientation) {
  if (!BOARD_VALID(&level->board, pos.x, pos.y)) {
    return 0;
  }
  EntityId id = BOARD_AT(&level->board, pos.x, pos.y);
  if (id != ENTITY_NONE) {
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
    if (id == ENTITY_NONE) {
      continue;
    }
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

  // Entity ID that the cell was added to, if any
  EntityId current_id = ENTITY_NONE;

  // If we have an output entity, start by extending it with the current
  // position
  if (output_entity) {
    int is_start = pipe_is_start_cell(&output_entity->pipe, output_position);
    int is_aligned =
        orientation == pipe_orientation(&output_entity->pipe, output_position);

    // If the output position is a turn cell, disconnect it from any existing
    // inputs
    if (pipe_is_turn_cell(&output_entity->pipe, output_position)) {
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
      pipe_extend_input(&output_entity->pipe, pos, orientation);
      current_id = output_id;
    }
  }

  // Pick one input pipe to connect to
  Orientation picked_input;
  if (input_entities[orientation_opposite(orientation)]) {
    picked_input = orientation_opposite(orientation);
  } else if (input_count == 1) {
    for (picked_input = ORIENTATION_ZERO; picked_input < ORIENTATION_COUNT;
         ++picked_input) {
      if (input_entities[picked_input]) {
        break;
      }
    }
  } else {
    picked_input = ORIENTATION_INVALID;
  }

  // Connect to it, unless it would introduce a loop
  if (picked_input >= 0 && input_ids[picked_input] != current_id) {
    Entity *input_entity = input_entities[picked_input];
    if (current_id != ENTITY_NONE) {
      // We have a pipe already, merge the two
      Entity *current_entity = ENTITY_AT(&level->entity_pool, current_id);

      for (int i = 0; i < input_entity->pipe.cells.size; ++i) {
        Vector2i cell = DEQUE_AT(input_entity->pipe.cells, i).pos;
        BOARD_AT(&level->board, cell.x, cell.y) = current_id;
      }

      Pipe new_pipe;
      pipe_merge(&input_entity->pipe, &current_entity->pipe, &new_pipe);
      pipe_free(&input_entity->pipe);
      pipe_free(&current_entity->pipe);
      current_entity->pipe = new_pipe;
      entity_destroy(&level->entity_pool, input_ids[picked_input]);
    } else {
      pipe_extend_output(&input_entity->pipe, pos, orientation);
      current_id = input_ids[picked_input];
    }
  }

  // If we weren't able to connect into an existing pipe, create a new one
  if (current_id == ENTITY_NONE) {
    Pipe pipe;
    pipe_init(&pipe, 1);
    pipe_extend_input(&pipe, pos, orientation);
    current_id = entity_create(&level->entity_pool, (Entity){
                                                        .type = ENTITY_PIPE,
                                                        .pipe = pipe,
                                                    });
  }

  BOARD_AT(&level->board, pos.x, pos.y) = current_id;
  return 1;
}

int level_remove(Level *level, Vector2i pos) {
  EntityId id = BOARD_AT(&level->board, pos.x, pos.y);
  if (id == 0) {
    return 0;
  }

  Entity *entity = ENTITY_AT(&level->entity_pool, id);
  if (entity == NULL) {
    fprintf(stderr, "Entity not found: id=%d\n", id);
    return 0;
  }

  switch (entity->type) {
  case ENTITY_PIPE:
    BOARD_AT(&level->board, pos.x, pos.y) = ENTITY_NONE;
    if (entity->pipe.cells.size == 1) {
      entity_destroy(&level->entity_pool, id);
    } else {
      // Split the pipe into two pipes and remove this cell
      Pipe result_input, result_output;
      pipe_split(&entity->pipe, pos, &result_input, &result_output);
      pipe_contract_input(&result_output);

      // Reuse the existing pipe entity for the output side
      pipe_free(&entity->pipe);
      entity->pipe = result_output;

      // Create a new pipe entity for the input side
      if (result_input.cells.size > 0) {
        EntityId input_id =
            entity_create(&level->entity_pool, (Entity){
                                                   .type = ENTITY_PIPE,
                                                   .pipe = result_input,
                                               });
        for (int i = 0; i < result_input.cells.size; i++) {
          PipeCell cell = DEQUE_AT(result_input.cells, i);
          BOARD_AT(&level->board, cell.pos.x, cell.pos.y) = input_id;
        }
      } else {
        pipe_free(&result_input);
      }
    }
    return 1;
  default:
    return 0;
  }
}
