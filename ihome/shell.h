#ifndef _SHELL_H_
#define _SHELL_H_

#include "shell_io.h"

int shell_init(struct shell_io_t *io);
void shell_run(void (*exec) (char *s));
void shell_stop(void);
int shell_set_io(struct shell_io_t *io);
void shell_set_prompt(char *prompt);
char *shell_get_prompt(void);
int shell_printf(char *fmt, ...);

#endif
