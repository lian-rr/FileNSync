#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "filehistory.h"

struct ArrayList *history;
struct ArrayList *files;
struct ArrayList *changes;

void update_local_history(char *dir);

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    printf("\n====================================\n");
    printf("Updating local history...");
    printf("\n------------------------------------\n\n");

    update_local_history(argv[1]);

    printf("\n====================================\n");
    printf("Initializing server...");
    printf("\n------------------------------------\n\n");

    return 0;
}

void update_local_history(char *dir)
{
    open_dir();

    history = load_data();

    printf("Reading files in: %s\n", dir);

    files = list_dir(dir);

    changes = find_differences(history, files);
    int i;
    for (i = 0; i < arraylist_size(changes); i++)
    {
        struct Change *change = (struct Change *)arraylist_get(changes, i);
        printf("File named: %s was %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
    }

    save_data(files);
}
