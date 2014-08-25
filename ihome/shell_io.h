#ifndef _SHELL_IO_H_
#define _SHELL_IO_H_

struct shell_buf_t {
	char *buf;
	int alloced;
};

struct shell_io_t {
	int (*in) (char *, int);
	int (*out) (char *);
	struct shell_buf_t in_buf;
	struct shell_buf_t out_buf;
};

#endif
