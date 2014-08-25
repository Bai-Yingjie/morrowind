#ifndef __FORWARD_SERVER_H
#define __FORWARD_SERVER_H

#include <stdio.h>
#include <stdlib.h>

#define PROGRAM		"forward_server"
#define VERSION		"0.4"

#define ERROR	"error"

#define DEBUG	"debug"
#define DEBUG_LEVEL_MIN		1
#define DEBUG_LEVEL_MAX		4

#define DEFAULT_USER_INPUT_NAME "/tmp/user.input"
#define DEFAULT_USER_OUTPUT_NAME "/tmp/user.output"

#define DEFAULT_TTY_NAME  "/dev/ttyS0"
#define DEFAULT_TTY_CONFIG "/tmp/tty.config"

#define NUM_THREADS	2

/* thread priority and schedule policy */
#define DEF_PRIORITY       80
#define DEF_SCHED_POLICY   SCHED_RR

#define STRTOL(x) ((unsigned int)strtol(x,NULL,10))

/* TCP server port */
#define PORT    11910

/* define the length of listen */
#define BACKLOG 5
#define LENGTH  1024

enum forward_type {
  READ_WRITE_OK = 0,
  READ_FAILURE = -1, 
  WRITE_FAILURE = -2,
};

enum serial_mode_type {
  RAW_MODE = 0,
  NORMAL_MODE = 1,
};

typedef struct serial_control_s {
  int fd;		/* file desc */
  int speed;	/* baudrate */
  int bits;	/* data bits  */
  int parity;  /* parity check */
  int stop; /* stop bits */
  int mode; /* raw or normal mode */
}serial_control_t;

typedef struct thread_param_s {
   char *name;
   int from;
   int to;
}thread_param_t;

#endif /* __FORWARD_SERVER_H */
