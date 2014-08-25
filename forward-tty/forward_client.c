
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include "parse_pkt.h"
#include "forward_client.h"

#define LENGTH PKT_BUF_SIZE

#define is_coodinator_start_ind(pkt) ((pkt[2]==CMD1_RECEIVE) && (pkt[3] == CMD2_COODINATOR_START))
#define is_load_status_ind(pkt) ((pkt[2]==CMD1_RECEIVE) && (pkt[3] == CMD2_LOAD_STATUS))
#define is_sensor_reg_ind(pkt) ((pkt[2]==CMD1_RECEIVE) && (pkt[3] == CMD2_SENSOR_REG))
#define is_sensor_dereg_ind(pkt) ((pkt[2]==CMD1_RECEIVE) && (pkt[3] == CMD2_SENSOR_DEREG))

static int fd = -1;
static int debug = 0;
static device_address_t device_addr = {0, 0};

static int program_printf(const char *format, ...)
{
	va_list args;
	int printed = 0;

	va_start(args, format);
	fprintf(stderr, "[" PROGRAM "] ");
	printed = vprintf(format, args);
	va_end(args);

	return printed;
}

static int error_printf(const char *format, ...)
{
	va_list args;
	int printed = 0;

	va_start(args, format);
	fprintf(stderr, "[" ERROR "] ");
	printed = vprintf(format, args);
	va_end(args);

	return printed;
}

static int debug_printf(const char *format, ...)
{
	va_list args;
	int printed = 0;

	if (debug >= DEBUG_LEVEL_MIN) {
		va_start(args, format);
		fprintf(stderr, "[" DEBUG "] ");
		printed = vprintf(format, args);
		va_end(args);
	}

	return printed;
}
  
void sig_pipe(int signo)  
{  
    program_printf("Catch a signal\n");  
    if(signo == SIGTSTP)  
    {  
        program_printf("receive SIGTSTP\n");  
        int ret = close(fd);  
        if(ret == 0)  
            debug_printf("sig_pipe: close socket success\n");  
        else if(ret ==-1 )  
            error_printf("sig_pipe: close socket failed\n");  
        exit(1);  
    }  
}  


static void switch_led(int fd)
{
  unsigned int length = 0;
  unsigned int size = 0;
  unsigned int i;
  forward_pattern_t *pattern = NULL;
  unsigned char *pkt;
 
  /* => case 1: led_remote_switch_req */
  length = 0x2;
  size = sizeof(forward_pattern_t) + length * sizeof(unsigned char) + 1;
  pattern = (forward_pattern_t *)malloc(size);
  pattern->start= SOI;
  pattern->len = length;
  pattern->cmd1 = CMD1_SEND;
  pattern->cmd2 = CMD2_LED_REMOTE_SWITCH;
  pattern->payload[0] = device_addr.addr1;
  pattern->payload[1] = device_addr.addr2;
  pattern->payload[length] = EOI;

  /* send packets to tx buffer */
  pkt = (unsigned char *)pattern;
  write(fd, pattern, size);
  fprintf(stderr, "\nwrite %d bytes to server(fd = %d):", size, fd);
  for (i = 0; i < size; i++)
    fprintf(stderr, " 0x%02x", *(unsigned char *) (pkt + i));
  fprintf(stderr, "\n");

  free(pattern);
}


static void handle_message(int fd)
{
	int size;
	char buffer[LENGTH];

	/* read from console, write to server */
	memset(buffer, '\0', LENGTH);
	size = read(0, buffer, LENGTH);
	if ((strncmp(buffer, "quit", 4) == 0) ||
	     (strncmp(buffer, "exit", 4) == 0)) {
		write(fd, buffer, size);
		close(fd);
		fprintf(stderr, "\nexit now!\n");
		exit(EXIT_SUCCESS);
	}
	switch_led(fd);
}

