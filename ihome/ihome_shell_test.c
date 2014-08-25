#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ihome_command.h"
#include "ihome_shell.h"

int main(int argc, char *argv[])
{
	int ret = -1;
	if (argc > 1) {
		if (strcmp("--stdio", argv[1]) == 0) {
			ret = ihome_shell_init(SHELL_CONSOLE);
			fprintf(stderr, "ihome_shell_init, stdio, ret=%d\n", ret);
		} else if (strcmp("--xtty", argv[1]) == 0) {
			ret = ihome_shell_init(SHELL_XTTY);
			fprintf(stderr, "ihome_shell_init, xtty, ret=%d\n", ret);
		} else
			fprintf(stderr, "unknow input=%s\n", argv[1]);
	}

	if (ret == 0) {
		ihome_printf("enter ihome_shell_run\n");
		ihome_shell_run();
		ihome_printf("exit ihome_shell_run\n");
	} else {
		ihome_printf("no shell!\n");
	}

	return 0;
}
