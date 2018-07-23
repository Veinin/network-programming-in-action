#include <signal.h>
#include <sys/wait.h>

#include "unp.h"

int tcp_listen()
{
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("bind error");

    if (listen(listenfd, SOMAXCONN) < 0)
        errno_abort("listen error");

    return listenfd;
}

void sig_child_handle(int signo)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
}

void echo_handle(int connfd)
{
    int ret;
    char data[1024];
    
    while (1)
    {
        memset(&data, 0, sizeof(data));

        if ((ret = read(connfd, data, sizeof(data))) == -1)
            errno_abort("read error");
        else if (ret == 0)
        {
            printf("clinet disconnect\n");
            close(connfd);
            break;
        }

        printf("echo %d bytes, data receved at %s", ret, data);

        if (write(connfd, data, ret) != ret)
            errno_abort("write error");
    }
}

int main()
{
    int listenfd, connfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddrlen = sizeof(peeraddr);
    pid_t pid;

    listenfd = tcp_listen();

    signal(SIGCHLD, sig_child_handle);
    signal(SIGPIPE, SIG_IGN);

    for ( ; ; )
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