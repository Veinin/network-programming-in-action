# I/O 复用：epoll 与 kqueue

上一节讲了，Unix 系统提供的两个I/O复用函数，`select` 和 `poll`，它们接受一个文件描述符列表，阻塞知道其中某个描述符为I/O做好准备，并返回就绪的文件描述符列表。执行这种调用的时间与文件描述符的数量成正比，这意味着它们在几千个文件描述符之上会变得非常低效。

Unix 的各种实现为上面这个问题提供了更加高效的解决方案，我们完全可以用这些拓展替换 `select` 和 `poll`，例如 `Linux` 提供的 `epoll` 和 `FreeBSD` 提供的 `kqueue` 等。虽然这些新的解决方案原理差不多，但其接口却不同，这使得它们会变得难以学习，如果想编写可以移植性的代码也会变得很困难。

## epoll 函数

`epoll` 用于可拓展的 I/O 时间通知机制，与 Linux 2.5.44 版本引入。它的功能是为了取代旧版本 `POSIX` 标准的 `select` 和 `poll` 函数，以实现应用对更高性能要求。它监听的描述符将不再受限制，它使用红黑树的数据结构来跟踪当前正在监视的所有文件描述符，且工作时间复杂度为 O(1) 。

### epoll 函数 API

1.创建 `epoll` 对象

```c
#include <sys / epoll.h>

int epoll_create(int size);
int epoll_create1(int flags);
```

该函数会创建一个 `epoll` 对象实例，该对象会占用一个文件描述符，并在创建完成后返回该描述符，该描述如应该在所有其监听的描述符关闭后再使用 `close` 函数关闭其自身返回的文件描述符。

其中 `epoll_create` 函数，参数 `size` 在旧版本的函数中是可以修改 `epoll` 的行为，其目的是给内核初始分配内部数据结构大小的一个建议。但在 Linux 2.6.8 之后该参数将被系统忽略，但 **必须大于零**。

而 `epoll_create1` 函数是 Linux内核版本2.6.27和glibc版本2.9的旧函数，它和 `epoll_create` 差不多，当 `flags` 为0时，它和 `epoll_create` 函数是一样的。另外也还可以设`flag` 的值为 `EPOLL_CLOEXEC`，会把文件描述符上设置为 `close-on-exec`，其目的是在 `fork` 子进程时执行 `exec` 的时候，清理掉父进程创建的 `socket` 套接字，防止套接字泄露给子进程。

2.控制文件描述符

```c
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

参数 `epfd` 是 `epoll_create` 函数创建的文件描述符。

参数 `op` 指定对目标文件描述符的 `fd` 的操作类型，它可以使用一下3个宏操作：

- EPOLL_CTL_ADD，把文件描述符 `fd` 注册到 `epoll` 实例中去。
- EPOLL_CTL_MOD，修改文件描述符 `fd` 关联的事件。
- EPOLL_CTL_DEL，删除（取消注册）目标文件描述符 `fd` 的事件。

参数 `event` 告诉内核对于描述符 `fd` 需要监听那些事件， `struct epoll_event` 定义如下：

```c
typedef union epoll_data {
    void        *ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;      /* Epoll events */
    epoll_data_t data;        /* User data variable */
};
```

其中 events 可以通过以下宏操作进行或运算得出要监听的事件集合：

- EPOLLIN，关联文件描述符可读操作。
- EPOLLOUT，关联文件描述符可写操作。
- EPOLLRDHUP，套接字对等方关闭连接操作，如果对等端连接断开触发的 `epoll` 事件会包含 `EPOLLIN | EPOLLRDHUP`。
- EPOLLPRI，产生带外数据。
- EPOLLERR，调用 `read` 或 `write` 函数时发生异常，如果阻塞调用 `epoll_wait` 则没必要设置。
- EPOLLHUP，挂起关联的文件描述符，如果阻塞调用 `epoll_wait` 则没必要设置。
- EPOLLET，把文件描述符设为边缘触发(Edge Triggered)模式，默认情况下 `epoll` 是水平触发(Level Triggered)。
- EPOLLONESHOT，设置关联描述符只触发一次事件，当产生一次事件后如需再次监听则需要使用 `EPOLL_CTL_MOD` 选项修改其行为以再次触发事件。

参数 `data` 指向一个联合体 `epoll_data`，该联合体用于保存调用 `epoll_ctl` 时设置的用户数据。比如你可以将一个套接字保存到 `fd` 字段中，在事件产生后，通过 `fd` 字段可以直接拿到所产生事件的套接字文件描述符。

3.等待事件触发

```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

调用该函数后将等待 `epfd` 的事件产生。

`events` 指针在函数返回后，将包含一个当已经触发的事件集合。参数 `maxevents` 可以指定返回事件最大数量，且 `maxevents` 参数在设置时必须大于0。

