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

void tree_panel_init(tree_panel_t *self, widget_context_init_t *ctx, char *path) {
  draw_init_context(&self->ctx, ctx);
  tree_init(&self->tree, path);
  self->hover = NULL;
}

void tree_panel_free(tree_panel_t *self) {
  tree_free(&self->tree);
}

void tree_panel_dispatch(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield) {
  __auto_type ctx = &self->ctx;
  switch(msg->tag) {
  case Expose: {
    void go(int indent, int line, tree_t *tree) {
      switch(tree->tag) {
      case Tree_File: {
        __auto_type name = basename(tree->file.path);
        __auto_type color = self->hover == tree ? ctx->palette->primary_text : ctx->palette->secondary_text;
        rect_t clip = {ctx->clip.x + 8 + indent * 16, ctx->clip.y + 8 + line * ctx->font->extents.height};
        cairo_text_extents_t extents;
        draw_set_color(ctx, color);
        draw_text(ctx, clip.x, clip.y + ctx->font->extents.ascent, name);
        draw_measure_text(ctx, name, &extents);
        clip.w = extents.x_advance;
        clip.h = extents.height;
        tree->file.clip = clip;
        break;
      }
      case Tree_Directory: {
        __auto_type name = basename(tree->directory.path);
        __auto_type color = self->hover == tree ? ctx->palette->primary_text : ctx->palette->secondary_text;
        rect_t clip = {ctx->clip.x + 8 + indent * 16, ctx->clip.y + 8 + (line * ctx->font->extents.height)};
        cairo_text_extents_t extents;
        draw_set_color(ctx, color);
        draw_text(ctx, clip.x, clip.y + ctx->font->extents.ascent, name);
        draw_measure_text(ctx, name, &extents);
        clip.w = extents.x_advance;
        clip.h = extents.height;
        tree->directory.clip = clip;
        if (tree->directory.state == Tree_Expanded) {
          int i = 0;
          for (__auto_type iter = tree->directory.items.first; iter; iter = iter->next){
            go(indent + 1, line + ++i, &iter->item);
          }
        }
        break;
      }}
    }
    draw_set_font(ctx, ctx->font);
    draw_set_color(ctx, ctx->palette->default_bg);
    draw_rect(ctx, ctx->clip);
    return go(0, 0, &self->tree);
  }
  case Widget_Free: {
    return tree_panel_free(self);
  }
  case MotionNotify: {
    __auto_type motion = &msg->widget.x_event.xmotion;
    tree_t *go(tree_t *tree) {
      switch(tree->tag) {
      case Tree_File: {
        if (rect_is_inside(tree->file.clip, motion->x, motion->y)) {
          return tree;
        }
        break;
      }
      case Tree_Directory: {
        if (rect_is_inside(tree->directory.clip, motion->x, motion->y)) {
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
    __auto_type new_hover = go(&self->tree);
    if (new_hover != self->hover) {
      self->hover = new_hover;
      return yield(&msg_view);
    }
    return;
  }
  case ButtonPress: {
    __auto_type button = &msg->widget.x_event.xbutton;
    if (self->hover) {
      tree_panel_msg_t next_msg = {.tag = TreePanel_DirectoryToggle, .directory_toggle = self->hover};
      return yield(&next_msg);
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
  case TreePanel_DirectoryToggle: {
    switch (msg->directory_toggle->tag) {
    case Tree_Directory: {
      if (msg->directory_toggle->directory.state == Tree_Closed) {
        DIR *dp = opendir(msg->directory_toggle->directory.path);
        struct dirent *ep;
        while (ep = readdir(dp)) {
          if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) continue;
          __auto_type new_item = (directory_list_node_t *)malloc(sizeof(directory_list_node_t));
          __auto_type new_path_len = strlen(msg->directory_toggle->directory.path) + strlen("/") + strlen(ep->d_name) + 1;
          char new_path[new_path_len];
          strcpy(new_path, msg->directory_toggle->directory.path);
          strcat(new_path, "/");
          strcat(new_path, ep->d_name);
          tree_init(&new_item->item, new_path);
          dlist_insert_after((dlist_head_t *)&msg->directory_toggle->directory.items, (dlist_node_t *)new_item, (dlist_node_t *)msg->directory_toggle->directory.items.last);
        }
        closedir(dp);
        msg->directory_toggle->directory.state = Tree_Expanded;
      } else {
        for (__auto_type iter = msg->directory_toggle->directory.items.first; iter;){
          __auto_type next_iter = iter->next;
          tree_free(&iter->item);
          free(iter);
          iter = next_iter;
        }
        msg->directory_toggle->directory.state = Tree_Closed;
        msg->directory_toggle->directory.items.first = NULL;
        msg->directory_toggle->directory.items.last = NULL;
      }
      return yield(&msg_view);
    }}
    return;
  }
  case Widget_Layout: {
    return;
  }}
}

void tree_init(tree_t *self, char *path) {
  struct stat st;
  __auto_type fd = open(path, O_RDONLY);
  __auto_type own_path = (char *)malloc(strlen(path) + 1);
  strcpy(own_path, path);
  fstat(fd, &st);
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
