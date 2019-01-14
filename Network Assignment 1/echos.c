#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>

ssize_t writen(int fd, const void *vptr, size_t n) /* Write "n" bytes to a descriptor. */

{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;   /* and call write() again */
            else
                return (-1);    /* error */
         }

         nleft -= nwritten;
         ptr += nwritten;
    }
    return (n);
}

void str_echo(int sockfd)
{
    ssize_t n;
    char    buf[256];

  again:
    while ( (n = read(sockfd, buf, 256)) > 0)
        writen(sockfd, buf, n);

    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0){
        printf("str_echo: read error");
        exit;}
}

int main(int argc, char **argv)
{
    int     listenfd, connfd;
    pid_t   childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket (AF_INET, SOCK_STREAM, 0);

    int SERV_PORT = argv[1];
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    servaddr.sin_port = htons (SERV_PORT);

    bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

    listen(listenfd, 5);

    for ( ; ; )  {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

        if ( (childpid = fork()) == 0) { /* child process */
            close(listenfd);    /* close listening socket */
            str_echo(connfd);   /* process the request */
            exit (0);
        }
        close(connfd);          /* parent closes connected socket */
    }
}
