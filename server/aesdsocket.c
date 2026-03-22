/* Assignment 5 Socket*/
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
#include <errno.h>

#define DATAFILE "/var/tmp/aesdsocketdata"

int serverfd = -1;
volatile sig_atomic_t exit_requested = 0;

void signal_handler(int signo)
{
(void)signo;
syslog(LOG_INFO, "Caught signal, exiting");
exit_requested = 1;
}

void daemonize()
{
    pid_t pid = fork();
    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS);

    if(setsid() < 0)
    exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS);

    chdir("/");

    int devnull = open("/dev/null", O_RDWR);
    if(devnull >= 0){
    dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
    if(devnull > STDERR_FILENO)
            close(devnull);
    }
}

int send_datafile(int cfd)
{
    char buf[4096];
    int fd = open(DATAFILE, O_RDONLY);
    if(fd < 0) return -1;

    ssize_t nr;
    while((nr = read(fd, buf, sizeof(buf))) > 0){
        ssize_t sent = 0;
    while(sent < nr){
            ssize_t n = send(cfd, buf + sent, (size_t)(nr - sent), 0);
            if(n < 0){
            close(fd);
                return -1;
            }
        sent += n;
        }
    }
    close(fd);
    return 0;
}

void handle_client(int cfd, const char *ip)
{
    char recvbuf[1024];
    char *pkt = NULL;
    size_t pktlen = 0;

    while(1){
    ssize_t bytes = recv(cfd, recvbuf, sizeof(recvbuf), 0);

        if(bytes < 0){
        if(errno == EINTR) break;
            syslog(LOG_ERR, "recieve error from %s", ip);
        break;
        }

	if(bytes <= 0){
	    if(pktlen > 0){
		int fd = open(DATAFILE, O_CREAT|O_WRONLY|O_APPEND|O_SYNC, 0644);
		if(fd >= 0){
		    write(fd, pkt, pktlen);
		    close(fd);
		    send_datafile(cfd);
		}
	    }
	    break;
	}

        char *tmp = realloc(pkt, pktlen + (size_t)bytes);
        if(!tmp){
        syslog(LOG_ERR, "realloc failed");
            free(pkt);
        pkt = NULL;
            pktlen = 0;
        break;
        }
    pkt = tmp;
        memcpy(pkt + pktlen, recvbuf, (size_t)bytes);
    pktlen += (size_t)bytes;

        if(memchr(pkt, '\n', pktlen) || bytes == 0)
        {
        int fd = open(DATAFILE, O_CREAT|O_WRONLY|O_APPEND|O_SYNC, 0644);
            if(fd >= 0){
            write(fd, pkt, pktlen);
                close(fd);
        } else {
                syslog(LOG_ERR, "open datafile failed");
        }

        free(pkt);
            pkt = NULL;
        pktlen = 0;

            if(send_datafile(cfd) < 0)
            syslog(LOG_ERR, "send file failed for %s", ip);

        break;
        }
    }

    free(pkt);
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;
    if(argc > 1 && strcmp(argv[1], "-d") == 0)
    daemon_mode = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd < 0){
    syslog(LOG_ERR, "socket failed");
        closelog();
    return -1;
    }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9000);

    if(bind(serverfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        syslog(LOG_ERR, "bind failed");
    close(serverfd);
        closelog();
    return -1;
    }

    if(daemon_mode)
    daemonize();

    if(listen(serverfd, 5) < 0){
    syslog(LOG_ERR, "listen failed");
        close(serverfd);
    closelog();
        return -1;
    }

    while(!exit_requested){
    struct sockaddr_in client_addr;
        socklen_t clen = sizeof(client_addr);

    int cfd = accept(serverfd, (struct sockaddr *)&client_addr, &clen);
        if(cfd < 0){
        if(errno == EINTR) break;
            syslog(LOG_ERR, "accept failed");
        continue;
        }

        char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        syslog(LOG_INFO, "Accepted connection from %s", ip);

    handle_client(cfd, ip);

        close(cfd);
    syslog(LOG_INFO, "Closed connection from %s", ip);
    }

    close(serverfd);
    remove(DATAFILE);
    closelog();
    return 0;
}
