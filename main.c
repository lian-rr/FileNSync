#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

struct File
{
    char *name;    /* File Name */
    time_t c_time; /* Creation date */
    time_t m_time; /* Modification date */
    off_t st_size; /* File size */
};

void *open_dir(char *dir_path);
int dir_size(DIR *d);
void scan_dir(struct File **f_list, DIR *d);

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    printf("Reading files: %s\n", argv[1]);

    DIR *d = open_dir(argv[1]);

    if (d == NULL)
    {
        printf("\n[Error] Not possible to open the directory with name: %s\n\n", argv[1]);
        return 1;
    }

    int d_size = dir_size(d);

    printf("Directory size: %d\n", d_size);

    struct File **f_list = malloc(sizeof(struct File) * d_size);

    scan_dir(f_list, d);

    int i;
    for (i = 0; i < d_size; i++)
    {
        printf("%s\n", f_list[i]->name);
    }

    //closing dir
    closedir(d);

    return 0;
}

void *open_dir(char *dir_path)
{
    DIR *d;

    d = opendir(dir_path);

    return d ? d : NULL;
}

int dir_size(DIR *d)
{
    int size = 0;
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp(dir->d_name, ".") == 0 ||
            strcmp(dir->d_name, "..") == 0)
            continue;
        size++;
    }
    return size;
}

void scan_dir(struct File **f_list, DIR *d)
{
    int len = 0;
    struct dirent *dir;
    printf("Pre while\n");
    while ((dir = readdir(d)) != NULL)
    {
        printf("I'm alive\n");
        if (strcmp(dir->d_name, ".") == 0 ||
            strcmp(dir->d_name, "..") == 0)
            continue;

        struct File f;

        printf("File name: %s", dir->d_name);
        f.name = dir->d_name;
        printf("File name in struct: %s", f.name);

        f_list[len++] = &f;
        printf("File name in list of struct: %s", f_list[len-1]->name);
    }
}
