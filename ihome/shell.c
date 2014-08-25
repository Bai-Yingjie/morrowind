#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utility.h"
#include "shell.h"

static struct shell_io_t *_io = NULL;
static int stop = 1;
static char *prompt = NULL;

int shell_init(struct shell_io_t *io)
{
	shell_set_prompt("==>");
	return shell_set_io(io);
}

static inline int shell_out(char *buf)
{
	return _io->out(buf);
}

static void new_line()
{
	shell_out("\r\n");
}

static void show_prompt()
{
	new_line();
	if (prompt)
		shell_out(prompt);
}

static inline int shell_invalid_line()
{
	return _io->in_buf.buf[0] == '\n';
}

static int shell_read_line()
{
	int len;
	len = _io->in(_io->in_buf.buf, _io->in_buf.alloced);
	if (len <= 0 || shell_invalid_line())
		return 0;
	_io->in_buf.buf[len] = '\0';
	return len;
}

static void shell_exec(void (*exec) (char *s))
{
	new_line();
	if (exec)
		exec(_io->in_buf.buf);
	else
		shell_out("no command exec function!!!\n");
}

void shell_run(void (*exec) (char *s))
{
	for (stop = 0; !stop;) {
		show_prompt();
		if (shell_read_line())
			shell_exec(exec);
	}
}

void shell_stop()
{
	stop = 1;
}

int shell_printf(char *fmt, ...)
{
	int n;
	char *buf;

	va_list arg;
	va_start(arg, fmt);

	if (!stop) {
		buf = _io->out_buf.buf;
		n = vsprintf(buf, fmt, arg);
		va_end(arg);
		buf[n] = '\0';
		n = shell_out(buf);
	} else {
		n = vfprintf(stderr, fmt, arg);
	}
	return n;
}

static inline int __io_buf_ok(struct shell_buf_t *buf)
{
	return buf->buf && buf->alloced > 0;
}

static inline int __io_ok(struct shell_io_t *io)
{
	return io && io->in && io->out;
}

int shell_set_io(struct shell_io_t *io)
{
	if (__io_ok(io) && __io_buf_ok(&io->in_buf) && __io_buf_ok(&io->out_buf)) {
		_io = io;
		return 0;
	}
	return -1;
}

void shell_set_prompt(char *_prompt)
{
	if (prompt) {
		xfree(prompt);
		prompt = NULL;
	}

	if (_prompt)
		prompt = xstrdup(_prompt);
}

char *shell_get_prompt(void)
{
	return prompt;
}
