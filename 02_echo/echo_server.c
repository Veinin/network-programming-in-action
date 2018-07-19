#include "unp.h"

void echo_handle(int connfd)
{
	int ret;
	char recvbuf[1024];
	while (1)
	{
		memset(&recvbuf, 0, sizeof(recvbuf));
		if ((ret = read(connfd, recvbuf, sizeof(recvbuf))) <= 0)
			break;

		printf("echo %ld bytes, data receved at %s", strlen(recvbuf), recvbuf);
		write(connfd, recvbuf, ret);
	}
	close(connfd);
}

int main(void)
{
	int listenfd;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errno_abort("socket error");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		errno_abort("bind error");

	if (listen(listenfd, SOMAXCONN) < 0)
		errno_abort("listen error");

	int connfd;
	struct sockaddr_in peeraddr;
	socklen_t peerlen = sizeof(peeraddr);
	if ((connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peerlen)) < 0)
		errno_abort("accept error");
	else
		printf("new connection, ip = %s port = %d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

	echo_handle(connfd);

	return 0;
}
