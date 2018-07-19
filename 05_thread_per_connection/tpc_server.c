#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#include "unp.h"

typedef struct session_type {
    pthread_t thread_id;
    int connfd;
} session_t;

void echo_handle(int connfd)
{
    char data[1024];
    while (1)
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

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(listenfd, SOMAXCONN);

    signal(SIGPIPE, SIG_IGN);

    while (1)
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