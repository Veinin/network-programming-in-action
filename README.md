# 网络编程实战

## 目录

* 入门
  * TCP/IP模型
  * [套接字编程基础](./doc/1_1_get_started_socket_programming.md)
  * [TCP回射服务器](./doc/1_2_tcp_echo_server.md)
  * [TCP粘包问题](./doc/1_3_tcp_stick_package.md)
  * [TCP状态机](./doc/1_4_tcp_state_machine.md)
  * [套接字选项](./doc/1_5_scoket_options.md)
  * UDP

* 设计
  * [C10K 与 C10M 问题](./dock/2_0_the_c10k_and_c10m_problem.md)
  * [网络服务器程序设计方案](./doc/2_1_network_server_design.md)
  * [PPC 与 TPC](./doc/2_2_ppc_and_tpc.md)
  * [I/O 复用：select 与 poll](./doc/2_3_io_multiplexing_select_and_poll.md)
  * [非阻塞I/O](./doc/2_4_non_blocking_io.md)
  * [I/O 复用：epoll 与 kqueue](./doc/2_5_io_multiplexing_epol_and_kqueue.md)

* Reactor
  * Reactor 模式详解
  * Reactor 网络库实现

* 测试
  * 网络传输性能测试要点
  * Netcat 实现
  * TTCP 实现

* 协议
  * HTTP与HTTPS
  * DNS
  * SOCKS
  * RPC