#pragma once
#include <stdbool.h>

typedef struct dlist_node_t {
  struct dlist_node_t *next;
  struct dlist_node_t *prev;
} dlist_node_t;

typedef struct {
  dlist_node_t *first;
  dlist_node_t *last;
} dlist_head_t;

typedef struct {
  dlist_node_t *next;
} dlist_iter_t;

static inline void
dlist_insert_before(
  dlist_head_t *head,
  dlist_node_t *new,
  dlist_node_t *anchor
) {
  new->next = anchor;
  if (anchor) {
    anchor->prev = new;
    new->prev = anchor->prev;
    if (anchor->prev) anchor->prev->next = new;
  } else {
    new->prev = head->last;
    if (head->last) head->last->next = new;
    head->last = new;
  }

  if (head->first == anchor) head->first = new;
}

static inline void
dlist_insert_after(
  dlist_head_t *head,
  dlist_node_t *new,
  dlist_node_t *anchor
) {
  new->prev = anchor;
  if (anchor) {
    __auto_type next = anchor->next;
    anchor->next = new;
    new->next = next;
    if (next) next->prev = new;
  } else {
    new->next = head->first;
    if (head->first) head->first->next = new;
    head->first = new;
  }

  if (head->last == anchor) head->last = new;
}

static inline void
dlist_delete(
  dlist_head_t *head,
  dlist_node_t *deleted
) {
  if (head->first == deleted) head->first = deleted->next;
  if (head->last == deleted) head->last = deleted->prev;
  if (deleted->prev) deleted->prev->next = deleted->next;
  if (deleted->next) deleted->next->prev = deleted->prev;
}

static inline void
dlist_filter(
  dlist_head_t *head,
  void (*free_node)(dlist_node_t *),
  bool (*pred)(dlist_node_t *)
) {
  __auto_type iter = head->first;
  dlist_node_t *next;
  for(; iter; iter = next) {
    next = iter ? iter->next : NULL;
    if (pred(iter) == false) {
      dlist_delete(head, iter);
      free_node(iter);
    }
  }
}

static inline void
dlist_delete_iter_fixup(
  dlist_iter_t *iter,
  dlist_head_t *head,
  dlist_node_t *deleted
) {
  if (iter->next == deleted) iter->next = deleted->prev ? deleted->prev : head->first;
}
