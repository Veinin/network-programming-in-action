#include "unp.h"

int tcp_listen()
{
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("bind error");

    if (listen(listenfd, SOMAXCONN) < 0)
        errno_abort("listen error");

    return listenfd;
}

void server_poll_handler(int listenfd)
{
    int i, maxi, connfd, sockfd;
    ssize_t n;
    char buf[MAX_MSG_SIZE];
    int nready;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    struct pollfd client[POLL_FD_SIZE];

    for (i = 0; i < POLL_FD_SIZE; i++)
        client[i].fd = -1;

    maxi = 0;
    client[0].fd = listenfd;
    client[0].events = POLLIN;

    for ( ; ; )
    {
        nready = poll(client, maxi + 1, -1);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;

            errno_abort("poll error");
        }

        if (client[0].revents & POLLIN)
        {
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
                errno_abort("accept error");

            printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            for (i = 0; i < POLL_FD_SIZE; i++)
                if (client[i].fd < 0)
                {
                    client[i].fd = connfd;
                    client[i].events = POLLIN;
                    if (i > maxi)
                        maxi = i;
                    break;
                }

            if (i == POLL_FD_SIZE)
                err_quit("too many clients");

            if (--nready <= 0)
                continue;
        }

        for (i = 1; i <= maxi; i++)
        {
            sockfd = client[i].fd;

            if (sockfd < 0)
                continue;

            if (client[i].revents & POLLIN)
            {
                memset(&buf, 0, sizeof(buf));

                if ((n = readline(sockfd, buf, MAX_MSG_SIZE)) < 0)
                    errno_abort("readline error");
                else if (n == 0)
                {
                    printf("client disconnect\n");
                    client[i].fd = -1;
                    close(sockfd);
                }
                else
                {
                    printf("echo %ld bytes, data receved at %s", n, buf);
                    
                    if (writen(sockfd, buf, n) != n)
                        errno_abort("writen error");
                }

                if (--nready <= 0)
                    break;
            }
        }
    }
}

int main(void)
{
    int listenfd;

    listenfd = tcp_listen();

    server_poll_handler(listenfd);

    return 0;
}