参数 `timeout` 将指定 `epoll_wait` 需要阻塞等待事件的超时事件（毫秒）。如果参数为 0，函数将立即返回；如果参数为-1，将永远阻塞，直到有事件产生。

### 水平触发（Level-triggered）与边缘触发（Edge-triggered）

`epoll` 接口对描述符能执行两种操作模式：水平触发（LT）与边缘触发（ET）。LT可以理解为水平触发，只要有数据可以读，不管怎样都会通知。而ET为边缘触发，只有状态发生变化时才会通知。

这两种触发机制详细描述如下：

1.LT 水平触发模式

这种模式下同时支持阻塞和非阻塞的I/O，可以使用 `EPOLLET` 来设置文件描述符。

它完全依靠内核来驱动，应用程序只需要处理从 `epoll_wait` 返回的描述符即可，在返回后即使应用程序没有任何操作，那么下次继续调用 `epoll_wait` 也还会返回上次未处理的文件描述符。

综上，这种方式出错的几率会比较小，编程的难道也更小，它和传统的 `select/poll` 就是工作方式类似。

2.ET 边缘触发模式

该模式仅支持非阻塞I/O，可以使用 `EPOLLET` 来设置文件描述符。

这种工作模式下，当 `epoll_wait` 检查文件描述符产生事件后内核只会通知一次，通知完成后，内核认为你已经知道描述符已经处于事件就绪状态了，`epoll` 将不再继续关注，下次也就就不会继续通知。

所以，对于我们的应用程序来说必须维护状态，当应用程序处于大并发的系统中时，这种模式更有优势，但同样也带来了复杂度。

### epoll 服务器实现

下面是一个使用 `epoll` 的单线程并发回射服务器程序（源文件`epoll_server.c`）：

```c
#include <sys/epoll.h>

#include "unp.h"

static int tcp_listen()
{
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        errno_abort("setsockopt error");

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("bind error");

    if (listen(listenfd, SOMAXCONN) < 0)
        errno_abort("listen error");

    return listenfd;
}

static int fd_create()
{
    return epoll_create(1024);
}

static void fd_close(int sockfd)
{
    close(sockfd);
}

static int fd_add(int efd, int sockfd)
{
    struct epoll_event event;
    event.events = POLLIN | EPOLLET;
    event.data.fd = sockfd;
    return epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, event);
}

static void fd_delete(int efd, int sockfd)
{
    epoll_ctr(efd, EPOLL_CTL_DELETE, sockfd, NULL);
}

static int fd_nonbloking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void on_connect(int kfd, int listenfd)
{
    int j;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
        errno_abort("accept error");

    printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    if (fd_nonbloking(connfd) < 0)
        errno_abort("fd_nonbloking error");

    if (fd_add(kfd, connfd) < 0)
        errno_abort("fd_add error");
}

static void on_message(int efd, int sockfd)
{
    int n;
    char buf[MAX_MSG_SIZE] = {0};

    if ((n = readline(sockfd, buf, MAX_MSG_SIZE)) < 0)
        errno_abort("readline error");
    else if (n == 0)
    {
        printf("client disconnect\n");
        fd_delete(efd, sockfd);
        fd_close(sockfd);
    }
    else
    {
        printf("echo %ld bytes, data receved at %s", n, buf);

        if (writen(sockfd, buf, n) != n)
            errno_abort("writen error");
    }
}

static void echo_handle(int efd, int listenfd)
{
    int i, nready, sockfd;
    struct epoll_event event;
    struct epoll_event events[POLL_FD_SIZE];

    for (;;)
    {
        nready = epoll_wait(efd, &events, POLL_FD_SIZE, -1);
        if (nready < 0)
            errno_abort("epoll_wait error");

        for (i = 0; i < nready; i++)
        {
            event = events[i];
            sockfd = event.data.fd;

            if (sockfd == listenfd)
            {
                on_connect(efd, sockfd);
            }
            else
            {
                on_message(efd, sockfd);
            }
        }
    }
}

int main()
{
    int efd, listenfd;

    listenfd = tcp_listen();

    if (fd_nonbloking(listenfd) < 0)
        errno_abort("fd_nonbloking error");

    if ((efd = fd_create()) < 0)
        errno_abort("fd_create error");

    if (fd_add(efd, listenfd) < 0)
        errno_abort("fd_add error");

    echo_handle(efd, listenfd);

    return 0;
}
```

首先程序针对 `epoll` 定制了一系列用于操作套接字的函数，其中每个函数都是针对 `epoll` 接口进行实现：