static void on_received_pkt(unsigned char *pkt, int size)
{
        int i;
	forward_pattern_t *pattern = (forward_pattern_t *)pkt;

        fprintf(stderr, "\non_received_pkt: receive %d pkt, content: ", size);
        for (i = 0; i < size; i++)
        	fprintf(stderr,  " 0x%02x", *(unsigned char *) (pkt + i));
        fprintf(stderr, "\n");

        if(is_coodinator_start_ind(pkt)) {
        	fprintf(stderr,  "on_received_pkt: is_coodinator_start_ind\n");
        } 
        
        if(is_load_status_ind(pkt)) {
 		fprintf(stderr,  "on_received_pkt: is_load_status_ind\n");
        	device_addr.addr1 = pattern->payload[8];
        	device_addr.addr2 = pattern->payload[9];
 		fprintf(stderr, "64 bit address(hex): ");
		for (i = 0; i < 8; i++)
			fprintf(stderr, "0x%02x ", pattern->payload[i]);
 		fprintf(stderr, "\n16 bit address(hex): 0x%02x 0x%02x\n", device_addr.addr1, device_addr.addr2);
        }

        if(is_sensor_reg_ind(pkt)) {
 		fprintf(stderr,  "on_received_pkt: is_sensor_reg_ind\n");
        	device_addr.addr1 = pattern->payload[0];
        	device_addr.addr2 = pattern->payload[1];
 		fprintf(stderr, "16 bit address(hex): 0x%02x 0x%02x\n", device_addr.addr1, device_addr.addr2);
        }

}


static int receive_message(int fd)
{
  unsigned int size = 0;
  unsigned char buffer[LENGTH];

  if((size = read(fd, buffer, LENGTH))>0)
    parse_pkt(buffer, size, on_received_pkt);

  return 0;
}

static void process_conn_client(int fd)
{
	fprintf(stderr, "enter process_conn_client()\n");
	
	for (;;) {
		fd_set read_set;
		int numReady;

		FD_ZERO(&read_set);
		FD_SET(fd, &read_set);
		FD_SET(0, &read_set);		

		fprintf(stderr, "input 'quit' or 'exit' to exit!\npress any key to continue ->");

		numReady = select(FD_SETSIZE, &read_set, 0, 0, 0);
		if (numReady && FD_ISSET(fd, &read_set)) {
				receive_message(fd);
		}
		
		if (numReady && FD_ISSET(0, &read_set)) {
				handle_message(fd);
		}
	}
}

static int create_forward_client(void)
{
	int fd = -1;
	struct sockaddr_in server_addr;
	socklen_t addrlen;
	int err;
	char server_ip[50] = "";

	/********************socket()*********************/
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		error_printf("create socket error\n");
		return -1;
	}

	/*******************install signal()*********************/
	   if(SIG_ERR == signal(SIGTSTP,sig_pipe))  
	    {  
	        error_printf("signal install failed\n");  
	        return -1;  
	    }  
	    else  
	        debug_printf("signal install sucess\n") ;  


	/*******************connect()*********************/
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	fprintf(stderr, "please input server ip address : ");
	read(0, server_ip, 50);
	fprintf(stderr, "server ip address: %s", server_ip);
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	addrlen = sizeof(server_addr);
	err = connect(fd, (struct sockaddr *) &server_addr, addrlen);
	if (err == 0) {
		debug_printf("connect to server\n");
	} 
	else if (err < 0) {
		error_printf("couldn't connect to remote shell on port %d\r\n", server_addr.sin_port);
		return -1;
	}

	return fd;
}

static void usage(char *program_name)
{
	printf("Usage: %s [options]\n", program_name);
	printf("Options:\n"
		   "    -h		: help (this text)\n"
		   "    -d level	: specify debug level\n");
	exit(EXIT_FAILURE);
}

static void parse_command_line(int argc, char *argv[])
{
	int c;
	char *debug_level;
	opterr = 0;
	while ((c = getopt(argc, argv, "d:h")) != -1) {
		switch (c) {
		case 'd':
			debug_level = optarg;
			debug = atoi(debug_level);
			break;
		case 'h':
			usage(argv[0]);
			break;
		case '?':
			if (optopt == 't')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			exit(EXIT_SUCCESS);
			break;
		default:
			abort();
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	program_printf("version %s\n", VERSION);
	
	parse_command_line(argc, argv);

	fd = create_forward_client();
	if (fd < 0) {
	     error_printf("create_forward_client() error \n");
	     exit(EXIT_FAILURE);
	}

	process_conn_client(fd);
	
	close(fd);
	
	return (EXIT_SUCCESS);
}


