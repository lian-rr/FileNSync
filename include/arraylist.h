#include <stdlib.h>
#include <stdio.h>

#ifndef __ARRAYLIST_H__
#define __ARRAYLIST_H__

#define INITIAL_CAPACITY ((size_t)10)
#define MIN_INCREMENTAL_CAPACITY ((size_t)2)

struct ArrayList
{
    size_t lenght;
    void **data;
    size_t _capacity;
};

struct ArrayList *
create_arraylist_with_capacity(size_t capacity)
{
    if (capacity <= 0)
    {
        return NULL;
    }

    struct ArrayList *list = malloc(sizeof(struct ArrayList));
    list->lenght = 0;
    list->_capacity = capacity;
    list->data = calloc(capacity, sizeof(void *));
    if (list->data == NULL)
    {
        free(list);
        return NULL;
    }

    return list;
}

struct ArrayList *
create_arraylist()
{
    return create_arraylist_with_capacity(INITIAL_CAPACITY);
}

size_t arraylist_size(struct ArrayList *arraylist)
{
    return arraylist->lenght;
}

void *arraylist_get(struct ArrayList *arraylist, int pos)
{
    if (pos >= arraylist->lenght)
    {
        return NULL;
    }

    return arraylist->data[pos];
}

struct ArrayList *
arraylist_copy(struct ArrayList *original_arraylist, size_t capacity)
{
    struct ArrayList *new_arraylist = create_arraylist_with_capacity(capacity);
    int i;
    for (i = 0; i < original_arraylist->lenght; i++)
    {
        new_arraylist->data[i] = original_arraylist->data[i];
    }
    return new_arraylist;
}

void arraylist_ensure_capacity(struct ArrayList **arraylist, int minCapacity)
{
    if (minCapacity > (*arraylist)->_capacity)
    {
        size_t old_capacity = (*arraylist)->lenght;
        size_t new_capacity = old_capacity + (old_capacity >> 1);
        if (new_capacity < MIN_INCREMENTAL_CAPACITY)
            new_capacity = MIN_INCREMENTAL_CAPACITY;
        *arraylist = arraylist_copy(*arraylist, new_capacity);
        (*arraylist)->lenght = old_capacity;
        (*arraylist)->_capacity = new_capacity;
    }
}

void arraylist_destroy(struct ArrayList *arraylist)
{
    int i;
    for (i = 0; i < arraylist->lenght; i++)
    {
        arraylist->data[i] = NULL;
        free(arraylist->data[i]);
    }
    free(arraylist->data);
    free(arraylist);
}

void arraylist_add(struct ArrayList **arraylist, void *val)
{
    arraylist_ensure_capacity(arraylist, (size_t)((*arraylist)->lenght) + 1);
    (*arraylist)->data[(*arraylist)->lenght++] = val;
}

int arraylist_is_empty(struct ArrayList *arraylist)
{
    return (arraylist == NULL || arraylist->lenght == 0);
}

int arraylist_getInteger(struct ArrayList *arraylist, int pos)
{
    return *((int *)arraylist_get(arraylist, pos));
}

float arraylist_getFloat(struct ArrayList *arraylist, int pos)
{
    return *((float *)arraylist_get(arraylist, pos));
}

char *arraylist_getString(struct ArrayList *arraylist, int pos)
{
    return (char *)arraylist_get(arraylist, pos);
}

char arraylist_getChar(struct ArrayList *arraylist, int pos)
{
    return *((char *)arraylist_get(arraylist, pos));
}

#endif