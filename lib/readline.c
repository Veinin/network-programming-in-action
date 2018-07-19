#include "unp.h"

static size_t recv_peek(int sockfd, void *buff, size_t len)
{
    while (1)
    {
        int ret = recv(sockfd, buff, len, MSG_PEEK);
        if (ret == -1 && errno == EINTR)
            continue;
        return ret;
    }
}

size_t readline(int sockfd, void *buff, size_t maxlen)
{
    int ret;
    int nread;
    char *pbuff = (char *)buff;
    int nleft = maxlen;
    while (1)
    {
        if ((ret = recv_peek(sockfd, pbuff, nleft)) < 0)
            return ret;
        else if (ret == 0)
            return ret;

        nread = ret;
        int i;
        for (i = 0; i < nread; i++)
        {
            if (pbuff[i] == '\n') //找到\n标记，直接返回
            {
                ret = readn(sockfd, pbuff, i + 1);
                if (ret != i + 1)
                    exit(EXIT_FAILURE);

                return ret;
            }
        }

        if (nread > nleft)
            exit(EXIT_FAILURE);

        //没有找到\n标记，全部读出
        nleft -= nread;
        ret = readn(sockfd, pbuff, nread);
        if (ret != nread)
            exit(EXIT_FAILURE);

        pbuff += nread;
    }

    return -1;
}

size_t Readline(int fd, void *ptr, size_t maxlen)
{
    ssize_t n;

    if ((n = readline(fd, ptr, maxlen)) < 0)
        errno_abort("readline error");
    return (n);
}