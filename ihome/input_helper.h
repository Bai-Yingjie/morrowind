#ifndef __INPUT_HELPER_H_
#define __INPUT_HELPER_H_

struct input_helper;

struct input_helper *ih_init(int fd);
void ih_deinit(struct input_helper *ih);
int ih_enable_history_input(struct input_helper *ih);
int ih_disable_history_input(struct input_helper *ih);
int ih_read_line(struct input_helper *ih, char *buff, int size);

#endif
