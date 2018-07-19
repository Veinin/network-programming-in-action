#include <signal.h>
#include <sys/wait.h>

#include "unp.h"

void echo_handle(int connfd)
{
    char data[1024];
    while(1)
    {
        memset(&data, 0, sizeof(data));
        int ret = Read(connfd, data, sizeof(data));
        if (ret == 0)
        {
            printf("clinet disconnect\n");
            close(connfd);
            break;
        }

        printf("echo %lu bytes, data receved at %s", strlen(data), data);

        Write(connfd, data, ret);
    }
}

void sig_child_handle(int signo)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
}

int main()
{
    int listenfd, connfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddrlen = sizeof(peeraddr);
    pid_t pid;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(listenfd, SOMAXCONN);

    signal(SIGCHLD, sig_child_handle);
    signal(SIGPIPE, SIG_IGN);

    while (1)
    {
        if ((connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddrlen)) < 0)
            errno_abort("accept error");

        printf("new connection, ip = %s, port = %d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

        pid = fork();
        if (pid < 0)
            errno_abort("fork error");
        else if (pid == 0)
        {
            close(listenfd);
            echo_handle(connfd);
            exit(EXIT_SUCCESS);
        }
        else
            close(connfd);
    }

    return 0;
}