#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "utils.h"

void test_concat();

int main(void)
{
    test_concat();
}

void test_concat()
{
    printf("\n========================\n");
    printf("Testing concat method\n");

    char s1[] = "Hello";
    char s3[] = "World";

    char *r1 = string_concat(s1, " ");

    assert(strcmp(r1, "Hello ") == 0);

    char *r2 = string_concat(r1, s3);

    assert(strcmp(r2, "Hello World") == 0);

    printf("Concat method working\n");
}