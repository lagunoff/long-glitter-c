#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "autocomplete.h"
#include "dlist.h"
#include "utils.h"

void autocomplete_list_init_filename(const char *path, menulist_item_t **items, int *len) {
  *len = 0;
  *items = NULL;
  auto menulist_item_t *go();
  __auto_type fd = open(path, O_RDONLY); if (!fd) return;
  struct stat st; fstat(fd, &st);
  if (!S_ISDIR(st.st_mode)) return;
  DIR *dp = opendir(path);
  struct dirent *ep;
  go();
  closedir(dp);

  menulist_item_t *go() {
    while (ep = readdir(dp)) {
      if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) continue;
      (*len)++;
      __auto_type title = malloc(strlen(ep->d_name) + 1);
      strcpy(title, ep->d_name);
      __auto_type item_ptr = go();
      *item_ptr = (menulist_item_t){{Widget_Rectangle,{0,0}}, title, false, 0, 0};
      return item_ptr - 1;
    }
    // Reached the end of directory contents
    *items = malloc(sizeof(menulist_item_t) * (*len));
    return &((*items)[(*len) - 1]);
  }
}

void autocomplete_list_free(menulist_item_t **items, int *len) {
  if (*items == NULL) return;
  for(int i = 0; i < *len; i++) {
    free(((*items)[i]).title);
  }
  free(*items);
  *items = NULL;
}
