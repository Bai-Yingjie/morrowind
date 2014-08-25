#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"
#include "ihome_shell.h"
#include "console_io.h"
#include "xtty_io.h"
#include "ihome_command.h"

#define XTTY_NAME     "ihome_cli"

int ihome_shell_init(int which_io)
{
	struct shell_io_t *io;
	int ret;

	if (which_io == SHELL_XTTY)
		io = get_xtty_io(XTTY_NAME);
	else
		io = get_console_io();

	ret = shell_init(io);
	if (!ret)
		ret = ihome_command_init(io->out);
	return ret;
}

void ihome_shell_run(void)
{
	shell_run(ihome_command_exec);
}

void ihome_shell_stop(void)
{
	shell_stop();
}
