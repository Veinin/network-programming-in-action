#include "unp.h"

void echo_handle(int sockfd)
{
    int n;
    package sendbuf;
    package recvbuf;

    memset(&sendbuf, 0, sizeof(sendbuf));
    memset(&recvbuf, 0, sizeof(recvbuf));

    while (fgets(sendbuf.buff, sizeof(sendbuf.buff), stdin) != NULL)
    {
        // Write a package
        n = strlen(sendbuf.buff);
        sendbuf.lenth = htons(n);
        Writen(sockfd, &sendbuf, n + 2);

        // Read a package
        int ret = Readn(sockfd, &recvbuf.lenth, 2);
        if (ret < 2)
        {
            printf("server disconnect\n");
            break;
        }
        
        n = ntohs(recvbuf.lenth);
        ret = Readn(sockfd, recvbuf.buff, n);
        if (ret == 0)
        {
            printf("server disconnect\n");
            break;
        }

        printf("echo %d bytes, data receved at %s", n, recvbuf.buff);

        memset(&sendbuf, 0, sizeof(sendbuf));
        memset(&recvbuf, 0, sizeof(recvbuf));
    }
}

int main(void)
{
    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    Connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    echo_handle(sockfd);

    return 0;
}