#include "unp.h"

void echo_handle(int sockfd)
{
    int n;
    char sendline[1024] = {0};
    char recvline[1024] = {0};

    while (fgets(sendline, sizeof(sendline), stdin) != NULL)
    {
        // Write a line
        Writen(sockfd, sendline, strlen(sendline));

        // Read a line
        int ret = Readline(sockfd, recvline, sizeof(recvline));
        if (ret == 0)
        {
            printf("server disconnect\n");
            break;
        }

        printf("echo %ld bytes, data receved at %s", strlen(recvline), recvline);

        memset(&sendline, 0, sizeof(sendline));
        memset(&recvline, 0, sizeof(recvline));
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
