#include "unp.h"

void echo_handle(int connfd)
{
    int n;
    char line[1024] = {0};
    while (1)
    {
        // Read a line
        int ret = Readline(connfd, line, sizeof(line));
        if (ret == 0)
        {
            printf("client disconnect\n");
            break;
        }

        printf("echo %ld bytes, data receved at %s", strlen(line), line);

        // Write a line
        Writen(connfd, line, strlen(line));

        memset(&line, 0, sizeof(line));
    }
}

int main(void)
{
    int listenfd, connfd;
    struct sockaddr_in peeraddr;
    socklen_t peerlen = sizeof(peeraddr);

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(listenfd, SOMAXCONN);

    connfd = Accept(listenfd, (struct sockaddr *)&peeraddr, &peerlen);
    printf("new connection, ip = %s port = %d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

    echo_handle(connfd);

    return 0;
}
