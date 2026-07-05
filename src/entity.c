#include "entity.h"

#include <stdio.h>
#include <stdlib.h>

static void entity_free(Entity *entity) {
  switch (entity->type) {
  case ENTITY_PIPE:
    pipe_free(&entity->pipe);
    break;
  default:
    break;
  }
}

void entity_pool_init(EntityPool *pool, int capacity) {
  pool->capacity = capacity;
  pool->free_count = capacity;

  pool->buffer = malloc(capacity * sizeof(Entity));
  pool->free_list = malloc(capacity * sizeof(EntityId));

  for (int i = 0; i < capacity; i++) {
    pool->buffer[i].type = ENTITY_NONE;
    pool->free_list[i] = i;
  }
}

void entity_pool_free(EntityPool *pool) {
  for (int i = 1; i <= pool->capacity; i++) {
    entity_free(&pool->buffer[i]);
  }

  free(pool->buffer);
  free(pool->free_list);
}

EntityId entity_create(EntityPool *pool, Entity entity) {
  if (pool->free_count == 0) {
    fprintf(stderr, "Entity pool is full\n");
    return ENTITY_NONE;
  }

  EntityId id = pool->free_list[--pool->free_count];
  pool->buffer[id] = entity;
  return id;
}

void entity_destroy(EntityPool *pool, EntityId id) {
  entity_free(&pool->buffer[id]);
  pool->buffer[id].type = ENTITY_NONE;
  pool->free_list[pool->free_count++] = id;
}
