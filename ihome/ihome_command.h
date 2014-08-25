#ifndef _MARVELL_COMMAND_H_
#define _MARVELL_COMMAND_H_

#include "command.h"

extern int ihome_command_init(output_fun_t output);
extern void ihome_command_exec(char *cmdline);
extern void ihome_command_destroy(void);

#endif
