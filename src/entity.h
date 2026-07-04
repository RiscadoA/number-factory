#ifndef ENTITY_H
#define ENTITY_H

#include "pipe.h"

typedef int EntityId;

typedef enum {
  // Reserved for uninitialized entities
  ENTITY_NONE,
  ENTITY_PIPE,
} EntityType;

typedef struct {
  EntityType type;
  union {
    Pipe pipe;
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

#endif
