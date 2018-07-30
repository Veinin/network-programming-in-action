
# socket API 操作系统差异

尽管伯克利套接字在不同的平台都是互联网通信的标准底层标准。但并不是所有平台都一样的，如果需要进行跨平台开发，则需要知道不同平台的套接字操作差异性。

## 套接字类型表示差异

在 Window 平台 scoket 函数返回的是 SOCKET 类型。
而其他POSIX平台（Linux、Mac OS...）都是用一个 int 类型来表示。

```c
SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0); // windows 平台
int sockfd = socket(AF_INET, SOCK_STREAM, 0); // windows 平台
```

值得注意的是，不管任何平台，调用socket相关的操作函数时候，套接字socket都应该以**值传递**的形式进行。

## 头文件差异

Windows 的 socket 库使用的头文件是 `Winsock2.h`。在较早期的 Windows 旧版本中是 `Winsock.h`, 这个头文件已经默认放到了大多数 Window 程序需要用到的 `Windows.h` 头文件中。虽然 `Winsock` 是一个功能有限，未优化的版本，但已经有了一些基本的库函数，为了防止命名冲突，必须保证 `Windows.h` 出现在 `WindSock2.h` 之前。
另外一个常用的如地址转换功能函数，则放在了 `Ws2tcpip.h` 中。

```c
#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
```

而在 POSIX 平台，则是统一使用 `sys/socket.h` 头文件。还有诸如 IPv4 功能的 `netinet/in.h`，地址转换功能头文件 `arpa/inet.h`，名称解析头文件 `netdb.h`。

## 初始化和关闭差异

对于 socket 库，POSIX 平台默认是开启状态的。但是，Winsock2 则需要显示的激活和关闭，并指定用户所用的版本。
在 Windows 中使用 socket 库之前，需要用 WSAStartup 来激活 socket 库。

```c
int WSAStartup(WORD wVersionRequested, LPWSADATE lpWSAData); // 返回值为0表示成功
```

wVersionRequested 是两个字节的 WORD，低字节表示主版本号，高字节表示最低版本号。比如使用 MAKEWORD(2, 2) 表示版本号 2.2。
lpWSAData 指向特定的 Windows 数据结构，通常这个值与特定的版本相匹配，不需要检查该数据。

在 WSAStartup 激活成功后，你才能运行 WSACleanup 来关闭 socket 库。

```c
int WSACleanup();
```

## 错误处理差异

大部分平台如果执行的函数错误时，通常会返回 `-1` 错误码。
在 Windows 中，可以使用宏 `SOCKET_ERROR` 来代替 `-1`，并使用 `WSAGetLastError` 函数来获取额外的错误提示代码。
在 POSIX 中，使用的是 C 标准库的全局变量 `errno` 来报告错误代码，也可以使用诸如 `perror` 函数来获取错误信息。