- `fd_create`，使用 `epoll_create` 创建文件描述符。
- `fd_close`，套接字文件描述符和 `epoll_create` 创建的文件名描述符，都可以统一使用 `close` 函数关闭。
- `fd_add`，本例中，因为只是简单的回射服务器，所有不管监听套套接字，还是已连接套接字都只需要监听 `POLLIN` 事件即可。
- `fd_delete`，使用 `EPOLL_CTL_DELETE` 选项删除事件监听。
- `fd_fd_nonbloking`，把指定套接字设为非阻塞模式。

程序刚开始调用 `tcp_listen` 函数产生一个监听套接字，然后设置为非阻塞模式。

然后，调用 `fd_create` 创建一个 epoll 文件名描述符, 并使用 `fd_add` 将监听套接字加入到 `epoll` 监听列表中。

函数 `echo_handle` 使用 `epoll_wait` 等待事件产生。
如果监听套接字 `listenfd` 产生事件，则调用 `on_connect` 函数接受新连接，并把新连接的套接字加入到 `epoll` 事件监听列表中。
如果是已连接套接字产生事件，则调用 `on_message` 函数处理消息，读取指定消息后回射给客户端，如果客户端关闭，则删除套接字的事件监听，并关闭套接字。

## kqueue 函数

kqueue 是 `FreeBSD` 上的一种 I/O 多路复用机制，它与 `epoll` 类似，也是一个可扩展的基于事件通知的编程接口，它于2000年7月在FreeBSD 4.1中引入，同时也支持NetBSD，OpenBSD，DragonflyBSD 和 MacOS。

### kqueue 函数 API

相比 `epoll` 函数来说，`kqueue` 的接口更加简洁，易用性也更好。

```c
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int kqueue(void);

int kevent(int kq, const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents,
    const struct timespec *timeout);

EV_SET(kev, ident, filter, flags, fflags, data, udata);
```

1.创建 kqueue 对象

通过 `kqueue` 函数，可以直接创建一个 `kqueue` 对象，如果失败会返回-1，如下代码：

```c
int kq;

if ((kq = kqueue()) == -1)
    errno_abort("kqueue error");
```

`kqueue` 函数会在内核中创建一个事件队列，并返回该对象的返回一个描述符，如果使用 `fork` 等函数，子进程将不会继承该队列。

2.注册、取消事件

`kqueue` 事件数据结构叫做 `kevent`，结构定义如下：

```c
struct kevent {
     uintptr_t ident;    /* 事件ID */
     short     filter;   /* 事件过滤器 */
     u_short   flags;    /* 活动标识 */
     u_int     fflags;   /* 过滤器标识 */
     intptr_t  data;     /* 过滤器数据 */
     void      *udata;   /* 应用透传数据 */
};
```

`kevent` 各个字段定义：

- ident，用于标识事件，它可以是一个描述符（文件、套接字、流）、进程ID或信号编号，这取决于你要监控的内容。
- filter，事件过滤器，用于标识内核处理事件时使用的过滤器。
- flags，活动标识，指定需要对哪些事件执行操作。
- fflags，过滤器相关标识符。
- data，过滤器相关的数据。
- udata，通过内核携带的额外用户数据，该数据不会被内核更改。

可以用以下过滤器类型（只介绍了Socket相关的过滤器）来设置事件过滤器：

- EVFILT_READ，当监听套接字在此过滤器触发时，表示有多少个套接字已经完成三次握手，可以接受连接，可以使用 `data` 字段获取已连接的套接字数量，并用 `accpt` 接受新连接；当要给已连接套接字或流接收缓冲区有数据可读时，事件被通知，并可以通过 `data` 字段获取可读数据的字节数量。
- EVFILT_WRITE，当对套接字写入数据时，事件会触发，可以通过 `data` 字段获取写缓冲区还有多少字节的空闲空间。

可以用以下选项值来标识上面的 `flags` 相关字段：

- EV_ADD，添加指定直接到 `kqueue`。
- EV_ENABLE，启用一个过滤器事件，默认情况下过滤器是可用的。
- EV_DISABLE，禁用过滤器事件，如果被禁用，当事件触发时，不会得到事件通知。
- EV_DELETE，将指定监听事件删除。
- EV_ERROR，文件描述符产生错误，当错误发生时，我们需要及时处理错误，否则 `kevent` 会不断提示该描述符错误。

我们可以通过 `EV_SET` 宏来初始化设置 `kevent` 的值：

```c
EV_SET(kev, ident, filter, flags, fflags, data, udata);
```

在 `kevent` 值设置好后，我们可以通过 `kevent` 函数向内核注册或取消一个指定文件描述符的事件：

```c
int kevent(int kq, const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents,
    const struct timespec *timeout);
```

比如下面例子中，设置一个套接字 `sockfd` 的可读事件，并注册到 `kqueue`:

```c
struct kevent event;
EV_SET(&event, sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
kevent(kfd, &event, 1, NULL, 0, NULL)
```

