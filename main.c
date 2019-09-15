#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "filehistory.h"

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    open_dir();

    struct ArrayList *history = load_data();

    printf("\nReading files in: %s\n\n", argv[1]);

    struct ArrayList *files = list_dir(argv[1]);

    struct ArrayList *changes = find_differences(history, files);
    int i;
    for (i = 0; i < arraylist_size(changes); i++)
    {
        struct Change *change = (struct Change *)arraylist_get(changes, i);
        printf("File named: %s was: %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
    }

    save_data(files);

    return 0;
}