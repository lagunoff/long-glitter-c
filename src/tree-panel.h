#pragma once
#include "draw.h"
#include "widget.h"

typedef struct tree_t {
  enum {
    Tree_File,
    Tree_Directory,
  } tag;
  union {
    struct {
      char       *path;
      struct stat st;
    } file;
    struct {
      char          *path;
      int            len;
      struct tree_t *contents;
      enum {
        Tree_Closed,
        Tree_Expanded,
      } state;
    } directory;
  };
} tree_t;

typedef struct {
  widget_context_t ctx;
  char            *path;
  tree_t           tree;
} tree_panel_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      TreePanel_DirectoryToggle = MSG_LAST,
    } tag;
    union {
      char *directory_toggle;
    };
  };
} tree_panel_msg_t;

void tree_panel_init(tree_panel_t *self, widget_context_init_t *ctx, char *path);
void tree_panel_free(tree_panel_t *self);
void tree_panel_dispatch(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield);
