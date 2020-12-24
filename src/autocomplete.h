#pragma once

#include "input.h"

void autocomplete_list_init_filename(const char *path, menulist_item_t **items, int *len);
void autocomplete_list_free(menulist_item_t **items, int *len);
