#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse_pkt.h"

static struct pkt_buf pkt = { {0}, 0 };

static void process_on_buf_empty(unsigned char *buff, unsigned int size, on_pkt_received recv_cb)
{
	int i;
	int soi;
	int copied;

	i = 0;
	while (1) {
		while (i < size && buff[i] != SOI)
			i++;

		if (i >= size)
			break;
		while (i < size && buff[i] == SOI)
			i++;
		soi = i - 1;
		while (i < size && buff[i] != EOI)
			i++;

		if (i >= size) {
			copied = size - soi;
			memcpy(pkt.data, buff + soi, copied);
			pkt.length = copied;
			break;
		} else {
			recv_cb(buff + soi, i - soi + 1);
		}
	}
}

static void process_on_buf_have_data(unsigned char *buff, unsigned int size, on_pkt_received recv_cb)
{
	int i;
	int copied;

	i = 0;
	while (i < size && buff[i] != SOI && buff[i] != EOI)
		i++;

	if (buff[i] == SOI) {
		process_on_buf_empty(buff + i, size - i, recv_cb);
	} else if (buff[i] == EOI) {
		copied = i + 1;
		memcpy(&pkt.data[pkt.length], buff, copied);
		pkt.length += copied;
		recv_cb(pkt.data, pkt.length);
		process_on_buf_empty(buff + copied, size - copied, recv_cb);
	} else {
		memcpy(&pkt.data[pkt.length], buff, size);
		pkt.length += size;
	}
}

void parse_pkt(unsigned char *buff, unsigned int size, on_pkt_received recv_cb)
{
	if (!buff || !size)
		return;

	if (buf_empty(pkt))
		process_on_buf_empty(buff, size, recv_cb);
	else
		process_on_buf_have_data(buff, size, recv_cb);
}
