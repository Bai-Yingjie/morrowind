#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utility.h"
#include "input_helper.h"
#include "console_io.h"

#define CONSOLE_IO_BUF_SIZE  2048
#define STD_IN_FD    0
#define STD_OUT_FD   1

static struct shell_io_t io;
static int has_init = 0;
static struct input_helper *ih = NULL;

static int console_io_in(char *buf, int buf_size)
{
	return ih_read_line(ih, buf, buf_size);
}

static int console_io_out(char *str)
{
	return write(STD_OUT_FD, str, strlen(str));
}

struct shell_io_t *get_console_io()
{
	if (!has_init) {
		io.in = console_io_in;
		io.out = console_io_out;
		io.in_buf.buf = xmalloc(CONSOLE_IO_BUF_SIZE);
		io.in_buf.alloced = CONSOLE_IO_BUF_SIZE;
		io.out_buf.buf = xmalloc(CONSOLE_IO_BUF_SIZE);
		io.out_buf.alloced = CONSOLE_IO_BUF_SIZE;
		ih = ih_init(STD_IN_FD);
		ih_enable_history_input(ih);
		has_init = 1;
	}
	return &io;
}
