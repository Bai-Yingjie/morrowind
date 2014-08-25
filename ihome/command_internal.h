#ifndef _COMMAND_INTERNAL_H_
#define _COMMAND_INTERNAL_H_

#include "command.h"

#define command_lock_init(lock)
#define command_lock_deinit(lock)
#define command_lock(lock)
#define command_unlock(lock)
#define command_ok_char(c)  ((c) >= 0x21 && (c) <= 0x7e)

struct command_t {
	char *name;
	exec_fun_t exec;
	struct command_t *next;
};

struct argv_kit_t {
#define max_argv_buf_size 1024
#define max_argc          32
	char *buf;
	char **argv;
	int argc;
	int alloced;
};

struct command_lock_t {
	int lock;
};

struct command_head_t {
	struct command_lock_t lock;
	struct command_t *user;
	struct command_t *sys;
	struct argv_kit_t *argv_kit;
	output_fun_t output;
#define max_output_buf_size 2048
	char *output_buf;
};

#endif
