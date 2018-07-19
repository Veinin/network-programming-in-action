#include "unp.h"

void echo_handle(int connfd)
{
    int n;
    package recvbuf;
    while (1)
    {
        memset(&recvbuf, 0, sizeof(recvbuf));

        // Read a package
        int ret = Readn(connfd, &recvbuf.lenth, 2);
        if (ret < 2)
        {
            printf("client disconnect\n");
            break;
        }

        n = ntohs(recvbuf.lenth);
        ret = Readn(connfd, recvbuf.buff, n);
        if (ret < n)
        {
            printf("client disconnect\n");
            break;
        }

        printf("echo %d bytes, data receved at %s", n, recvbuf.buff);

        // Write a package
        Writen(connfd, &recvbuf, n + 2);
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
