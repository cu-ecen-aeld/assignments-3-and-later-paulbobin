#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: finder <directory> <searchstring>\n");
        return 1;
    }

    char *dirpath = argv[1];
    char *search = argv[2];

    DIR *d = opendir(dirpath);
    if (d == NULL)
    {
        printf("Error: %s is not a directory\n", dirpath);
        return 1;
    }

    struct dirent *entry;
    int filecount = 0;
    int matchcount = 0;

    while ((entry = readdir(d)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[500];
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        struct stat s;
        stat(filepath, &s);
        if (S_ISDIR(s.st_mode))
            continue;

        FILE *f = fopen(filepath, "r");
        if (f == NULL) continue;

        filecount++;

        char line[500];
        while (fgets(line, sizeof(line), f))
        {
            if (strstr(line, search) != NULL)
                matchcount++;
        }

        fclose(f);
    }

    closedir(d);

    printf("The number of files are %d and the number of matching lines are %d\n",
            filecount, matchcount);

    return 0;
}

