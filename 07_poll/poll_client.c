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

void client_poll_handler(int sockfd)
{
    int nready;
    ssize_t n;
    struct pollfd fds[2];

    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    fds[1].fd = fileno(stdin);
    fds[1].events = POLLOUT;

    char sendline[1024] = {0};
    char recvline[1024] = {0};

    for ( ; ; )
    {
        nready = poll(fds, 2, -1);
        if (nready == -1)
            errno_abort("poll error");

        if (nready == 0)
            continue;

        if (fds[0].revents & POLLIN)
        {
            if ((n = readline(sockfd, recvline, sizeof(recvline))) < 0)
                errno_abort("readline error");
            if (n == 0)
            {
                printf("server disconnect\n");
                break;
            }

            printf("echo %ld bytes, data receved at %s", n, recvline);
            memset(&recvline, 0, sizeof(recvline)); 
        }

        if (fds[1].revents & POLLOUT)
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

    client_poll_handler(sockfd);

    return 0;
}