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

enum change_type
{
    created,
    modified,
    deleted
};

struct Change
{
    enum change_type type;
    struct File *file;
};

struct ArrayList *
list_dir(char *);
void open_dir();
int save_data(struct ArrayList *);
struct ArrayList *load_data();
struct ArrayList *find_differences(struct ArrayList *, struct ArrayList *);

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

struct ArrayList *
list_dir(char *dir_path)
{
    DIR *d;

    d = opendir(dir_path);

    struct dirent *dir;
    struct stat sb;
    struct ArrayList *list = create_arraylist();

    char *clean_path;
    if (dir_path[strlen(dir_path) - 1] != '/')
        clean_path = string_concat(dir_path, "/");
    else
        clean_path = dir_path;

    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp(dir->d_name, ".") == 0 ||
            strcmp(dir->d_name, "..") == 0 ||
            strcmp(dir->d_name, ".fileNsync") == 0)
            continue;

        char *file_path = string_concat(clean_path, dir->d_name);

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

struct ArrayList *find_differences(struct ArrayList *of, struct ArrayList *nf)
{
    if (nf == NULL || arraylist_size(nf) == 0)
        return of;
    if (of == NULL || arraylist_size(of) == 0)
        return nf;

    struct ArrayList *df_list = create_arraylist();

    int i, j, found;
    for (i = 0; i < arraylist_size(of); i++)
    {
        found = 0;

        struct File *f = (struct File *)arraylist_get(of, i);
        for (j = 0; j < arraylist_size(nf); j++)
        {
            struct File *f2 = (struct File *)arraylist_get(nf, j);

            if (strcmp(f->name, f2->name) == 0)
            {
                found = 1;
                //File modified
                if ((f->m_time != f2->m_time ||
                     f->size != f2->size))
                {
                    struct Change *change = malloc(sizeof(struct Change));
                    change->type = modified;
                    change->file = f2;

                    arraylist_add(&df_list, change);
                    break;
                }
            }
        }
        //File deleted
        if (!found)
        {
            struct Change *change = malloc(sizeof(struct Change));
            change->type = deleted;
            change->file = f;
            arraylist_add(&df_list, change);
        }
    }

    for (i = 0; i < arraylist_size(nf); i++)
    {
        found = 0;

        struct File *f = (struct File *)arraylist_get(nf, i);

        for (j = 0; j < arraylist_size(df_list); j++)
        {
            if (strcmp(f->name, ((struct Change *)arraylist_get(df_list, j))->file->name) == 0)
            {
                found = 1;
                break;
            }
        }

        if (found)
            continue;

        for (j = 0; j < arraylist_size(of); j++)
        {
            if (strcmp(f->name, ((struct File *)arraylist_get(of, j))->name) == 0)
            {
                found = 1;
                break;
            }
        }

        if (found)
            continue;

        //new file
        struct Change *change = malloc(sizeof(struct Change));
        change->type = created;
        change->file = f;
        arraylist_add(&df_list, change);
    }

    return df_list;
}