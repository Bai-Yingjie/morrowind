#ifndef _COMMAND_H_
#define _COMMAND_H_

struct command_head_t;

typedef void (*exec_fun_t) (int argc, char *argv[]);
typedef int (*output_fun_t) (char *str);

int command_init(struct command_head_t **cmd_head);
int command_install(struct command_head_t *cmd_head, char *cmd_name, exec_fun_t exec);
void command_exec(struct command_head_t *cmd_head, char *cmdline);
void command_set_output(struct command_head_t *cmd_head, output_fun_t new, output_fun_t old);
void command_destroy(struct command_head_t *cmd_head);

#endif
