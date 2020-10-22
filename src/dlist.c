#include <stdarg.h>
#include <stddef.h>

#include "dlist.h"
#include "main.h"

struct int_list {
  struct int_list *next;
  struct int_list *prev;
  int value;
};

int dlist_unittest() {
  struct dlist_head list = {NULL, NULL};
  struct int_list items[10];

  for (int i = 0; i < 10; i++) {
    items[i].value = i;
    struct dlist_node *it = (struct dlist_node *) &items[i];
    dlist_insert_before(&list, it, NULL);
  }

  struct int_list *iter = (struct int_list *) list.first;

  for (;iter; iter = iter->next) {
    inspect(%d, iter->value);
  }

  fputs("-------------\n", stderr);

  iter = (struct int_list *) list.last;
  for (;iter; iter = iter->prev) {
    inspect(%d, iter->value);
  }
}
