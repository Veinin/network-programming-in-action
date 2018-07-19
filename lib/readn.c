#include "unp.h"

size_t readn(int fd, void *buff, size_t size)
{
    size_t nread;
    size_t nleft = size;
    char *pbuff = (char *)buff;

    while (nleft > 0)
    {
        if ((nread = read(fd, buff, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        else if (nread == 0)
            return size - nleft;

        pbuff += nread;
        nleft -= nread;
    }

    return size;
}

size_t Readn(int fd, void *ptr, size_t nbytes)
{
    size_t n;
    if ((n = readn(fd, ptr, nbytes)) < 0)
        errno_abort("readn error");
    return n;
}