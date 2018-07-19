#include "unp.h"

void echo_handle(int sockfd)
{
	char sendbuf[1024] = {0};
	char recvbuf[1024] = {0};
	while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
	{
		write(sockfd, sendbuf, strlen(sendbuf));
		read(sockfd, recvbuf, sizeof(recvbuf));

		printf("echo %ld bytes, data receved at %s", strlen(recvbuf), recvbuf);

		memset(&sendbuf, 0, sizeof(sendbuf));
		memset(&recvbuf, 0, sizeof(recvbuf));
	}
}

int main(void)
{
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errno_abort("socket error");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		errno_abort("connect error");

	echo_handle(sockfd);

	return 0;
}