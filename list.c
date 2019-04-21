#include "list.h"

#include <stdlib.h>

void push_item_back(struct list_item **list, struct list_item *item) {
    while (*list)
        list = &(*list)->next;
    *list = item;
}

void push_item_order(struct list_item **list, void *content, int (*sort)(void *x, void *y)) {
    struct list_item *item = malloc(sizeof(*item));
    *item = (struct list_item){
        .content = content,
        .next = NULL,
    };
    while (*list && (*sort)((*list)->content, item->content))
        list = &(*list)->next;

    item->next = *list;
    *list = item;
}
