#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utility.h"
#include "command_internal.h"

static int command_matched(char *cmd_name, char *name)
{
	return strcmp(cmd_name, name) == 0;
}

static struct command_t *new_command(char *name, exec_fun_t exec)
{
	struct command_t *cmd;

	if (!name || !exec)
		return NULL;

	cmd = xmalloc(sizeof(struct command_t));
	cmd->name = xstrdup(name);
	cmd->exec = exec;
	cmd->next = NULL;
	return cmd;
}

static int __command_add(struct command_t **list, struct command_t *cmd)
{
	struct command_t *curr, *prev;

	if (*list == NULL || strcmp(cmd->name, (*list)->name) < 0) {
		cmd->next = *list;
		*list = cmd;
	} else {
		prev = *list;
		curr = prev->next;

		while (curr) {
			if (strcmp(cmd->name, curr->name) < 0) {
				cmd->next = curr;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
		prev->next = cmd;
	}
	return 0;
}

static int command_add(struct command_head_t *cmd_head, struct command_t *cmd, int sys_cmd)
{
	int ret = 0;
	command_lock(cmd_head->lock);
	if (sys_cmd)
		ret = __command_add(&cmd_head->sys, cmd);
	else
		ret = __command_add(&cmd_head->user, cmd);
	command_unlock(cmd_head->lock);
	return ret;
}

static struct command_t *__command_find(struct command_t *list, char *name)
{
	struct command_t *cmd = list;
	while (cmd) {
		if (command_matched(cmd->name, name))
			break;
		cmd = cmd->next;
	}
	return cmd;
}

static void command_destroy_list(struct command_t *list)
{
	struct command_t *curr, *next;
	curr = list;
	while (curr) {
		next = curr->next;
		xfree(curr->name);
		xfree(curr);
		curr = next;
	}
}

static struct command_t *command_find(struct command_head_t *cmd_head, char *cmd_name, int sys_cmd)
{
	struct command_t *cmd;
	command_lock(cmd_head->lock);
	if (sys_cmd)
		cmd = __command_find(cmd_head->sys, cmd_name);
	else
		cmd = __command_find(cmd_head->user, cmd_name);
	command_unlock(cmd_head->lock);
	return cmd;
}

static int command_sys_install(struct command_head_t *cmd_head)
{
	return 0;
}

static int command_default_output(char *str)
{
	int n = fprintf(stdout, "%s", str);
	fflush(stdout);
	return n;
}

static int command_printf(struct command_head_t *cmd_head, char *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = vsprintf(cmd_head->output_buf, fmt, arg);
	va_end(arg);
	cmd_head->output_buf[n] = '\0';
	return cmd_head->output(cmd_head->output_buf);
}

static inline void command_init_argv_kit(struct argv_kit_t *argv_kit)
{
	argv_kit->argc = 0;
}

static int command_prepare_argv(struct argv_kit_t *argv_kit, char *cmdline)
{
	char *s, *e;

	strcpy(argv_kit->buf, cmdline);

	s = argv_kit->buf;
	e = s;
	for (;;) {
		while (*e != '\0' && !command_ok_char(*e))
			e++;
		if (*e == '\0')
			break;
		s = e;

		while (command_ok_char(*e))
			e++;
		if (*e != '\0')
			*e++ = '\0';

		argv_kit->argv[argv_kit->argc] = s;
		argv_kit->argc++;
		if (argv_kit->argc >= argv_kit->alloced)
			break;
	}
	return argv_kit->argc;
}

static inline int command_is_help(char *name)
{
	return command_matched(name, "?");
}

static void command_show(struct command_head_t *cmd_head, struct command_t *list)
{
	int i = 0;
	struct command_t *cmd = list;
	while (cmd) {
		if (i % 4 == 0)
			command_printf(cmd_head, "\r\n");
		command_printf(cmd_head, "%-12s ", cmd->name);
		cmd = cmd->next;
		i++;
	}
}

static void command_sys_help(struct command_head_t *cmd_head)
{
	command_show(cmd_head, cmd_head->sys);
	command_show(cmd_head, cmd_head->user);
}

static int command_exec_sys(struct command_head_t *cmd_head, struct argv_kit_t *argv_kit)
{
	struct command_t *cmd;
	char *cmd_name;
	int ret = 0;

	cmd_name = argv_kit->argv[0];
	if (!command_is_help(cmd_name)) {
		cmd = command_find(cmd_head, cmd_name, 1);
		if (cmd)
			cmd->exec(argv_kit->argc, argv_kit->argv);
		else
			ret = -1;
	} else {
		command_sys_help(cmd_head);
	}

	return ret;
}

static int command_exec_user(struct command_head_t *cmd_head, struct argv_kit_t *argv_kit)
{
	int ret = -1;
	struct command_t *cmd = command_find(cmd_head, argv_kit->argv[0], 0);
	if (cmd) {
		cmd->exec(argv_kit->argc, argv_kit->argv);
		ret = 0;
	}
	return ret;
}

static void __command_exec(struct command_head_t *cmd_head, struct argv_kit_t *argv_kit)
{
	if (command_exec_sys(cmd_head, argv_kit)) {
		if (command_exec_user(cmd_head, argv_kit)) {
			command_printf(cmd_head, "no such commamnd: %s\n", argv_kit->argv[0]);
		}
	}
}

int command_init(struct command_head_t **cmd_head)
{
	struct command_head_t *head;
	struct argv_kit_t *argv_kit;

	argv_kit = xmalloc(sizeof(struct argv_kit_t));
	argv_kit->buf = xmalloc(max_argv_buf_size);
	argv_kit->argv = xmalloc(sizeof(char *) * max_argc);
	argv_kit->argc = 0;
	argv_kit->alloced = max_argc;

	head = xmalloc(sizeof(struct command_head_t));
	head->argv_kit = argv_kit;
	head->user = NULL;
	head->sys = NULL;
	head->output = command_default_output;
	head->output_buf = xmalloc(max_output_buf_size);;
	command_lock_init(head->lock);

	if (command_sys_install(head)) {
		command_destroy(head);
		return -1;
	}

	*cmd_head = head;
	return 0;
}

int command_install(struct command_head_t *cmd_head, char *cmd_name, exec_fun_t exec)
{
	int ret = -1;
	struct command_t *cmd;
	cmd = new_command(cmd_name, exec);
	if (cmd)
		ret = command_add(cmd_head, cmd, 0);
	return ret;
}

void command_exec(struct command_head_t *cmd_head, char *cmdline)
{
	command_init_argv_kit(cmd_head->argv_kit);
	if (command_prepare_argv(cmd_head->argv_kit, cmdline))
		__command_exec(cmd_head, cmd_head->argv_kit);
	else
		command_printf(cmd_head, "invalid cmdline\n");
}

void command_set_output(struct command_head_t *cmd_head, output_fun_t new, output_fun_t old)
{
	if (old)
		old = cmd_head->output;
	if (new)
		cmd_head->output = new;
}

void command_destroy(struct command_head_t *cmd_head)
{
	xfree(cmd_head->argv_kit->buf);
	xfree(cmd_head->argv_kit->argv);
	xfree(cmd_head->argv_kit);
	xfree(cmd_head->output_buf);
	command_destroy_list(cmd_head->sys);
	command_destroy_list(cmd_head->user);
	command_lock_deinit(cmd_head->lock);
	xfree(cmd_head);
}
