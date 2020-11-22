#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tree-panel.h"

void tree_init(tree_t *self, char *path);
void tree_free(tree_t *self);

void tree_panel_init(tree_panel_t *self, widget_context_init_t *ctx, char *path) {
  draw_init_context(&self->ctx, ctx);
  tree_init(&self->tree, path);
}

void tree_panel_free(tree_panel_t *self) {
  tree_free(&self->tree);
}

void tree_panel_dispatch(tree_panel_t *self, tree_panel_msg_t *msg, yield_t yield) {
  __auto_type ctx = &self->ctx;
  switch(msg->tag) {
  case Expose: {
    __auto_type path = self->tree.tag == Tree_File ? self->tree.file.path : self->tree.directory.path;
    draw_set_color(ctx, ctx->palette->default_bg);
    draw_rect(ctx, ctx->clip);
    draw_set_color(ctx, ctx->palette->secondary_text);
    draw_set_font(ctx, ctx->font);
    draw_text(ctx, ctx->clip.x + 8, ctx->clip.y + 8 + ctx->font->extents.height, path, 0);
    return;
  }
  case MSG_FREE: {
    return tree_panel_free(self);
  }
  case MotionNotify: {
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
    return;
  }
  case MSG_LAYOUT: {
    return;
  }}
}

void tree_init(tree_t *self, char *path) {
  struct stat st;
  __auto_type fd = open(path, O_RDONLY);
  fstat(fd, &st);
  if (S_ISDIR(st.st_mode)) {
    self->tag = Tree_Directory;
    self->directory.path = path;
    self->directory.state = Tree_Closed;
  } else {
    self->tag = Tree_File;
    self->file.path = path;
    self->file.st = st;
  }
}

void tree_free(tree_t *self) {
  if (self->tag == Tree_Directory && self->directory.state == Tree_Expanded) {
    for (int i = 0; i < self->directory.len; i++) {
      tree_free(&self->directory.contents[i]);
    }
    free(self->directory.contents);
  }
}
