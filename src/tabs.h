#pragma once
#include "graphics.h"
#include "widget.h"
#include "buffer.h"

typedef struct buffer_list_node_t {
  struct buffer_list_node_t *next;
  struct buffer_list_node_t *prev;
  rect_t   title_clip;
  buffer_t buffer;
} buffer_list_node_t;

typedef struct {
  buffer_list_node_t *first;
  buffer_list_node_t *last;
} buffer_list_t;

typedef struct {
  widget_t      widget;
  buffer_list_t tabs;
  buffer_list_node_t *active; // @Nullable
  rect_t        tabs_clip;
  rect_t        content_clip;
} tabs_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      Tabs_Content = Widget_Last,
      Tabs_New,
      Tabs_Close,
      Tabs_TabClicked,
      Tabs_Next,
      Tabs_Prev,
    } tag;
    union {
      struct {
        buffer_list_node_t *inst;
        buffer_msg_t       *msg;
      } content;
      struct {char *path;} new;
      buffer_list_node_t  *tab_clicked;
      buffer_list_node_t  *close;
    };
  };
} tabs_msg_t;

void tabs_init(tabs_t *self, widget_context_t *ctx, char *path);
void tabs_free(tabs_t *self);
void tabs_view(tabs_t *self);
void tabs_dispatch(tabs_t *self, tabs_msg_t *msg, yield_t yield);
