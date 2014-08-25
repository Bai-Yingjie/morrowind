#ifndef __PARSE_PKT_H__
#define __PARSE_PKT_H__

#define SOI          0xfe
#define EOI          0xc5
#define PKT_BUF_SIZE 1024
	
#define buf_empty(pkt_buf) ((pkt_buf).length == 0)

struct pkt_buf {
	unsigned char data[PKT_BUF_SIZE];
	unsigned int length;
};

typedef void (*on_pkt_received) (unsigned char *, int);

extern void parse_pkt(unsigned char *buff, unsigned int size, on_pkt_received recv_cb);

#endif
