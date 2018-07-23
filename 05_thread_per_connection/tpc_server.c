#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#include "unp.h"

typedef struct session_type {
    pthread_t thread_id;
    int connfd;
} session_t;

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

void *thread_routine(void *arg)
{
    int status;
    session_t *session = (session_t *)arg;

    status = pthread_detach(session->thread_id);
    if (status != 0)
        err_abort(status, "Detach thread");

    echo_handle(session->connfd);

    free(session);

    return NULL;
}

int main()
{
    int listenfd, connfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddrlen = sizeof(peeraddr);
    int status;

    listenfd = tcp_listen();

    signal(SIGPIPE, SIG_IGN);

    for ( ; ; )
    {
        if ((connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddrlen)) < 0)
            errno_abort("accept error");

        printf("new connection, ip = %s, port = %d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

        session_t *session = malloc(sizeof(session_t));
        if (session == NULL)
            errno_abort("Allocate structure");

        session->connfd = connfd;

        status = pthread_create(&session->thread_id, NULL, thread_routine, session);
        if (status != 0)
            err_abort(status, "Create thread");
    }

    return 0;
}