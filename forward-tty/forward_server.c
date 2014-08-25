#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <ctype.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ini_file.h"
#include "forward_server.h"

static int debug = 0;

static char *tty_name = DEFAULT_TTY_NAME;
static char *tty_config = DEFAULT_TTY_CONFIG;
static int ttyfd = -1;
static int inputfd = -1;
static int outputfd = -1;
static int serverfd = -1;
static int clientfd = -1;
static struct termios origtio;

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

static void create_user_file(const char *file)
{
	FILE *fp;
	fp = fopen(file, "w+");
	if (!fp) {
		error_printf("can't open file: %s\n", file);
		return;
	}
	if (0 != fclose(fp))
		error_printf("Note: couldn't close file: %s correctly\n", file);
}

static void create_config_file(const char *file)
{
	FILE *fp;
	fp = fopen(file, "w+");
	if (!fp) {
		error_printf("can't open file: %s\n", file);
		return;
	}

	fprintf(fp, "MODE=RAW\r\n");
	fprintf(fp, "BAUDRATE=115200\r\n");
	fprintf(fp, "DATA_BITS=8\r\n");
	fprintf(fp, "PARITY='N'\r\n");
	fprintf(fp, "STOP_BITS=1\r\n");

	if (0 != fclose(fp))
		error_printf("create_config_file: close file: %s failed!\n", file);
}

static int read_config_file(serial_control_t *ctrl)
{
	char mode[10], baudrate[20], bits[10], parity[10], stop[10];

	if (read_conf_value("MODE", mode, tty_config) != 0) {
		error_printf("serial_config: get MODE failed\n");
		return EXIT_FAILURE;
	}

	if (read_conf_value("BAUDRATE", baudrate, tty_config) != 0) {
		error_printf("serial_config: get BAUDRATE failed\n");
		return EXIT_FAILURE;
	}

	if (read_conf_value("DATA_BITS", bits, tty_config) != 0) {
		error_printf("serial_config: get DATA_BITS failed\n");
		return EXIT_FAILURE;
	}

	if (read_conf_value("PARITY", parity, tty_config) != 0) {
		error_printf("serial_config: get PARITY failed\n");
		return EXIT_FAILURE;
	}

	if (read_conf_value("STOP_BITS", stop, tty_config) != 0) {
		error_printf("serial_config: get STOP_BITS failed\n");
		return EXIT_FAILURE;
	}

	if ((strcmp(mode, "RAW") == 0)||(strcmp(mode, "raw") == 0)) {
		ctrl->mode = RAW_MODE;
	} else {
		ctrl->mode = NORMAL_MODE;
	}
	
	ctrl->speed = STRTOL(baudrate);
	ctrl->bits = STRTOL(bits);
	ctrl->parity = STRTOL(parity);
	ctrl->stop = STRTOL(stop);

	debug_printf("read_config_file ->  %s\n",  tty_config);
	debug_printf("----------------------------------\n");
	debug_printf("serial_config: MODE=%s\n", mode);
	debug_printf("serial_config: BAUDRATE=%d\n", ctrl->speed);
	debug_printf("serial_config: DATA_BITS=%d\n", ctrl->bits);
	debug_printf("serial_config: PARITY=%s\n", parity);
	debug_printf("serial_config: STOP_BITS=%d\n", ctrl->stop);
	debug_printf("----------------------------------\n");

	return EXIT_SUCCESS;
}

