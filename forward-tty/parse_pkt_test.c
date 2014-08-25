#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse_pkt.h"

static void on_received_pkt(unsigned char *pkt, int size)
{
	int i;
	printf("reveive pkt, size %d\n", size);
	for (i = 0; i < size; i++)
		printf("%02X ", *(unsigned char *) (pkt + i));
	printf("\n");
}

static unsigned char test1_data[] = {
	0xFE, 0xAA, 0xAA, 0xAA, 0xC5,
	0xFE, 0xBB, 0xBB, 0xBB, 0xC5,
	0xFE, 0xCC, 0xCC, 0xCC
};

static unsigned char test2_data[] = {
	0xC5, 0xFE, 0xDD, 0xDD, 0xDD, 0xC5,
	0xFE, 0xEE, 0xEE, 0xEE, 0xC5
};

#define dim(a) sizeof(a)/(sizeof((a)[0]))

int main(int argc, char *argv[])
{
	printf("test1...\n");
	parse_pkt(test1_data, dim(test1_data), on_received_pkt);

	printf("test2...\n");
	parse_pkt(test2_data, dim(test2_data), on_received_pkt);

	return 0;
}
