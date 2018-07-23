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