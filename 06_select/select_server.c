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

void server_select_handler(int listenfd)
{
    int i, maxi, maxfd, connfd, sockfd;
    ssize_t n;
    char buf[MAX_MSG_SIZE];
    int nready, client[FD_SETSIZE];
    fd_set rset, allset;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    maxi = -1;
    maxfd = listenfd;
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for ( ; ; )
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;

            errno_abort("select error");
        }

        if (FD_ISSET(listenfd, &rset))
        {
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
                errno_abort("accept error");

            printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0)
                {
                    client[i] = connfd;
                    if (i > maxi)
                        maxi = i;
                    break;
                }

            if (i == FD_SETSIZE)
                err_quit("too many clients");

            FD_SET(connfd, &allset);

            if (connfd > maxfd)
                maxfd = connfd;

            if (--nready <= 0)
                continue;
        }

        for (i = 0; i <= maxi; i++)
        {
            sockfd = client[i];

            if (sockfd < 0)
                continue;

            if (FD_ISSET(sockfd, &rset))
            {
                memset(&buf, 0, sizeof(buf));

                if ((n = readline(sockfd, buf, MAX_MSG_SIZE)) < 0)
                    errno_abort("readline error");
                else if (n == 0)
                {
                    printf("client disconnect\n");
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
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

    server_select_handler(listenfd);

    return 0;
}