#ifndef __FORWARD_CLIENT_H
#define __FORWARD_CLIENT_H

#include <stdio.h>
#include <stdlib.h>

#define PROGRAM		"forward_client"
#define VERSION		"0.4"

#define ERROR	"error"

#define DEBUG	"debug"
#define DEBUG_LEVEL_MIN		1
#define DEBUG_LEVEL_MAX		3

#define PORT    11910

enum cmd1_e {
  CMD1_RECEIVE = 0x80,
  CMD1_SEND = 0x01
};

enum cmd2_e {
  CMD2_COODINATOR_START = 0x16,
  CMD2_LOAD_STATUS = 0x17,
  CMD2_SENSOR_REG = 0x18,
  CMD2_SENSOR_DEREG = 0x19,
  CMD2_APP_DATA = 0x20,
  CMD2_LED_REMOTE_SWITCH = 0x90
};

typedef struct forward_pattern_s {
  unsigned char start;
  unsigned char len;   /* length of payload */
  unsigned char cmd1;
  unsigned char cmd2;
  unsigned char payload[];
}forward_pattern_t;

typedef struct forward_control_s {
  int index;  /* record index of test cases */
  char *str;
  forward_pattern_t *pattern;
}forward_control_t;

typedef struct device_address_s {
  int addr1;
  int addr2;
}device_address_t;

#endif /* __FORWARD_CLIENT_H */
