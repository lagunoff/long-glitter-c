#pragma once
#include <stdbool.h>

struct dlist_node {
  struct dlist_node *next;
  struct dlist_node *prev;
};

struct dlist_head {
  struct dlist_node *first;
  struct dlist_node *last;
};

struct dlist_iter {
  struct dlist_node **next;
};

static inline void
dlist_insert_before(
  struct dlist_head *head,
  struct dlist_node *new,
  struct dlist_node *anchor
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
  struct dlist_head *head,
  struct dlist_node *deleted
) {
  if (head->first == deleted) head->first = deleted->next;
  if (head->last == deleted) head->last = deleted->prev;
  if (deleted->prev) deleted->prev->next = deleted->next;
  if (deleted->next) deleted->next->prev = deleted->prev;
}

static inline void
dlist_insert_before_iter_fixup(
  struct dlist_iter *iter,
  struct dlist_node *new,
  struct dlist_node *anchor
) {
  if (*iter->next == anchor) iter->next = &new->next;
}

static inline void
dlist_delete_iter_fixup(
  struct dlist_iter *iter,
  struct dlist_head *head,
  struct dlist_node *deleted
) {
  if (iter->next == deleted) iter->next = deleted->prev ? &deleted->prev->next : &head->first;
}
