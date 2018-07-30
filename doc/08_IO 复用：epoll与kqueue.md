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
- EPOLLERR，调用 `read` 或 `write` 函数时发生异常。
- EPOLLHUP，
- EPOLLET，
- EPOLLONESHOT，

3.等待注册事件

```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

调用该函数后将等待 `epfd` 的事件产生。

`events` 指针在函数返回后，将包含一个当已经触发的事件集合。参数 `maxevents` 可以指定返回事件最大数量，且 `maxevents` 参数在设置时必须大于0。

参数 `timeout` 将指定 `epoll_wait` 需要阻塞等待事件的超时事件（毫秒）。如果参数为 0，函数将立即返回；如果参数为-1，将永远阻塞，知道有事件产生。

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

下面是一个使用了 `LT` 触发模式的服务器程序（源文件`epoll_server.c`）：

## kqueue 函数

`kqueue` 与 `epoll` 类似，也是一个可扩展的基于事件通知的编程接口，它于2000年7月在FreeBSD 4.1中引入，同时也支持NetBSD，OpenBSD，DragonflyBSD 和 MacOS。

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

通过 `kqueue` 函数，可以直接创建一个 `kqueue` 对象，如下代码：

```c
int kq;

if ((kq = kqueue()) == -1)
    errno_abort("kqueue error");
```

`kqueue` 函数会在内核中创建一个事件队列，并返回该对象的返回一个描述符，如果使用 `fork` 等函数，子进程将不会继承该队列。

2.注册事件

`kqueue` 事件数据结构叫做 `kevent`，结构定义如下：

```c
struct kevent {
     uintptr_t ident;    /* 事件ID */
     short     filter;   /* 事件过滤器 */
     u_short   flags;    /* 活动标识 */
     u_int     fflags;   /* 过滤器标识 */
     intptr_t  data;     /* 过滤器数据 */
     void      *udata;   /* 应用穿透数据 */
};
```

`kevent` 各个字段定义：

- ident，用于标识事件，它可以是一个描述符（文件、套接字、流）、进程ID或信号编号，这取决于你要监控的内容。
- filter，事件过滤器，用于标识内核处理事件时使用的过滤器。
- flags，活动标识，指定需要对哪些事件执行操作。
- fflags，过滤器相关标识符。
- data，过滤器相关的数据。
- udata，通过内核携带的额外用户数据，该数据不会被内核更改。

可以用以下值来标识上面的 `flags` 字段：

## 参考

[epoll(7) - Linux Programmer's Manual](http://man7.org/linux/man-pages/man7/epoll.7.html)
[epoll 维基百科](https://zh.wikipedia.org/wiki/Epoll)
[kqueue(2) - FreeBSD Manual Pages](https://www.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2)
[kqueue tutorial](https://wiki.netbsd.org/tutorials/kqueue_tutorial/#index1h1)



>>>>>>> c687d4f640edac269dd56ccb923624b5cf722e32
