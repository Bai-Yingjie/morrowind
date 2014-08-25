#include <stdio.h>
#include "command.h"
#include "ihome_shell.h"
#include "ihome_command.h"

static struct command_head_t *ihome_command_head;

/* ihome APIs about commands*/
static void show(int argc, char *argv[])
{
	ihome_printf("############# argc=%d, cmd=%s\n", argc, argv[0]);
}

static void quit(int argc, char *argv[])
{
	ihome_shell_stop();
}

/* Marvell commands list */
#define ihome_command_item(name,exec)  {#name,exec},
#define ihome_command_all_items                   \
	ihome_command_item(show, show)	    \
	ihome_command_item(quit, quit)
	/* new commands will be here */

struct ihome_command_t {
	char *name;
	exec_fun_t exec;
} commands[] = {
	ihome_command_all_items
};

static int ihome_command_install()
{
	int i;
	int nr = sizeof(commands) / sizeof(commands[0]);
	for (i = 0; i < nr; i++) {
		if (command_install(ihome_command_head, commands[i].name, commands[i].exec))
			return -1;
	}
	return 0;
}

int ihome_command_init(output_fun_t output)
{
#define _run(fun) do{ if(fun != 0) return -1; } while(0)
	_run(command_init(&ihome_command_head));
	command_set_output(ihome_command_head, output, NULL);
	_run(ihome_command_install());
	return 0;
#undef _run
}

void ihome_command_exec(char *cmdline)
{
	command_exec(ihome_command_head, cmdline);
}

void ihome_command_destroy()
{
	command_destroy(ihome_command_head);
}
