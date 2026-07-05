#ifndef ENTITY_H
#define ENTITY_H

#include "pipe.h"
#include "input.h"
#include "output.h"

typedef int EntityId;

typedef enum {
  // Reserved for uninitialized entities
  ENTITY_NONE = -1,
  ENTITY_PIPE,
  ENTITY_INPUT,
  ENTITY_OUTPUT,
} EntityType;

typedef struct {
  EntityType type;
  union {
    Pipe pipe;
    Input input;
    Output output;
  };
} Entity;

typedef struct {
  Entity *buffer;
  EntityId *free_list;
  int capacity;
  int free_count;
} EntityPool;

// Pool capacity is fixed, make sure it isn't possible to exceed it
void entity_pool_init(EntityPool *pool, int capacity);

void entity_pool_free(EntityPool *pool);

EntityId entity_create(EntityPool *pool, Entity entity);

void entity_destroy(EntityPool *pool, EntityId id);

#define ENTITY_AT(pool, id) (&(pool)->buffer[id])

#endif