而如果想取消事件，只需要把 `EV_ADD` 替换为 `EV_DELETE`，并调用 `kqueue` 即可实现取消操作。

3.等待事件触发

前面说的，`kqueue` 接口设计上面非常简洁，如果需要监听事件的发生，我们可以直接使用 `kevent` 来监听内核产生的事件。
不同于前面使用 `kqueue` 设置某个套机字事件，在监听产生事件时，我们可能需要监听多个事件的同时产生，其具体流程如下：

```c
#define POLL_MAX_FD 64

struct kevent events[POLL_MAX_FD];
int nready;

for (;;)
{
    nready = kevent(kq, NULL, 0, events, POLL_MAX_FD, NULL);
    if (nready == -1)
        errno_abort("kevent error");

    for (i = 0; i < nready; i++)
    {
        sockfd = events[i].ident;
        size = events[i].data;

        // TODO somethings...
    }
}
```

上面例子中，我们定义了一个 events 数组，并指定其最大项为64，每个循环调用 `kevent` 时，只需将该数组传入内核，并告知其应用能处理的最大套接字数量。内核在某些套接字产生事件时，会把产生事件的套接字数据填充到 `events` 数组内，并返回触发事件的个数。程序在 `kevent` 返回后，只需要循环获取套接字信息，根据事件类型依次处理相关触发事件。

### kqueue 服务器实现

下面时一个使用 `kqueue` 实现的回射服务器（kqueue_server.c），其内容于上面 `epoll` 实现类似，不同的只是，相关函数改成了 `kqueue` 的接口。

```c
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>

#include "unp.h"

static int tcp_listen()
{
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errno_abort("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        errno_abort("setsockopt error");

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errno_abort("bind error");

    if (listen(listenfd, SOMAXCONN) < 0)
        errno_abort("listen error");

    return listenfd;
}

static int fd_create()
{
    return kqueue();
}

static void fd_close(int sockfd)
{
    close(sockfd);
}

static int fd_add(int kfd, int sockfd)
{
    struct kevent event;
    EV_SET(&event, sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    return kevent(kfd, &event, 1, NULL, 0, NULL);
}

static void fd_delete(int kfd, int sockfd)
{
    struct kevent event;
    EV_SET(&event, sockfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kfd, &event, 1, NULL, 0, NULL);
}

static int fd_nonbloking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void on_connect(int kfd, int listenfd, int size)
{
    int j;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    for (j = 0; j < size; j++)
    {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
            errno_abort("accept error");

        printf("new connection, ip = %s port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        if (fd_nonbloking(connfd) < 0)
            errno_abort("fd_nonbloking error");

        if (fd_add(kfd, connfd) < 0)
            errno_abort("fd_add error");
    }
}

static void on_message(int kfd, int sockfd, int size)
{
    int n;
    char buf[MAX_MSG_SIZE] = {0};

    if ((n = recv(sockfd, buf, size, 0)) != size)
        errno_abort("recv error");
    else if (n == 0)
    {
        printf("client disconnect\n");
        fd_delete(kfd, sockfd);
        fd_close(sockfd);
    }
    else
    {
        printf("echo %d bytes, data receved at %s", size, buf);

        if (send(sockfd, buf, n, 0) != n)
            errno_abort("writen error");
    }
}

static void echo_handle(int kfd, int listenfd)
{
    int i, nready, sockfd, size;
    struct kevent events[POLL_FD_SIZE];

    for (;;)
    {
        nready = kevent(kfd, NULL, 0, events, POLL_FD_SIZE, NULL);
        if (nready < 0)
            errno_abort("kevent error");

        for (i = 0; i < nready; i++)
        {
            sockfd = events[i].ident;
            size = events[i].data;

            if (sockfd == listenfd)
            {
                on_connect(kfd, listenfd, size);
            }
            else
            {
                on_message(kfd, sockfd, size);
            }
        }
    }
}

int main()
{
    int kfd, listenfd;

    listenfd = tcp_listen();

    if (fd_nonbloking(listenfd) < 0)
        errno_abort("fd_nonbloking error");

    if ((kfd = fd_create()) < 0)
        errno_abort("fd_create error");

    if (fd_add(kfd, listenfd) < 0)
        errno_abort("fd_add error");

    echo_handle(kfd, listenfd);

    return 0;
}
```

## 参考

- [epoll(7) - Linux Programmer's Manual](http://man7.org/linux/man-pages/man7/epoll.7.html)
- [epoll 维基百科](https://zh.wikipedia.org/wiki/Epoll)
- [kqueue(2) - FreeBSD Manual Pages](https://www.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2)
- [kqueue tutorial](https://wiki.netbsd.org/tutorials/kqueue_tutorial/#index1h1)
- [IBM developerWorks](https://www.ibm.com/developerworks/cn/aix/library/1105_huangrg_kqueue/index.html)