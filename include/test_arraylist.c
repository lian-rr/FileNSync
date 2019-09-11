#include <stdio.h>
#include <assert.h>
#include <string.h>
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
    arraylist_add(&al, &tmp);
    assert(al);
    assert(al->lenght == 1);
    assert(arraylist_size(al) == al->lenght);
    assert(al->_capacity == 10);
    assert((int *)arraylist_get(al, 0) == (int *)al->data[0]);
    assert((int *)arraylist_get(al, 0) == &tmp);

    printf("Insert successful...\n");
    printf("========================================\n\n");

    printf("Testing type-specific gets...\n");

    assert(arraylist_getInteger(al, 0) == 100);

    char a = 'a';
    arraylist_add(&al, &a);
    assert(arraylist_getChar(al, 1));

    float f = 1.2;
    arraylist_add(&al, &f);
    assert(arraylist_getFloat(al, 2));

    char *s = "Hello world!";
    arraylist_add(&al, s);    
    assert(strcmp(arraylist_getString(al, 3), s) == 0);

    assert(al->_capacity == 10);

    printf("Testing type-specific gets...\n");
    printf("========================================\n\n");

    printf("Testing arraylist destruction...\n");

    arraylist_destroy(al);

    printf("Arraylist destruction successful...\n");
    printf("========================================\n\n");

    printf("Testing arraylist grow...\n");

    struct ArrayList *al2 = create_arraylist_with_capacity(1);
    assert(al2);
    assert(al2->lenght == 0);
    assert(al2->_capacity == 1);

    arraylist_add(&al2, (int *)20);
    arraylist_add(&al2, (int *)30);

    assert(al2->_capacity == 2);

    arraylist_add(&al2, (int *)40);

    assert(al2->_capacity == 3);

    arraylist_add(&al2, (int *)50);

    assert(al2->_capacity == 4);

    arraylist_add(&al2, (int *)60);

    assert(al2->_capacity == 6);

    arraylist_add(&al2, (int *)70);

    assert(al2->_capacity == 6);

    arraylist_add(&al2, (int *)80);

    assert(al2->_capacity == 9);

    printf("Arraylist grow successful...\n");
    printf("========================================\n\n");
}