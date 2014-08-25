#ifndef _MARVELL_SHELL_H_
#define _MARVELL_SHELL_H_

#include "shell.h"

#define SHELL_CONSOLE 0
#define SHELL_XTTY    1

#define ihome_printf   shell_printf

int ihome_shell_init(int which_io);
void ihome_shell_run(void);
void ihome_shell_stop(void);

#endif
