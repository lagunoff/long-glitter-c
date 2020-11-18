#pragma once
#include "draw.h"
#include "widget.h"
#include "buffer.h"

typedef struct buffer_list_node_t {
  struct buffer_list_node_t *next;
  struct buffer_list_node_t *prev;
  int      key;
  buffer_t buffer;
} buffer_list_node_t;

typedef struct {
  buffer_list_node_t *first;
  buffer_list_node_t *last;
} buffer_list_t;

typedef struct {
  widget_context_t    ctx;
  buffer_list_t       tabs;
  buffer_list_node_t *active; // @Nullable
  int                 last_key;
  rect_t              tabs_clip;
} tabs_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      TABS_ACTIVE = MSG_LAST,
      TABS_KEY,
    } tag;
    union {
      buffer_msg_t active;
      struct {int key; buffer_msg_t msg;} key;
    };
  };
} tabs_msg_t;

void tabs_init(tabs_t *self, widget_context_init_t *ctx, char *path);
void tabs_free(tabs_t *self);
void tabs_dispatch(tabs_t *self, tabs_msg_t *msg, yield_t yield);
