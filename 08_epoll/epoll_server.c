#include <sys/epoll.h>

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
    return epoll_create(1024);
}

static void fd_close(int sockfd)
{
    close(sockfd);
}

static int fd_add(int efd, int sockfd)
{
    struct epoll_event event;
    event.events = POLLIN | EPOLLET;
    event.data.fd = sockfd;
    return epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, event);
}

static void fd_delete(int efd, int sockfd)
{
    epoll_ctr(efd, EPOLL_CTL_DELETE, sockfd, NULL);
}

static int fd_nonbloking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void on_connect(int kfd, int listenfd)
{
    int j;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
        errno_abort("accept error");

    printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    if (fd_nonbloking(connfd) < 0)
        errno_abort("fd_nonbloking error");

    if (fd_add(kfd, connfd) < 0)
        errno_abort("fd_add error");
}

static void on_message(int efd, int sockfd)
{
    int n;
    char buf[MAX_MSG_SIZE] = {0};

    if ((n = readline(sockfd, buf, MAX_MSG_SIZE)) < 0)
        errno_abort("readline error");
    else if (n == 0)
    {
        printf("client disconnect\n");
        fd_delete(efd, sockfd);
        fd_close(sockfd);
    }
    else
    {
        printf("echo %ld bytes, data receved at %s", n, buf);

        if (writen(sockfd, buf, n) != n)
            errno_abort("writen error");
    }
}

static void echo_handle(int efd, int listenfd)
{
    int i, nready, sockfd;
    struct epoll_event event;
    struct epoll_event events[POLL_FD_SIZE];

    for (;;)
    {
        nready = epoll_wait(efd, &events, POLL_FD_SIZE, -1);
        if (nready < 0)
            errno_abort("epoll_wait error");

        for (i = 0; i < nready; i++)
        {
            event = events[i];
            sockfd = event.data.fd;

            if (sockfd == listenfd)
            {
                on_connect(efd, sockfd);
            }
            else
            {
                on_message(efd, sockfd);
            }
        }
    }
}

int main()
{
    int efd, listenfd;

    listenfd = tcp_listen();

    if (fd_nonbloking(listenfd) < 0)
        errno_abort("fd_nonbloking error");

    if ((efd = fd_create()) < 0)
        errno_abort("fd_create error");

    if (fd_add(efd, listenfd) < 0)
        errno_abort("fd_add error");

    echo_handle(efd, listenfd);

    return 0;
}