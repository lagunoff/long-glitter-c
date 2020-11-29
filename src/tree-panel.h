#pragma once
#include "graphics.h"
#include "widget.h"

typedef struct {
  struct directory_list_node_t *first;
  struct directory_list_node_t *last;
} directory_list_t;

typedef struct tree_t {
  enum {
    Tree_File,
    Tree_Directory,
  } tag;
  union {
    struct {
      rect_t  clip;
      char   *path;
    } common;

    struct {
      rect_t      clip;
      char       *path;
      struct stat st;
    } file;

    struct {
      rect_t           clip;
      char            *path;
      directory_list_t items;
      enum {
        Tree_Closed,
        Tree_Expanded,
      } state;
    } directory;
  };
} tree_t;

typedef struct directory_list_node_t {
  struct directory_list_node_t *next;
  struct directory_list_node_t *prev;
  struct tree_t item;
} directory_list_node_t;

typedef struct {
  widget_t widget;
  tree_t  *tree;
  tree_t  *hover;
  font_t  *font;
} tree_panel_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      TreePanel_ItemClicked = Widget_Last,
      TreePanel_Up,
    } tag;
    union {
      tree_t *item_clicked;
    };
  };
} tree_panel_msg_t;

void tree_panel_init(tree_panel_t *self, widget_context_t *ctx, char *path);
void tree_panel_free(tree_panel_t *self);
void tree_panel_dispatch(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield);
