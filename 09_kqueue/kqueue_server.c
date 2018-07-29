#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "unp.h"

static int tcp_listen()
{
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        errno_abort("setsockopt error");

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("bind error");

    if (listen(listenfd, SOMAXCONN) < 0)
        errno_abort("listen error");

    return listenfd;
}

static int fd_create()
{
    return kqueue();
}

static void fd_close(int sockfd)
{
    close(sockfd);
}

static int fd_add(int kfd, int sockfd)
{
    struct kevent event;
    EV_SET(&event, sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    return kevent(kfd, &event, 1, NULL, 0, NULL);
}

static void fd_delete(int kfd, int sockfd)
{
    struct kevent event;
    EV_SET(&event, sockfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kfd, &event, 1, NULL, 0, NULL);
}

static void on_connect(int kfd, int listenfd, int size)
{
    int j;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    for (j = 0; j < size; j++)
    {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
            errno_abort("accept error");

        printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        if (fd_add(kfd, connfd) < 0)
            errno_abort("fd_add error");
    }
}

static void on_message(int kfd, int sockfd, int size)
{
    int n;
    char buf[MAX_MSG_SIZE] = {0};

    if ((n = recv(sockfd, buf, size, 0)) != size)
        errno_abort("recv error");
    else if (n == 0)
    {
        printf("client disconnect\n");
        fd_delete(kfd, sockfd);
        fd_close(sockfd);
    }
    else
    {
        printf("echo %d bytes, data receved at %s", size, buf);

        if (send(sockfd, buf, n, 0) != n)
            errno_abort("writen error");
    }
}

static void echo_handle(int kfd, int listenfd)
{
    int i, nready, sockfd, size;
    struct kevent events[POLL_FD_SIZE];

    for (;;)
    {
        nready = kevent(kfd, NULL, 0, events, POLL_FD_SIZE, NULL);
        if (nready < 0)
            errno_abort("kevent error");

        for (i = 0; i < nready; i++)
        {
            sockfd = events[i].ident;
            size = events[i].data;

            if (sockfd == listenfd)
            {
                on_connect(kfd, listenfd, size);
            }
            else
            {
                on_message(kfd, sockfd, size);
            }
        }
    }
}

int main()
{
    int kfd, listenfd;

    listenfd = tcp_listen();

    if ((kfd = fd_create()) < 0)
        errno_abort("fd_create error");

    if (fd_add(kfd, listenfd) < 0)
        errno_abort("fd_add error");

    echo_handle(kfd, listenfd);

    return 0;
}