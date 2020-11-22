#pragma once
#include "draw.h"
#include "widget.h"
#include "tabs.h"
#include "tree-panel.h"


typedef struct {
  widget_context_t ctx;
  tree_panel_t     sidebar;
  tabs_t           content;
  widget_t        *focused;
  bool             show_sidebar;
} main_window_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      MainWindow_Sidebar = MSG_LAST,
      MainWindow_Content,
    } tag;
    union {
      tree_panel_msg_t sidebar;
      tabs_msg_t       content;
    };
  };
} main_window_msg_t;

void main_window_init(main_window_t *self, widget_context_init_t *ctx);
void main_window_free(main_window_t *self);
void main_window_dispatch(main_window_t *self, main_window_msg_t *msg, yield_t yield);
