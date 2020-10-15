#ifndef __USER_PROJECT3_H_
#define __USER_PROJECT3_H_
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <linux/userfaultfd.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);	\
	} while (0)

struct user_args {
	long uffd;
	uint64_t pre_addr;
};

struct map_info {
	uint64_t length;
	void* mem_addr;
};

struct info_mem {
	uint64_t addr;
	uint64_t size;
};

struct socket_args {
	uint64_t length;
	uint64_t mem_address;
	int soc;
};

void* fault_handler_thread(void* arg);
long fault_region(struct map_info* k, void** start_handle, pthread_t* thr);

void* socket_handler_thread(void* arg);

static void all_pages();

int connect_server(int port);
int connect_client(int port, struct map_info* k);
void delay(int secs);
void to_read();
void to_write(int port);
void equate_(uint64_t addr);
#endif /* end of include guard: __USER_PROJECT3_H_ */