static int serial_setup(serial_control_t *ctrl)
{
	/* just setup serial at debug level= 0, 1, 4, 
	    for debug level = 2, 3 is reserved for simulation test locally */
	if ((debug <2) || (debug >3)) {
		struct termios newtio, oldtio;

		/*
		 * Here we setup the standard input tty settings. We have to do it here,
		 * because we need to restore them when this application exits (so that the Linux
		 * shell continues to work as expected.)
		 * The tty settings of the external tty are set in the remote build.
		 */
		if (tcgetattr(ctrl->fd, &oldtio) != 0) {
			error_printf("tcgetattr failed: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		memset(&newtio, 0, sizeof(newtio));
		memcpy(&newtio, &oldtio, sizeof(struct termios));

		if (ctrl->mode == RAW_MODE) {
			/* Set raw mode: the remote application will handle all terminal characters */
			cfmakeraw(&newtio);
		}

		switch (ctrl->bits) {
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
		}
		switch (ctrl->parity) {
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E':
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
		}
		switch (ctrl->speed) {
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		}

		if (ctrl->stop == 1)
			newtio.c_cflag &= ~CSTOPB;
		else if (ctrl->stop == 2)
			newtio.c_cflag |= CSTOPB;

		// important
		newtio.c_cflag |= CLOCAL | CREAD;
		newtio.c_cflag &= ~CSIZE;
		if (ctrl->mode == RAW_MODE) {
			newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
			newtio.c_oflag &= ~OPOST;
			newtio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		}

		newtio.c_cc[VTIME] = 0;
		newtio.c_cc[VMIN] = 0;

		tcflush(ctrl->fd, TCIFLUSH);

		if (tcsetattr(ctrl->fd, TCSANOW, &newtio) < 0) {
			error_printf("tcsetattr failed: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		debug_printf("serial_control: setting has done!\n");
	}
	return EXIT_SUCCESS;
}

static int serial_open(char *name)
{
	int fd = 0, ft = 0;

	if ((fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
		error_printf("Can't open serial port %s: %s\n", name, strerror(errno));
		return EXIT_FAILURE;
	} else {
		debug_printf("Open serial port [%s] success, fd-open=%d\n", name, fd);
	}

	/* set serial to block status
	   wait for read from serial port */
	ft = fcntl(fd, F_SETFL, 0);
	if (-1 == ft)
		error_printf("fcntl failed!\n");
	else
		debug_printf("fcntl=%d\n", ft);

	if (isatty(fd) == 1)
		debug_printf("isatty(%d) = 1, [%s] is a terminal device\n", fd, name);
	else
		debug_printf("isatty(%d) = 0, [%s] is not a terminal device\n", fd, name);

	return fd;
}

static int serial_restore(int fd)
{
	if (tcsetattr(fd, TCSAFLUSH, &origtio) < 0) {
		error_printf("tcsetattr failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/* Only read the bytes readily available, even if less than count */
static ssize_t safety_read(int fd, void *buf, ssize_t count, char *errstr)
{
	ssize_t ret = 0;

	while ((ret = read(fd, buf, count)) != 0) {

		if (ret == -1) {
			if (errno == EINTR)
				/* just restart */
				continue;

			/* error */
			fprintf(stderr, "%s (%s) (fd=%d)\r\n", errstr, strerror(errno), fd);
			break;
		}

		break;
	}

	return ret;
}


static ssize_t safety_write(int fd, const void *buf, size_t count, char *errstr)
{
	ssize_t ret = 0;
	ssize_t len = count;

	while (len != 0 && (ret = write(fd, buf, len)) != 0) {

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "%s (%s) (fd=%d) (errno=%d)\r\n", errstr, strerror(errno), fd, errno);
			break;
		}

		len -= ret;
		buf += ret;
	}

	if (ret == -1)
		return ret;
	else
		return count - len;
}

static int forward_message(int from, int to)
{
	char buffer[64];
	int num_read, num_write;

	num_read = safety_read(from, buffer, sizeof(buffer), "[" PROGRAM "]: reading data failed");
	if (num_read < 0) {
		/* other end is not available anymore */
		error_printf("forward_message: no data available in fd %d, channel has closed.\n", from);
		return READ_FAILURE;
	} else if (num_read < 0) {
		return READ_FAILURE;
	}

	if ((strncmp(buffer, "quit", 4) == 0) || (strncmp(buffer, "exit", 4) == 0))
		return WRITE_FAILURE;

	if (debug > DEBUG_LEVEL_MAX) {
		int i;
		fprintf(stderr, "read %d bytes from %d:", num_read, from);
		for (i = 0; i < num_read; i++)
			fprintf(stderr, " %1$02x|%1$c", buffer[i]);
		fprintf(stderr, "\n");
	}

	num_write = safety_write(to, buffer, num_read, "[" PROGRAM "]: writing data failed");
	if (num_write < 0) {
		return WRITE_FAILURE;
	}

	return READ_WRITE_OK;
}

static void *forward_thread(void *argument)
{
	enum forward_type ret;
	thread_param_t thread_arg, *arg;

	thread_arg = *(thread_param_t *) argument;
	arg = &thread_arg;

	/* set thread name */
	debug_printf("\nforward_thread: arg->name = %s\n", arg->name);
	prctl(PR_SET_NAME, (unsigned int) arg->name);

	while (1) {
		fd_set read_set;
		int numReady;

		FD_ZERO(&read_set);
		FD_SET(arg->from, &read_set);

		numReady = select(FD_SETSIZE, &read_set, 0, 0, 0);
		if (numReady) {
			if (FD_ISSET(arg->from, &read_set)) {
				ret = forward_message(arg->from, arg->to);
				if (0 != ret)
					break;
			}
		}
	}

	/* check if use STDIN_FILENO or not */
	if (arg->from == STDIN_FILENO)
		(void) serial_restore(STDIN_FILENO);

	if (READ_FAILURE == ret)
		close(arg->from);
	else if (WRITE_FAILURE == ret)
		close(arg->to);

	pthread_exit((void *) arg->name);

	return NULL;
}

static int forward_create_thread(pthread_t *tid, thread_param_t *arg)
{
	int t;
	pthread_attr_t attr;
	struct sched_param param;

	pthread_attr_init(&attr);

	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	pthread_attr_setschedpolicy(&attr, DEF_SCHED_POLICY);

	pthread_attr_getschedparam(&attr, &param);
	param.sched_priority = DEF_PRIORITY;
	pthread_attr_setschedparam(&attr, &param);

	for (t = 0; t < NUM_THREADS; t++) {
		if (pthread_create(&tid[t], &attr, forward_thread, (void *) &arg[t])) {
			error_printf("forward_create_thread failed\n");
			return -1;
		}
	}

	pthread_attr_destroy(&attr);
	return 0;
}

static int forward_wait_thread(pthread_t *tid)
{
	int t;
	void *name[NUM_THREADS];

	for (t = 0; t < NUM_THREADS; t++) {
		if (0 != pthread_join(tid[t], &name[t])) {
			error_printf("pthread_join failed from thread: %s\n", (char *) name[t]);
			return -1;
		} else {
			debug_printf("pthread_join ok from thread: %s\n", (char *) name[t]);
			return 0;
		}
	}
	return 0;
}

static int forward_process(int fd, int ttyfd)
{
	pthread_t tid[NUM_THREADS];
	thread_param_t arg[NUM_THREADS] = {
		{
		 "fwd_send",		/* name */
		 fd,				/* from */
		 ttyfd			/* to */
		 },
		{
		 "fwd_receive",	/* name */
		 ttyfd,			/* from */
		 fd				/* to */
		 }
	};
	if (forward_create_thread(tid, arg) != 0) {
		error_printf("forward_create_thread error\n");
		if (0 != close(ttyfd))
			error_printf("Note: couldn't close file: %s correctly\n", tty_name);
		exit(EXIT_FAILURE);
	}

	if (forward_wait_thread(tid) != 0) {
		error_printf("forward_wait_thread error\n");
		exit(EXIT_FAILURE);
	}
	debug_printf("forward_process exit\n");
	return 0;
}

static void forward_test_case(int case_num)
{
	if ((debug == 2) || (debug == 3)) {
		if (case_num == 0) {
			char *user_input_name = DEFAULT_USER_INPUT_NAME;
			char *user_output_name = DEFAULT_USER_OUTPUT_NAME;

	              if (access(DEFAULT_USER_INPUT_NAME, F_OK) != 0) {
				create_user_file(user_input_name);
	                }
			debug_printf("create default user file (%s)\n", DEFAULT_USER_INPUT_NAME);
			inputfd = open(user_input_name, O_RDONLY);
			if (!inputfd) {
				error_printf("can't open file: %s\n", user_input_name);
				exit (EXIT_FAILURE);
			}
			
	              if (access(DEFAULT_USER_OUTPUT_NAME, F_OK) != 0) {
				create_user_file(user_output_name);
	               }
			debug_printf("create default user file (%s)\n", DEFAULT_USER_OUTPUT_NAME);
			outputfd = open(user_output_name, O_WRONLY);
			if (!outputfd) {
				error_printf("can't open file: %s\n", user_output_name);
				exit (EXIT_FAILURE);
			}
		}
		else if ((case_num == 1) && (debug == 2)) {
			debug_printf("Test case 1 => begin \n");
			debug_printf("from input file: %s to ttyfd: %s\n", DEFAULT_USER_INPUT_NAME, tty_name);
			forward_message(inputfd, ttyfd);
			debug_printf("from ttyfd: %s to output file: %s\n", tty_name, DEFAULT_USER_OUTPUT_NAME);	
			forward_message(ttyfd, outputfd);
			if (0 != close(inputfd))
				error_printf("Note: couldn't close file: %s correctly\n", DEFAULT_USER_INPUT_NAME);
			if (0 != close(outputfd))
				error_printf("Note: couldn't close file: %s correctly\n", DEFAULT_USER_OUTPUT_NAME);
			debug_printf("Test case 1 => end \n");
			exit (EXIT_SUCCESS);
		}
		else if ((case_num == 2) && (debug == 3)) {
			int err = 0;
			debug_printf("Test case 2 => begin \n");
			do {
				debug_printf("from input file: %s to socketfd: %d\n", DEFAULT_USER_INPUT_NAME, clientfd);
				err = forward_message(inputfd, clientfd);
				debug_printf("from socketfd: %d to output file: %s\n", clientfd, DEFAULT_USER_OUTPUT_NAME);	
				err = forward_message(clientfd, outputfd);
			} while (err == READ_WRITE_OK);
			close(serverfd);
		 	close(clientfd);
			if (0 != close(inputfd))
				error_printf("Note: couldn't close file: %s correctly\n", DEFAULT_USER_INPUT_NAME);
			if (0 != close(outputfd))
				error_printf("Note: couldn't close file: %s correctly\n", DEFAULT_USER_OUTPUT_NAME);
			debug_printf("Test case 2 => end \n");		
			exit(EXIT_SUCCESS);
		}
	}
}

static int forward_create_server(int *fd)
{
	/* ss is server fd */
	int ss;
	int err;
	struct sockaddr_in server_addr;
	socklen_t addrlen;

	/*****************socket()***************/
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0) {
		error_printf("server : create socket error\n");
		return -1;
	}

	/* set socket can be reused */
	int opt = SO_REUSEADDR;
	setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	/******************bind()****************/
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	addrlen = sizeof(server_addr);
	err = bind(ss, (struct sockaddr *) &server_addr, addrlen);
	if (err < 0) {
		error_printf("server : bind socket error\n");
		return -1;
	}

	/*****************listen()***************/
	err = listen(ss, BACKLOG);
	if (err < 0) {
		error_printf("server : listen error\n");
		return -1;
	}

	*fd = ss;
	return 0;
}

static int forward_accept_client(int fd, int ttyfd)
{
	int sc;
	pid_t pid;
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	/* parent process doesn't care whenever child process exit,
	    just ask kernel to recycle child process, don't send signal to parent,
	    which in order to avoid child process as Zombie process,  */
	signal(SIGCHLD, SIG_IGN);	

	while (1) {
		addrlen = sizeof(client_addr);
		/* accept return the fd of client */
		sc = accept(fd, (struct sockaddr *) &client_addr, &addrlen);
		if (sc < 0) {
			continue;
		} else {
			debug_printf("forward_server : connected\n");
		}

		if (debug ==3) {
			serverfd = fd;
			clientfd = sc;
		}
		forward_test_case(2);

		/* create one child process, handle communication between client and server */
		pid = fork();

		/* child process reture 0, parent process return pid of child process */
		if (pid == 0) {	/* child process */
			close(fd);
			if (forward_process(sc, ttyfd) == 0) {
				program_printf("child process exit now!\n");
				sleep(1);
				exit(EXIT_SUCCESS);
			}
		} else {		/* parent process */
			close(sc);
			continue;  //double check??? FIXME
		}
	}

	return 0;
}

static void usage(char *program_name)
{
	printf("Usage: %s [options]\n", program_name);
	printf("Options:\n"
		   "    -h		: help (this text)\n"
		   "    -c		: create default tty config file (%s)\n"
		   "    -t tty	: specify tty device (default: %s)\n"
		   "    -i		: create default user file (%s)\n"
		   "    -o		: create default user file (%s)\n"
		   "    -d level	: specify debug level\n",
		   DEFAULT_TTY_CONFIG, DEFAULT_TTY_NAME,
		   DEFAULT_USER_INPUT_NAME, DEFAULT_USER_OUTPUT_NAME);
	exit(EXIT_FAILURE);
}

static void parse_command_line(int argc, char *argv[])
{
	int c;
	char *debug_level;
	opterr = 0;
	while ((c = getopt(argc, argv, "t:d:cioh")) != -1) {
		switch (c) {
		case 't':
			tty_name = optarg;
			break;
		case 'c':
			create_config_file(DEFAULT_TTY_CONFIG);
			printf("Options:\n" "    -c        : create default tty config file (%s)\n", DEFAULT_TTY_CONFIG);
			exit(EXIT_SUCCESS);
			break;
		case 'i':
			create_user_file(DEFAULT_USER_INPUT_NAME);
			printf("create default user file (%s)\n", DEFAULT_USER_INPUT_NAME);
			exit(EXIT_SUCCESS);
			break;
		case 'o':
			create_user_file(DEFAULT_USER_OUTPUT_NAME);
			printf("create default user file (%s)\n", DEFAULT_USER_INPUT_NAME);
			exit(EXIT_SUCCESS);
			break;
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
	int serverfd = -1;
	serial_control_t ttyctl;

	printf("\n");
	program_printf("version %s\n", VERSION);

	parse_command_line(argc, argv);

	forward_test_case(0);

	ttyfd = serial_open(tty_name);
	memset(&ttyctl, 0, sizeof(serial_control_t));
	ttyctl.fd = ttyfd;

	if (access(DEFAULT_TTY_CONFIG, F_OK) != 0) {
		/* config file not exsit, just create it */		
		create_config_file(DEFAULT_TTY_CONFIG);
	}

	read_config_file(&ttyctl);
	if (serial_setup(&ttyctl) != EXIT_SUCCESS) {
		error_printf("serial_control error\n");
		return EXIT_FAILURE;
	}

	forward_test_case(1);

	if (forward_create_server(&serverfd) != 0) {
		error_printf("forward_create_server error\n");
		return EXIT_FAILURE;
	}

	if (forward_accept_client(serverfd, ttyfd) != 0) {
		error_printf("forward_accept_client error\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


