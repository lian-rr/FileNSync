#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

void scan_dir(char* dir_name);

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    printf("Reading file: %s\n", argv[1]);

    scan_dir(argv[1]);

    return 0;
}

void scan_dir(char *dir_name)
{
    DIR *d;
    struct dirent *dir;

    d = opendir(dir_name);

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    else
    {
        printf("Dir not open\n");
    }
}
