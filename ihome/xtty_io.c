#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>

#include "utility.h"
#include "input_helper.h"
#include "xtty_io.h"

#define XTTY_IO_BUF_SIZE  2048

static int xtty_fd = -1;
static int has_init = 0;
static struct shell_io_t io;
static struct input_helper *ih = NULL;

int xtty_create(char *name)
{
	int ptmx_fd = -1;
	int pts_fd = -1;
	char *pts_name = NULL;

	ptmx_fd = open("/dev/ptmx", O_RDWR);

	if (ptmx_fd < 0)
		return -1;

	pts_name = (char *) ptsname(ptmx_fd);
	if (NULL == pts_name) {
		close(ptmx_fd);
		return -1;
	}

	grantpt(ptmx_fd);
	unlockpt(ptmx_fd);

	pts_fd = open(pts_name, O_RDWR);
	if (pts_fd < 0)
		return -1;

	/* tell fd which process to signal */
	if (fcntl(ptmx_fd, F_SETOWN, getpid()) < 0) {
		perror("\r\nF_SETOWN failed.");
		close(ptmx_fd);
		return -1;
	}

	unlink(name);
	if (symlink(pts_name, name) < 0) {
		perror("\r\nsymlink failed.");
		close(ptmx_fd);
		return -1;
	}

	fprintf(stderr, "\r\n%s -> %s\r\n", name, pts_name);

	return ptmx_fd;
}

static int xtty_io_in(char *buf, int buf_size)
{
	return ih_read_line(ih, buf, buf_size);
}

static int xtty_io_out(char *str)
{
	return write(xtty_fd, str, strlen(str));
}

struct shell_io_t *get_xtty_io(char *xtty_name)
{
	if (!has_init) {
		xtty_fd = xtty_create(xtty_name);
		if (xtty_fd > 0) {
			io.in = xtty_io_in;
			io.out = xtty_io_out;
			io.in_buf.buf = xmalloc(XTTY_IO_BUF_SIZE);
			io.in_buf.alloced = XTTY_IO_BUF_SIZE;
			io.out_buf.buf = xmalloc(XTTY_IO_BUF_SIZE);
			io.out_buf.alloced = XTTY_IO_BUF_SIZE;
			ih = ih_init(xtty_fd);
			ih_enable_history_input(ih);
			has_init = 1;
		}
	}
	return has_init ? &io : NULL;
}
