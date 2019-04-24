#include "list.h"

#include <stdlib.h>
#include <stdbool.h>

void push_item_front(struct list_item **list, void *content) {
    struct list_item *list_item = malloc(sizeof(*list_item));
    *list_item = (struct list_item) {
        .content = content,
        .next = *list,
    };
    *list = list_item;
}

void push_item_back(struct list_item **list, void *content) {
    while (*list)
        list = &(*list)->next;
    push_item_front(list, content);
}

void push_item_order(struct list_item **list, void *content, int (*sort)(void *x, void *y)) {
    while (*list && (*sort)((*list)->content, content) < 0)
        list = &(*list)->next;
    push_item_front(list, content);
}

void *pop_item_front(struct list_item **list) {
    struct list_item *list_item = *list;
    if (!list_item)
        return NULL;

    *list = list_item->next;
    void *ret = list_item->content;
    free(list_item);

    return ret;
}

void *pop_item_back(struct list_item **list) {
    if (!*list)
        return NULL;

    while ((*list)->next)
        list = &(*list)->next;

    return pop_item_front(list);
}

bool list_contains(struct list_item **list, void *item) {
    for (; *list; list = &(*list)->next)
        if ((*list)->content == item)
            return true;
    return false;
}
