#include "unp.h"

int tcp_connect()
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("connect error");

    return sockfd;
}

void client_select_handler(int sockfd)
{
    int count;
    int n;
    fd_set rset;
    int nready;
    int maxfd;

    char sendline[1024] = {0};
    char recvline[1024] = {0};

    int stdinfd = fileno(stdin);
    if (stdinfd > sockfd)
        maxfd = stdinfd;
    else
        maxfd = sockfd;

    FD_ZERO(&rset);

    for ( ; ; )
    {
        FD_SET(stdinfd, &rset);
        FD_SET(sockfd, &rset);

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1)
            errno_abort("select error");

        if (nready == 0)
            continue;

        if (FD_ISSET(sockfd, &rset))
        {
            if ((n = readline(sockfd, recvline, sizeof(recvline))) < 0)
                errno_abort("readline error");
            else if (n == 0)
            {
                printf("server disconnect\n");
                break;
            }

            printf("echo %d bytes, data receved at %s", n, recvline);
            memset(&recvline, 0, sizeof(recvline)); 
        }

        if (FD_ISSET(stdinfd, &rset))
        {
            if (fgets(sendline, sizeof(sendline), stdin) == NULL)
                break;
            
            n = strlen(sendline);
            if (writen(sockfd, sendline, n) != n)
                errno_abort("writen error");

            memset(&sendline, 0, sizeof(sendline));
        }
    }
}

int main(void)
{
    int sockfd;

    sockfd = tcp_connect();

    client_select_handler(sockfd);

    return 0;
}