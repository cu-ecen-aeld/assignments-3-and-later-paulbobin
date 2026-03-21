#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[1024];
    int bytes;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    while(1)
    {
        client_fd = accept(server_fd, NULL, NULL);

        int file_fd = open(FILE_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);

        while((bytes = recv(client_fd, buffer, sizeof(buffer), 0)) > 0)
        {
            write(file_fd, buffer, bytes);
        }

        lseek(file_fd, 0, SEEK_SET);

        while((bytes = read(file_fd, buffer, sizeof(buffer))) > 0)
        {
            send(client_fd, buffer, bytes, 0);
        }

        close(file_fd);
        close(client_fd);
    }

    return 0;
}
