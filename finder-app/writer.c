#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    openlog(NULL,0,LOG_USER);

    if(argc != 3)
    {
        syslog(LOG_ERR, "Invalid number of arguments");
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];
    char *dir = dirname(strdup(writefile));

    struct stat s;
    if(stat(dir, &s) != 0)
        mkdir(dir, 0777);

    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
    {
        syslog(LOG_ERR, "Error opening file %s", writefile);
        return 1;
    }

    if(write(fd, writestr, strlen(writestr)) != (ssize_t)strlen(writestr))
    {
        syslog(LOG_ERR, "Error writing string");
        close(fd);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    close(fd);
    closelog();
    return 0;
}

