#include "unp.h"

int main()
{
    // get host name
    char host[64] = {0};
    if (gethostname(host, sizeof(host)) < 0)
        errno_abort("gethostname");

    printf("host name : %s\n", host);

    // get host by name
    struct hostent *htptr;
    if ((htptr = gethostbyname(host)) == NULL)
        errno_abort("gethostbyname error");

    int i = 0;
    while (htptr->h_addr_list[i] != NULL)
    {
        printf("ip addres : %s\n", inet_ntoa(*(struct in_addr *)htptr->h_addr_list[i]));
        ++i;
    }

    // get host by addr
    struct in_addr hipaddr;
    hipaddr.s_addr = inet_addr("35.187.152.135");

    if ((htptr = gethostbyaddr(&hipaddr, sizeof(hipaddr), AF_INET)) == NULL)
        errno_abort("gethostbyaddr error");

    printf("host name : %s\n", htptr->h_name);

    return 0;
}