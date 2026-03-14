/* Assignment 5 Socket */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>

#define DATAFILE "/var/tmp/aesdsocketdata"

int serverfd = -1;
volatile sig_atomic_t exit_requested = 0;

void signal_handler(int signo)
{
    syslog(LOG_INFO, "Siganl Trigger, Exiting");
    exit_requested = 1;
}

void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
     }   

    if (setsid() < 0)
    {
        exit(EXIT_FAILURE);
    }   

    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }   
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        daemon_mode = 1;
    }

    openlog("aesdsocket", 0, LOG_USER);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct sockaddr_in server_addr;

    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        return -1;
    }
    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    if (bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        return -1;
    }
    if (daemon_mode)
    {
        daemonize();
	}
    if (listen(serverfd, 5) < 0)
    {
        return -1;
    }
    while (!exit_requested)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(serverfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            continue;
	}
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

        syslog(LOG_INFO, "Accepted connection from %s", ip);

        char buffer[1024];
        char *packet = NULL;
        size_t packet_size = 0;

        while (1)
        {
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0)
            {
                break;
	    }
	   char *temp = realloc(packet, packet_size + bytes);
	    if (!temp)
	    {
		 free(packet);
		 packet = NULL;
		 break;
	     }
            packet = temp;
            memcpy(packet + packet_size, buffer, bytes);
            packet_size += bytes;

            if (memchr(buffer, '\n', bytes))
            {
                int fd = open(DATAFILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
                if (fd >= 0)
                {
                    write(fd, packet, packet_size);
                    close(fd);
                }

                free(packet);
                packet = NULL;
                packet_size = 0;

                int file_fd = open(DATAFILE, O_RDONLY);
                if (file_fd >= 0)
                {
			while ((bytes = read(file_fd, buffer, sizeof(buffer))) > 0)
			{
			    ssize_t sent = 0;
			    while (sent < bytes)
			    {
				ssize_t n = send(client_fd, buffer + sent, bytes - sent, 0);
				if (n < 0)
				{
				    break;
				}
				sent += n;
			    }
			}
                    close(file_fd);
                }

                break;
            }
        }

        free(packet);
        close(client_fd);

        syslog(LOG_INFO, "Closed connection from %s", ip);
    }

    close(serverfd);
    remove(DATAFILE);
    closelog();

    return 0;
}
