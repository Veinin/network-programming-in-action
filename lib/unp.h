#ifndef UNP_H
#define UNP_H

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define SERV_PORT 8888
#define MAX_MSG_SIZE 1024
#define POLL_FD_SIZE 1024

typedef struct
{
	short lenth;
	char buff[MAX_MSG_SIZE];
} package;

#define err_quit(text) \
	do {\
		fprintf(stderr, "%s at \"%s\":%d\n",\
				text, __FILE__, __LINE__);\
		exit(1);\
	} while (0)

#define err_abort(code, text) \
	do {\
		fprintf(stderr, "%s at \"%s\":%d: %s\n",\
			text, __FILE__, __LINE__, strerror(code));\
		abort();\
	} while(0)

#define errno_abort(text) \
	do {\
		fprintf(stderr, "%s at \"%s\":%d: %s\n",\
			text, __FILE__, __LINE__, strerror(errno));\
		abort();\
	} while(0)

int 	Socket(int, int, int);
void 	Bind(int, const struct sockaddr *, socklen_t);
void 	Listen(int, int);
int 	Accept(int, struct sockaddr *, socklen_t *);
void 	Connect(int, const struct sockaddr *, socklen_t);
void 	Close(int);

void	Getpeername(int, struct sockaddr *, socklen_t *);
void	Getsockname(int, struct sockaddr *, socklen_t *);

void 	Setsockopt(int, int, int, const void *, socklen_t);

ssize_t	Read(int, void *, size_t);
void	Write(int, void *, size_t);

size_t	readn(int, void *, size_t);
size_t  Readn(int, void *, size_t);

size_t 	writen(int, void *, size_t);
void 	Writen(int, void *, size_t);

size_t  readline(int, void *, size_t);
size_t 	Readline(int, void *, size_t);

#endif // UNP_H