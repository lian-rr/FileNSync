#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "arraylist.h"
#include "utils.h"

struct File
{
    char *name;    /* File Name */
    time_t m_time; /* Last modification date */
    off_t size;    /* File size */
};

struct ArrayList *list_dir(char *dir_path);
void open_dir();
int save_data(struct ArrayList *files);

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    open_dir();

    // printf("Reading files in: %s\n", argv[1]);

    // struct ArrayList *files = list_dir(argv[1]);

    // int i;
    // for (i = 0; i < arraylist_size(list); i++)
    // {
    //     struct File *f = (struct File *)arraylist_get(files, i);
    //     printf("[%d]: %s (Size: %zu)", i, f->name, f->size);
    //     printf(" %s\n", ctime(&(f->m_time)));
    // }

    return 0;
}

struct ArrayList *
list_dir(char *dir_path)
{
    DIR *d;

    d = opendir(dir_path);

    struct dirent *dir;
    struct stat sb;
    struct ArrayList *list = create_arraylist();

    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp(dir->d_name, ".") == 0 ||
            strcmp(dir->d_name, "..") == 0)
            continue;

        char *file_path = string_concat(dir_path, "/");
        file_path = string_concat(file_path, dir->d_name);

        printf("Full path: %s\n", file_path);

        if (stat(file_path, &sb) == -1)
        {
            printf("[Warning]: Problem reading file named {%s}: %s\n", dir->d_name, strerror(errno));
            continue;
        }

        struct File *f = malloc(sizeof(struct File));
        f->name = dir->d_name;
        f->size = sb.st_size;
        f->m_time = sb.st_mtime;
        arraylist_add(&list, f);
    }

    closedir(d);
    return list;
}

void open_dir()
{
    struct stat sb;

    if (stat(".fileNsync", &sb) == -1)
        mkdir(".fileNsync", 0700);
}

int save_data(struct ArrayList *files)
{
    FILE *df;

    df = fopen(".fileNsync/localIndex", "w");

    if (df == NULL)
    {
        printf("[Error]: Error opening localIndex...\n");
        return -1;
    }

    int i;
    for (i = 0; i < arraylist_size(files); i++)
    {
        struct File *f = arraylist_get(files, i);
        printf("%s %zu %l", f->name, f->size, f->m_time);
    }
}