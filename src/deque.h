// Before including this file, define:
// - DEQUE_ELEMENT_TYPE to the type of elements you want to store in the deque
// - DEQUE_FUNCTION_PREFIX to the prefix you want to use for the deque functions
// - DEQUE_IMPLEMENT if you want functions to be defined instead of declared

#ifndef DEQUE_AT
#define DEQUE_AT(deque, index)                                                 \
  (deque).buffer[((deque).first + (index)) % (deque).capacity]
#endif

#ifndef DEQUE_ELEMENT_TYPE
#error "Define DEQUE_ELEMENT_TYPE before including deque.h"
#endif

#ifndef DEQUE_FUNCTION_PREFIX
#error "Define DEQUE_FUNCTION_PREFIX before including deque.h"
#endif

#define DEQUE_CONCAT_AUX(LEFT, RIGHT) LEFT##RIGHT
#define DEQUE_CONCAT(LEFT, RIGHT) DEQUE_CONCAT_AUX(LEFT, RIGHT)

#define DEQUE_TYPE DEQUE_CONCAT(DEQUE_ELEMENT_TYPE, Deque)

#define DEQUE_INIT_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _init)
#define DEQUE_FREE_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _free)
#define DEQUE_RESERVE_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _reserve)
#define DEQUE_PUSH_FRONT_FUNCTION                                              \
  DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _push_front)
#define DEQUE_PUSH_BACK_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _push_back)
#define DEQUE_POP_FRONT_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _pop_front)
#define DEQUE_POP_BACK_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _pop_back)
#define DEQUE_FRONT_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _front)
#define DEQUE_BACK_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _back)
#define DEQUE_CLEAR_FUNCTION DEQUE_CONCAT(DEQUE_FUNCTION_PREFIX, _clear)

#ifndef DEQUE_IMPLEMENT

typedef struct {
  DEQUE_ELEMENT_TYPE *buffer;
  int first;
  int size;
  int capacity;
} DEQUE_TYPE;

void DEQUE_INIT_FUNCTION(DEQUE_TYPE *deque, int capacity);
void DEQUE_FREE_FUNCTION(DEQUE_TYPE *deque);
void DEQUE_RESERVE_FUNCTION(DEQUE_TYPE *deque, int extra_capacity);
void DEQUE_PUSH_FRONT_FUNCTION(DEQUE_TYPE *deque, DEQUE_ELEMENT_TYPE value);
void DEQUE_PUSH_BACK_FUNCTION(DEQUE_TYPE *deque, DEQUE_ELEMENT_TYPE value);
DEQUE_ELEMENT_TYPE DEQUE_POP_FRONT_FUNCTION(DEQUE_TYPE *deque);
DEQUE_ELEMENT_TYPE DEQUE_POP_BACK_FUNCTION(DEQUE_TYPE *deque);
DEQUE_ELEMENT_TYPE DEQUE_FRONT_FUNCTION(DEQUE_TYPE *deque);
DEQUE_ELEMENT_TYPE DEQUE_BACK_FUNCTION(DEQUE_TYPE *deque);
void DEQUE_CLEAR_FUNCTION(DEQUE_TYPE *deque);

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

void DEQUE_INIT_FUNCTION(DEQUE_TYPE *deque, int capacity) {
  deque->buffer = malloc(capacity * sizeof(DEQUE_ELEMENT_TYPE));
  deque->first = 0;
  deque->size = 0;
  deque->capacity = capacity;
}

void DEQUE_FREE_FUNCTION(DEQUE_TYPE *deque) {
  free(deque->buffer);
  deque->buffer = NULL;
  deque->first = 0;
  deque->size = 0;
  deque->capacity = 0;
}

void DEQUE_RESERVE_FUNCTION(DEQUE_TYPE *deque, int extra_capacity) {
  if (extra_capacity <= 0) {
    return;
  }

  int wrapped_count = MAX(0, deque->first + deque->size - deque->capacity);
  int moved_count = MIN(wrapped_count, deque->size);
  int old_capacity = deque->capacity;

  deque->capacity += extra_capacity;
  deque->buffer =
      realloc(deque->buffer, deque->capacity * sizeof(DEQUE_ELEMENT_TYPE));

  if (moved_count > 0) {
    memmove(deque->buffer + old_capacity, deque->buffer,
            moved_count * sizeof(DEQUE_ELEMENT_TYPE));
  }
}

void DEQUE_PUSH_FRONT_FUNCTION(DEQUE_TYPE *deque, DEQUE_ELEMENT_TYPE value) {
  if (deque->size == deque->capacity) {
    DEQUE_RESERVE_FUNCTION(deque, deque->capacity);
  }
  deque->first = (deque->first - 1 + deque->capacity) % deque->capacity;
  deque->buffer[deque->first] = value;
  deque->size++;
}

void DEQUE_PUSH_BACK_FUNCTION(DEQUE_TYPE *deque, DEQUE_ELEMENT_TYPE value) {
  if (deque->size == deque->capacity) {
    DEQUE_RESERVE_FUNCTION(deque, deque->capacity);
  }
  deque->buffer[(deque->first + deque->size) % deque->capacity] = value;
  deque->size++;
}

DEQUE_ELEMENT_TYPE DEQUE_POP_FRONT_FUNCTION(DEQUE_TYPE *deque) {
  if (deque->size == 0) {
    fprintf(stderr, "deque is empty\n");
    exit(1);
  }
  DEQUE_ELEMENT_TYPE value = deque->buffer[deque->first];
  deque->first = (deque->first + 1) % deque->capacity;
  deque->size--;
  return value;
}

DEQUE_ELEMENT_TYPE DEQUE_POP_BACK_FUNCTION(DEQUE_TYPE *deque) {
  if (deque->size == 0) {
    fprintf(stderr, "deque is empty\n");
    exit(1);
  }
  DEQUE_ELEMENT_TYPE value =
      deque->buffer[(deque->first + deque->size - 1) % deque->capacity];
  deque->size--;
  return value;
}

DEQUE_ELEMENT_TYPE DEQUE_FRONT_FUNCTION(DEQUE_TYPE *deque) {
  if (deque->size == 0) {
    fprintf(stderr, "deque is empty\n");
    exit(1);
  }
  return deque->buffer[deque->first];
}

DEQUE_ELEMENT_TYPE DEQUE_BACK_FUNCTION(DEQUE_TYPE *deque) {
  if (deque->size == 0) {
    fprintf(stderr, "deque is empty\n");
    exit(1);
  }
  return deque->buffer[(deque->first + deque->size - 1) % deque->capacity];
}

void DEQUE_CLEAR_FUNCTION(DEQUE_TYPE *deque) {
  deque->first = 0;
  deque->size = 0;
}
#endif

#undef DEQUE_CONCAT_AUX
#undef DEQUE_CONCAT
#undef DEQUE_TYPE

#undef DEQUE_INIT_FUNCTION
#undef DEQUE_FREE_FUNCTION
#undef DEQUE_RESERVE_FUNCTION
#undef DEQUE_PUSH_FUNCTION
#undef DEQUE_POP_FUNCTION
#undef DEQUE_FRONT_FUNCTION
#undef DEQUE_BACK_FUNCTION
#undef DEQUE_CLEAR_FUNCTION

#undef DEQUE_ELEMENT_TYPE
#undef DEQUE_FUNCTION_PREFIX
#undef DEQUE_IMPLEMENT
