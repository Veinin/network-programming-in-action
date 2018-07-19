#include "unp.h"

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
            n = Readline(sockfd, recvline, sizeof(recvline));
            if (n == 0)
            {
                printf("server disconnect\n");
                break;
            }

            printf("echo %d bytes, data receved at %s", n, recvline);
            memset(&recvline, 0, sizeof(recvline)); 
        }

        if (fds[1].revents & POLLOUT)
        {
            if (fgets(sendline, sizeof(sendline), stdin) == NULL)
                break;

            Writen(sockfd, sendline, strlen(sendline));
            memset(&sendline, 0, sizeof(sendline));
        }
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

    client_poll_handler(sockfd);

    return 0;
}