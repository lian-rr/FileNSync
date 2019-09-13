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

struct ArrayList *list_dir(char *);
void open_dir();
int save_data(struct ArrayList *);
struct ArrayList *load_data();

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    open_dir();

    struct ArrayList *history = load_data();

    printf("Reading files in: %s\n", argv[1]);

    struct ArrayList *files = list_dir(argv[1]);

    int i;
    for (i = 0; i < arraylist_size(history); i++)
    {
        struct File *f = (struct File *)arraylist_get(history, i);
        struct File *f2 = (struct File *)arraylist_get(files, i);
        printf("[%d]: %s (Size: %zu) %s\n", i, f->name, f->size, ctime(&(f->m_time)));
        printf("[%d]: %s (Size: %zu) %s\n", i, f2->name, f2->size, ctime(&(f2->m_time)));
    }

    save_data(files);

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

        char *file_path = string_concat(dir_path, dir->d_name);

        printf("Full path: %s\n", file_path);

        if (stat(file_path, &sb) == -1)
        {
            printf("[Warning]: Problem reading file named {%s}: %s\n", dir->d_name, strerror(errno));
            continue;
        }

        struct File *f = malloc(sizeof(struct File));
        f->name = strdup(dir->d_name);
        f->size = sb.st_size;
        f->m_time = sb.st_mtime;
        arraylist_add(&list, f);

        free(file_path);
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
        fprintf(df, "%zu\n", strlen(f->name));
        fprintf(df, "%s %zu %lu\n", f->name, f->size, (unsigned long)(f->m_time));
    }

    fclose(df);
    return 0;
}

struct ArrayList *load_data()
{
    char *localIndex = ".fileNsync/localIndex";

    struct stat sb;

    if (stat(localIndex, &sb) != -1)
    {
        size_t lenght = sb.st_size;

        FILE *df;
        df = fopen(localIndex, "rt");

        if (df == NULL)
        {
            printf("[Error]: Error opening localIndex...\n");
            return NULL;
        }

        struct ArrayList *list = create_arraylist();

        char line[512];
        size_t name_size = 0;

        int count = 0;
        while (fgets(line, 512, df))
        {
            if (count % 2 == 0)
            {
                sscanf(line, "%zu", &name_size);
            }
            else
            {
                struct File *f = malloc(sizeof(struct File));

                char *name = malloc(sizeof(char) * name_size);

                sscanf(line, "%s %zu %lu", name, &(f->size), &(f->m_time));

                f->name = strdup(name);
                free(name);

                arraylist_add(&list, f);
            }
            count++;
        }
        return list;
    }
    return NULL;
}