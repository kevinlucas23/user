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

enum MSI_I {
	invalidate = 0,
	modified,
	shared
};

enum MSI_O {
	page_invalid = 0,
	page_request,
	page_reply,
	end_erything,
	left
};

struct msi_info
{
	void* mmap_addr;
	enum MSI_I protocol;
	pthread_mutex_t mutex;
};

struct user_args {
	long uffd;
	uint64_t u_addr;
	int ki;
};

struct mmap_info {
	uint64_t length;
	void* mmap_addr;
	int kc;
};

struct data_to {
	uint64_t addr;
	uint64_t size;
};

struct check_info
{
	struct data_to in_msi;
	enum MSI_O a_mess;
	char inside[4096];
};

struct sock_args {
	int soc;
	struct data_to info;
};

// For faulting
void* fault_handler_thread(void* arg);
long fault_region(struct mmap_info* k, void** start_handle, pthread_t* thr);
void page_fautl(int k, char* page, void* fault_addr, unsigned int rw);

// For socket thread
void thread_socket_handler(void* arg);
void* thread_socket(void* arg);

// For my pages
void all_pages();

// Connection between server and client.
int connect_server(int port, struct mmap_info* k, struct sock_args* luc);
int connect_client(int port, struct mmap_info* k, struct sock_args* luc);

// To keep the connection active
void delay(int secs);

// To read, view and write given the commands
void to_read();
void to_write(int k);
void to_msi();
void assign_addr_to_pages(uint64_t addr, int pa);

struct msi_info* getpage(void* addresses);
void request_a_page(int k, struct check_info* kev);
void invalidate_a_page(int k, struct check_info* kev);
void reply_to_page(int k, struct check_info* kev);
#endif /* end of include guard: __USER_PROJECT3_H_ */
