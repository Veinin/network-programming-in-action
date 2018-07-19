#include "unp.h"

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

    for (;;)
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
            connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

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
                n = Readline(sockfd, buf, MAX_MSG_SIZE);
                if (n == 0)
                {
                    printf("client disconnect\n");
                    FD_CLR(sockfd, &allset);
                    Close(sockfd);
                    client[i] = -1;
                }
                else
                {
                    printf("echo %ld bytes, data receved at %s", strlen(buf), buf);
                    Writen(sockfd, &buf, strlen(buf));
                }

                if (--nready <= 0)
                    break;
            }
        }
    }
}

int main(void)
{
    int listenfd, connfd;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(listenfd, SOMAXCONN);

    server_select_handler(listenfd);

    return 0;
}