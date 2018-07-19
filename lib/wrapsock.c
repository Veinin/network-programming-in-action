#include "unp.h"

int Socket(int family, int type, int protocol)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0)
        errno_abort("socket error");

    return n;
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (bind(fd, sa, salen) < 0)
        errno_abort("bind error");
}

void Listen(int fd, int backlog)
{
    if (listen(fd, backlog) < 0)
        errno_abort("listen error");
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;

    if ((n = accept(fd, sa, salenptr)) < 0)
        errno_abort("accept error");

    return n;
}

void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0)
		errno_abort("connect error");
}

void Close(int fd)
{
    if (close(fd) < 0)
        errno_abort("close error");
}

void Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	if (getpeername(fd, sa, salenptr) < 0)
		errno_abort("getpeername error");
}

void Getsockname(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	if (getsockname(fd, sa, salenptr) < 0)
		errno_abort("getsockname error");
}

void Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(fd, level, optname, optval, optlen) < 0)
        errno_abort("setsockopt error");
}