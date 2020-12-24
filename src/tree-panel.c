#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "tree-panel.h"
#include "utils.h"
#include "dlist.h"

void tree_init(tree_t *self, char *path);
void tree_free(tree_t *self);

void tree_panel_init(tree_panel_t *self, widget_context_t *ctx, char *path) {
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    ((dispatch_t)&tree_panel_dispatch),
    NULL, NULL
  };
  self->font = &ctx->palette->default_font;
  self->tree = malloc(sizeof(tree_t));
  tree_init(self->tree, path);
}

void tree_panel_free(tree_panel_t *self) {
  tree_free(self->tree);
  free(self->tree);
}

void tree_panel_dispatch_(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield, dispatch_t next) {
  __auto_type ctx = self->widget.ctx;
  switch(msg->tag) {
  case Expose: {
    void go(int indent, int line, tree_t *tree) {
      switch(tree->tag) {
      case Tree_File: {
        __auto_type name = basename(tree->file.path);
        __auto_type color = self->widget.hover == coerce_widget(tree) ? ctx->palette->primary_text : ctx->palette->secondary_text;
        rect_t clip = {self->widget.clip.x + 8 + indent * 16, self->widget.clip.y + 8 + line * ctx->font->extents.height};
        cairo_text_extents_t extents;
        gx_set_color(ctx, color);
        gx_text(ctx, clip.x, clip.y + ctx->font->extents.ascent, name);
        gx_measure_text(ctx, name, &extents);
        clip.w = extents.x_advance;
        clip.h = extents.height;
        tree->clip = clip;
        break;
      }
      case Tree_Directory: {
        __auto_type name = basename(tree->directory.path);
        __auto_type color = self->widget.hover == coerce_widget(tree) ? ctx->palette->primary_text : ctx->palette->secondary_text;
        rect_t clip = {self->widget.clip.x + 8 + indent * 16, self->widget.clip.y + 8 + (line * ctx->font->extents.height)};
        cairo_text_extents_t extents;
        gx_set_color(ctx, color);
        gx_text(ctx, clip.x, clip.y + ctx->font->extents.ascent, name);
        gx_measure_text(ctx, name, &extents);
        clip.w = extents.x_advance;
        clip.h = extents.height;
        tree->clip = clip;
        if (tree->directory.state == Tree_Expanded) {
          int i = 0;
          for (__auto_type iter = tree->directory.items.first; iter; iter = iter->next){
            go(indent + 1, line + ++i, &iter->item);
          }
        }
        break;
      }}
    }
    gx_set_font(ctx, self->font);
    gx_set_color(ctx, ctx->palette->default_bg);
    gx_rect(ctx, self->widget.clip);
    return go(0, 0, self->tree);
  }
  case Widget_Free: {
    return tree_panel_free(self);
  }
  case MotionNotify: {
    __auto_type motion = &msg->widget.x_event->xmotion;
    tree_t *go(tree_t *tree) {
      switch(tree->tag) {
      case Tree_File: {
        if (is_inside_xy(tree->clip, motion->x, motion->y)) {
          return tree;
        }
        break;
      }
      case Tree_Directory: {
        if (is_inside_xy(tree->clip, motion->x, motion->y)) {
          return tree;
        }
        if (tree->directory.state == Tree_Expanded) {
          for (__auto_type iter = tree->directory.items.first; iter; iter = iter->next){
            __auto_type found = go(&iter->item);
            if (found) return found;
          }
        }
        break;
      }}
      return NULL;
    }
    __auto_type new_hover = go(self->tree);
    if (coerce_widget(new_hover) != self->widget.hover) {
      self->widget.hover = coerce_widget(new_hover);
      return yield(&msg_view);
    }
    return;
  }
  case ButtonPress: {
    __auto_type button = &msg->widget.x_event->xbutton;
    if (self->widget.hover) {
      return yield(&(tree_panel_msg_t){.tag = TreePanel_ItemClicked, .item_clicked = (tree_t *)self->widget.hover});
    }
    return;
  }
  case KeyPress: {
    return;
  }
  case SelectionRequest: {
    return;
  }
  case SelectionNotify: {
    return;
  }
  case TreePanel_ItemClicked: {
    switch (msg->item_clicked->tag) {
    case Tree_Directory: {
      if (msg->item_clicked->directory.state == Tree_Closed) {
        DIR *dp = opendir(msg->item_clicked->directory.path);
        struct dirent *ep;
        while (ep = readdir(dp)) {
          if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) continue;
          __auto_type new_item = (directory_list_node_t *)malloc(sizeof(directory_list_node_t));
          __auto_type new_path_len = strlen(msg->item_clicked->directory.path) + strlen("/") + strlen(ep->d_name) + 1;
          char new_path[new_path_len];
          strcpy(new_path, msg->item_clicked->directory.path);
          strcat(new_path, "/");
          strcat(new_path, ep->d_name);
          tree_init(&new_item->item, new_path);
          dlist_insert_after((dlist_head_t *)&msg->item_clicked->directory.items, (dlist_node_t *)new_item, (dlist_node_t *)msg->item_clicked->directory.items.last);
        }
        closedir(dp);
        msg->item_clicked->directory.state = Tree_Expanded;
      } else {
        for (__auto_type iter = msg->item_clicked->directory.items.first; iter;){
          __auto_type next_iter = iter->next;
          tree_free(&iter->item);
          free(iter);
          iter = next_iter;
        }
        msg->item_clicked->directory.state = Tree_Closed;
        msg->item_clicked->directory.items.first = NULL;
        msg->item_clicked->directory.items.last = NULL;
      }
      return yield(&msg_view);
    }}
    return;
  }
  case TreePanel_Up: {
    char *path = self->tree->path;
    char parent_path[strlen(path) + 1];
    parent_path[dirname(path, parent_path)] = '\0';
    self->tree = malloc(sizeof(tree_t));
    tree_init(self->tree, parent_path);
    // FIXME(leak): mount previous tree to the new root
    return yield(&msg_view);
  }
  case Widget_Layout: {
    return;
  }}
  return next(self, msg, yield);
}

void tree_panel_dispatch(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield) {
  void step_1(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield) {
    return sync_container(self, msg, yield, &noop_dispatch);
  }
  return tree_panel_dispatch_(self, msg, yield, (dispatch_t)&step_1);
}

void tree_init(tree_t *self, char *path) {
  struct stat st;
  __auto_type fd = open(path, O_RDONLY);
  __auto_type own_path = (char *)malloc(strlen(path) + 1);
  strcpy(own_path, path);
  fstat(fd, &st);
  self->widget_tag = Widget_Rectangle;
  self->clip = (rect_t){0,0,0,0};
  if (S_ISDIR(st.st_mode)) {
    self->tag = Tree_Directory;
    self->directory.path = own_path;
    self->directory.state = Tree_Closed;
    self->directory.items.first = NULL;
    self->directory.items.last = NULL;
  } else {
    self->tag = Tree_File;
    self->file.path = own_path;
    self->file.st = st;
  }
}

void tree_free(tree_t *self) {
  switch(self->tag) {
  case Tree_File: {
    free(self->file.path);
    break;
  }
  case Tree_Directory: {
    if (self->directory.state == Tree_Expanded) {
      for (__auto_type iter = self->directory.items.first; iter;){
        __auto_type next_iter = iter->next;
        tree_free(&iter->item);
        free(iter);
        iter = next_iter;
      }
    }
    free(self->directory.path);
    break;
  }}
}
