#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>

#include "utility.h"

#define MAX_HISTORY_NUM        32
#define MAX_INPUT_BUFF_SIZE    1024
#define MAX_PREFIX_LEN         32

#define KEY_BACKSPACE          0x08
#define KEY_DELETE             0x7F
#define KEY_ENTER              0x0D
#define KEY_NEW_LINE           0x0A
#define KEY_ESC                0x1B
#define KEY_LEFT_BRACKETS      0x5B	/*[ */
#define KEY_UP_OR_A            0x41	/*ESC+[+A */
#define KEY_DOWN_OR_B          0x42	/*ESC+[+B */

struct history {
	int enable;
	int current;
	int index;
	char *list[MAX_HISTORY_NUM];
};

struct input_helper {
	int fd;
	char *input;
	struct history *h;
	struct termios orig_termios;
};

static inline int __is_printable_char(char c)
{
	return ((c >= 0x20) && (c <= 0x7E));
}

static void __history_clear(struct input_helper *ih)
{
	struct history *h = ih->h;
	int i;

	for (i = 0; i < MAX_HISTORY_NUM; i++) {
		if (h->list[i])
			xfree(h->list[i]);
		h->list[i] = NULL;
	}
	h->current = h->index = 0;
}

static void __remove_char(struct input_helper *ih, int num)
{
	while (num-- > 0)
		write(ih->fd, "\b \b", 3);
}

static int __is_enable_history(struct input_helper *ih)
{
	struct history *h = ih->h;
	return h->enable;
}

static void __history_add(struct input_helper *ih, char *input)
{
	struct history *h = ih->h;
	int len = strlen(input);
	char *t = xmalloc(len + 1);

	if (t) {
		strncpy(t, input, len);
		t[len] = '\0';
		if (!h->list[h->current])
			xfree(h->list[h->current]);
		h->list[h->current] = t;
		h->index = h->current = (h->current + 1) % MAX_HISTORY_NUM;
	}
}

static void __record_history(struct input_helper *ih, char *input)
{
	if (input[0] != '\n' && input[0] != '\0')
		__history_add(ih, input);
}

static char *__history_prev(struct input_helper *ih)
{
	struct history *h = ih->h;
	int t = (h->index + MAX_HISTORY_NUM - 1) % MAX_HISTORY_NUM;
	char *cmd = NULL;

	if (h->list[t]) {
		h->index = t;
		cmd = h->list[t];
	}
	return cmd;
}

static char *__history_next(struct input_helper *ih)
{
	struct history *h = ih->h;
	char *cmd = NULL;

	if (h->index != h->current) {
		h->index = (h->index + 1) % MAX_HISTORY_NUM;
		cmd = h->list[h->index];
	}
	return cmd;
}

static int __read_line(struct input_helper *ih, char *buff, int size)
{
	int len;
	char c;
	int fd;
	char *input;
	int r_len = 0;
	int finish = 0;
	int esc_pressed = 0;

	fd = ih->fd;
	input = ih->input;

	do {
		if (r_len >= MAX_INPUT_BUFF_SIZE)
			break;

		len = read(fd, &c, 1);
		if (len < 0)
			break;

		switch (c) {
		case KEY_ENTER:
		case KEY_NEW_LINE:
			esc_pressed = 0;
			if (r_len == 0)
				input[r_len++] = '\n';
			finish = 1;
			break;

		case KEY_BACKSPACE:
		case KEY_DELETE:
			esc_pressed = 0;
			if (r_len > 0) {
				r_len--;
				__remove_char(ih, 1);
			}
			break;

		case KEY_ESC:
			esc_pressed = 1;
			break;

		case KEY_LEFT_BRACKETS:
			if (!esc_pressed) {
				input[r_len++] = c;
				write(fd, &c, 1);
			}
			break;

		case KEY_UP_OR_A:
			if (!esc_pressed) {
				input[r_len++] = c;
				write(fd, &c, 1);
			} else {
				if (__is_enable_history(ih)) {
					char *p = __history_prev(ih);
					if (p) {
						__remove_char(ih, r_len);
						r_len = strlen(p);
						strncpy(input, p, r_len);
						write(fd, p, r_len);
					}
				}
			}
			break;

		case KEY_DOWN_OR_B:
			if (!esc_pressed) {
				input[r_len++] = c;
				write(fd, &c, 1);
			} else {
				if (__is_enable_history(ih)) {
					char *p = __history_next(ih);
					__remove_char(ih, r_len);
					r_len = 0;
					if (p) {
						r_len = strlen(p);
						strncpy(input, p, r_len);
						write(fd, p, r_len);
					}
				}
			}
			break;

		default:
			if (esc_pressed) {
				esc_pressed = 0;
				break;
			}

			if (__is_printable_char(c)) {
				input[r_len++] = c;
				write(fd, &c, 1);
				continue;
			}
		}

	} while ((!finish) && (r_len < MAX_INPUT_BUFF_SIZE));

	input[r_len] = buff[size - 1] = '\0';
	len = size > r_len ? r_len : size - 1;
	strncpy(buff, input, len);

	if (__is_enable_history(ih))
		__record_history(ih, input);

	return len;
}

static int __history(struct input_helper *ih, int enable)
{
	int ret = -1;
	struct history *h = ih->h;
	if (0 == enable) {
		if (h->enable == 1) {
			__history_clear(ih);
			h->enable = 0;
			ret = 0;
		}
	} else if (1 == enable) {
		if (h->enable == 0) {
			__history_clear(ih);
			h->enable = 1;
			ret = 0;
		}
	} else {
		;
	}
	return ret;
}

static int make_raw_mode_attr(int fd, struct termios *termios)
{
	if (tcgetattr(fd, termios) < 0)
		return -1;
	/* set raw mode */
	cfmakeraw(termios);
	return 0;
}

static int ih_change_mode(int fd, struct termios *new_termios, struct termios *old_termios)
{
	if (old_termios && tcgetattr(fd, old_termios) < 0) {
		perror("\r\ngetting current terminal's attributes failed.");
		return -1;
	}

	if (new_termios && tcsetattr(fd, TCSANOW, new_termios) < 0) {
		perror("\r\nsetting current terminal's attributes failed.");
		return -1;
	}

	return 0;
}

static int ih_set_raw_mode(int fd, struct termios *orig_termios)
{
	struct termios termios;
	if (!make_raw_mode_attr(fd, &termios))
		if (!ih_change_mode(fd, &termios, orig_termios))
			return 0;
	return -1;
}

int ih_enable_history_input(struct input_helper *ih)
{
	if (ih)
		return __history(ih, 1);
	return -1;
}

int ih_disable_history_input(struct input_helper *ih)
{
	if (ih)
		return __history(ih, 0);
	return -1;
}

int ih_read_line(struct input_helper *ih, char *buff, int size)
{
	if (ih && buff && size)
		return __read_line(ih, buff, size);
	return -1;
}

struct input_helper *ih_init(int fd)
{
	struct input_helper *ih = xmalloc(sizeof(struct input_helper));
	ih->input = xmalloc(MAX_INPUT_BUFF_SIZE);
	ih->h = xmalloc(sizeof(struct history));
	memset(ih->h, 0, sizeof(struct history));
	ih->fd = fd;
	ih_set_raw_mode(ih->fd, &ih->orig_termios);
	return ih;
}

void ih_deinit(struct input_helper *ih)
{
	if (ih) {
		__history_clear(ih);
		ih_change_mode(ih->fd, &ih->orig_termios, NULL);
		xfree(ih->input);
		xfree(ih->h);
		xfree(ih);
	}
}
