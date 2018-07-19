#include "unp.h"

size_t writen(int fd, void *buff, size_t nbytes)
{
    size_t nwrite;
    size_t nleft = nbytes;
    char *pbuff = (char *)buff;

    while (nleft > 0)
    {
        if ((nwrite = write(fd, buff, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (nwrite == 0)
            return nbytes - nleft;

        pbuff += nwrite;
        nleft -= nwrite;
    }

    return nbytes;
}

void Writen(int fd, void *ptr, size_t nbytes)
{
    if (writen(fd, ptr, nbytes) != nbytes)
        errno_abort("writen error");
}