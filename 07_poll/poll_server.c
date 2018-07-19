#include "unp.h"

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
            connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

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
                n = Readline(sockfd, buf, MAX_MSG_SIZE);
                if (n == 0)
                {
                    printf("client disconnect\n");
                    Close(sockfd);
                    client[i].fd = -1;
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

    server_poll_handler(listenfd);

    return 0;
}