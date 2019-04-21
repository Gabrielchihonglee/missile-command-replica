#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

struct list_item {
    void *content;
    struct list_item *next;
};

void push_item_back(struct list_item **list, struct list_item *item);

void push_item_order(struct list_item **list, void *content, int (*sort)(void *x, void *y));

#endif
