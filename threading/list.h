#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdbool.h>

struct list_item {
    void *content;
    struct list_item *next;
};

void push_item_front(struct list_item **list, void *content);

void push_item_back(struct list_item **list, void *content);

void push_item_order(struct list_item **list, void *content, int (*sort)(void *x, void *y));

void *pop_item_front(struct list_item **list);

void *pop_item_back(struct list_item **list);

bool list_contains(struct list_item **list, void *item);

#endif
