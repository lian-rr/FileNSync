#include <stdio.h>
#include <assert.h>
#include "arraylist.h"

int main(void)
{
    printf("\n========================================\n");
    printf("Testing default initialization...\n");

    struct ArrayList *al = create_arraylist();
    assert(al);
    assert(al->data);
    assert(al->lenght == 0);
    assert(al->_capacity == 10);

    printf("Default initialization successful...\n");
    printf("========================================\n\n");

    printf("Testing insertion...\n");
    int tmp = 100;
    arraylist_add(al, &tmp);
    assert(al);
    assert(al->lenght == 1);
    assert(arraylist_get_size(al) == al->lenght);
    assert(al->_capacity == 10);
    assert((int *)arraylist_get(al, 0) == (int *)al->data[0]);
    assert((int *)arraylist_get(al, 0) == &tmp);

    printf("Insert successful...\n");
    printf("========================================\n\n");

    printf("Testing arraylist destruction...\n");

    arraylist_destroy(al);

    printf("Arraylist destruction successful...\n");
    printf("========================================\n\n");

    printf("Testing convenience methods...\n");

    struct ArrayList *al2 = create_arraylist();
    assert(arraylist_get_size(al2) == 0);

    arraylist_addInteger(al2, 10);
    arraylist_addInteger(al2, 20);
    arraylist_addInteger(al2, 30);

    assert(arraylist_get_size(al2) == 3);
    assert(al2->_capacity == 10);

    printf("%d\n", arraylist_getInteger(al2, 0));
    printf("%d\n", *(int *)(al2->data[0]));
    printf("%d\n", *(int *)arraylist_get(al2, 0));
    assert(arraylist_getInteger(al2, 0) == 10);
    assert(arraylist_getInteger(al2, 1) == 20);
    assert(arraylist_getInteger(al2, 2) == 30);

    printf("Convenience methods successful...\n");
    printf("========================================\n\n");

    arraylist_destroy(al2);
}