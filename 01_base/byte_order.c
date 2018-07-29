#include <stdio.h>
#include <arpa/inet.h>

void print_byte_order(const unsigned char *p)
{
	printf("%0x %0x %0x %0x\n", p[0], p[1], p[2], p[3]);
	if (p[0] == 2 && p[1] == 1)
		printf("big-endian\n");
	else if (p[0] == 0 && p[1] == 0)
		printf("little-endian\n");
	else
		printf("unknown\n");
}

int main(void)
{
	unsigned int x = 0x0102;
	unsigned char *p = (unsigned char *)&x;
	print_byte_order(p);

	unsigned int y = htonl(x);
	p = (unsigned char *)&y;
	print_byte_order(p);

	return 0;
}