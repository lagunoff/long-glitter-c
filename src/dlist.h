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
dlist_delete_iter_fixup(
  dlist_iter_t *iter,
  dlist_head_t *head,
  dlist_node_t *deleted
) {
  if (iter->next == deleted) iter->next = deleted->prev ? deleted->prev : head->first;
}
