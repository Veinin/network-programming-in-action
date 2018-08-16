# TCP/IP 模型

传输层中，绝大多数客户/服务器网络应用使用TCP或UDP。这些传输协议都转而使用网络层协议IP：或是IPv4，或是 IPv6。
UDP是一个简单的、不可靠的数据报协议，而TCP是一个复杂、可靠的字节流协议。TCP的某些特性一旦理解，就很容易编写健壮的客户和服务器程序，也很容易使用诸如 netstat等普遍可用的工具来调试客户和服务器程序。

## IPv4

网际协议版本4（Internet Protocol version 4） 。IPv4（通常称之为IP）自20世纪80年代早期以来一直是网际协议族的主力协议。它使用32位地址 。IPv4给TCP、UDP、SCTP、ICMP和IGMP提供分组递送服务。

## IPv6

网际协议版本6（Internet Protocol version 6） 。IPv6是在20世纪90年代中期作为IPv4的一个替代品设计的。其主要变化是使用128位更大地址（见A.5节）以应对20世纪90年代因特网的爆发性增长。IPv6给TCP、UDP、SCTP和ICMPv6提供分组递送服务。当无需区别IPv4和IPv6时，我们经常把“IP”一词作为形容词使用，如IP层、IP地址等。

## TCP

传输控制协议（Transmission Control Protocol） 。TCP是一个面向连接的协议，为用户进程提供可靠的全双工字节流。TCP套接字是一种流套接字（stream socket）。TCP关心确认、超时和重传之类的细节。大多数因特网应用程序使用TCP。注意，TCP既可以使用IPv4，也可以使用IPv6。

## UDP

用户数据报协议（User Datagram Protocol） 。UDP是一个无连接协议。UDP套接字是一种数据报套接字（datagram socket）。UDP数据报不能保证最终到达它们的目的地。与TCP一样，UDP既可以使用IPv4，也可以使用IPv6。

## 物理层

## 链路层

## 网络层

## 传输层

## 应用层

## 坑太大，有时间再填