#include "user_project3.h"

unsigned long num_pages; // Stores the number of pages given from the user.
extern struct mmap_info all_page[];

void delay(int secs)
{
	int milli_seconds = 1000 * secs;
	clock_t start_time = clock();

	while (clock() < start_time + milli_seconds);
}

void to_read()
{
	char user_in[20];
	char* c;
	unsigned long num, i = 0;

	printf("For which page do you want to read? (0-%d, or -1 for all): ", ((int)num_pages - 1));
	if (!fgets(user_in, 20, stdin))
		errExit("fgets error");
	num = strtoul(user_in, NULL, 0);

	if ((int)num == -1) {
		 while (i < num_pages) {
			c = (char*)all_page[(int)i].mmap_addr;
			if (c == NULL) {
				printf(" [*] Page %lu: \n", i);
			}
			else {
				printf(" [*] Page %lu: \n%s\n", i, c);
			}
			i++;
		 }
	}
	else if(num < num_pages){
		c = (char*)all_page[(int)num].mmap_addr;
		if (c == NULL) {
			printf(" [*] Page :\n");
		}
		else {
			printf(" [*] Page %lu: \n%s\n", num, c);
		}
	}
}

void to_write()
{
	char user_in[20] = { 0 }, user_out[40] = { 0 };
	unsigned long num, i = 0;

	printf("For which page do you like to write to? (0-%d, or -1 for all): ", ((int)num_pages - 1));
	if (!fgets(user_in, 20, stdin))
		errExit("fgets error");
	num = strtoul(user_in, NULL, 0);

	printf("What would you like to write?: ");
	if (!fgets(user_out, 20, stdin))
		errExit("fgets error");
	
	if ((int)num == -1) {
		while (i < num_pages) {
			memcpy(all_page[(int)i].mmap_addr, user_out, strlen(user_out));
			i++;
		}
	}
	else if (num < num_pages) {
		memcpy(all_page[(int)num].mmap_addr, user_out, strlen(user_out));
	}
	else {
		printf("\nout of page range");
	}
}

void assign_addr_to_pages(uint64_t addr)
{
	int i = 0;
	uint64_t page = addr;
	int size_p = sysconf(_SC_PAGE_SIZE);
	for (i = 0; i < 100; ++i, page += size_p) {
		all_page[i].mmap_addr = (void*)page;
	}
}

void* fault_handler_thread(void* arg)
{
	static struct uffd_msg msg;   /* Data read from userfaultfd */
	long uffd;                    /* userfaultfd file descriptor */
	struct user_args* kev = (struct user_args*)arg;
	struct uffdio_copy uffdio_copy;
	ssize_t nread;

	char* page = (char*)kev->u_addr;
	uffd = kev->uffd;
	
	for (;;) {
		struct pollfd pollfd;
		int nready;
		pollfd.fd = uffd;
		pollfd.events = POLLIN;
		nready = poll(&pollfd, 1, -1);
		if (nready == -1)
			errExit("poll");

		nread = read(uffd, &msg, sizeof(msg));
		if (nread == 0) {
			printf("EOF on userfaultfd!\n");
			exit(EXIT_FAILURE);
		}

		if (nread == -1)
			errExit("read");

		if (msg.event != UFFD_EVENT_PAGEFAULT) {
			fprintf(stderr, "Unexpected event on userfaultfd\n");
			exit(EXIT_FAILURE);
		}

		uffdio_copy.src = (unsigned long)page;
		uffdio_copy.dst = (unsigned long)msg.arg.pagefault.address &
			~(sysconf(_SC_PAGE_SIZE) - 1);
		uffdio_copy.len = sysconf(_SC_PAGE_SIZE);
		uffdio_copy.mode = 0;
		uffdio_copy.copy = 0;

		if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
			errExit("ioctl-UFFDIO_COPY");
		printf("\n [X] PAGEFAULT on address (%p) \n", msg.arg.pagefault.address);
	}
}

long fault_region(struct mmap_info* k, void** start_handle, pthread_t* thr)
{
	long uffd;          /* userfaultfd file descriptor */
	struct uffdio_api uffdio_api;
	struct uffdio_register uffdio_register;
	int s;
	struct user_args* kev = (struct user_args*)malloc(sizeof(struct user_args));

	*start_handle = mmap(NULL, k->length, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (*start_handle == MAP_FAILED)
		errExit("mmap");
	memset(*start_handle, 0, k->length);

	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	kev->u_addr = (uint64_t)*start_handle;
	kev->uffd = uffd;
	if (uffd == -1) {
		errExit("userfaultfd");
	}	

	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
		errExit("ioctl-UFFDIO_API");

	uffdio_register.range.start = (unsigned long)k->mmap_addr;
	uffdio_register.range.len = k->length;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		errExit("ioctl-UFFDIO_REGISTER");

	s = pthread_create(thr, NULL, fault_handler_thread, (void*)kev);
	if (s != 0) {
		errno = s;
		errExit("pthread_create");
	}
	return uffd;
}

void all_pages()
{
	int j;
	for (j = 0; j < 100; ++j) {
		all_page[j].mmap_addr = NULL;
	}
}

int connect_server(int port, struct mmap_info* k)
{
	int sockfd, connfd, length, reaa;
	struct sockaddr_in saddr;
	char buff[20];
	struct data_to kev;
	void* map_t;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		errExit("Socket creation failed....\n");
	}
	memset(&saddr, 0, sizeof(saddr));

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);

	if ((bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr))) != 0) {
		errExit("Socket server bind failed...\n");
	}

	if ((listen(sockfd, 12)) != 0) {
		errExit("Socket server listen failed...\n");
	}

	connfd = accept(sockfd, NULL, NULL);
	if (connfd < 0) {
		errExit("Socket server accept failed..\n");
	}

	printf("[*] Paired!\n");

	reaa = read(connfd, &buff, sizeof(buff));
	if (reaa < 0)
		errExit("Can't read");
	num_pages = strtoul(buff, NULL, 0);
	length = strtoul(buff, NULL, 0) * sysconf(_SC_PAGE_SIZE);
	map_t = mmap(NULL, length, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (map_t == MAP_FAILED)
		errExit("mmap");

	printf("mmap addr: %p, and length: %d\n", map_t, length);
	kev.addr = (uint64_t)map_t;
	kev.size = length;
	k->mmap_addr = (void*)kev.addr;
	k->length = kev.size;
	reaa = write(connfd, &kev, sizeof(kev));
	if (reaa < 0)
		errExit("Can't write");
	close(sockfd);
	return connfd;
}

int connect_client(int port, struct mmap_info* k)
{
	int sockfd, reaa;
	struct sockaddr_in saddr;
	struct data_to kev;
	char buff[20];

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		errExit("Socket client creation failed....\n");
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(port);

	for (;;) {
		if (connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) == 0) {
			break;
		}
		printf("[-] Connection Failed\n\n");
		delay(1500);
	}

	printf("How many pages would you like to allocate? (greater than 0): ");
	if (!fgets(buff, 20, stdin)) {
		errExit("error getting input");
	}
	num_pages = strtoul(buff, NULL, 0);
	reaa = write(sockfd, &buff, sizeof(buff));
	if (reaa < 0)
		errExit("Can't write");

	reaa = read(sockfd, &kev, sizeof(kev));
	if (reaa < 0)
		errExit("Can't read");

	printf("Request received addr: 0x%lx, and length: %lu\n", kev.addr, kev.size);
	k->mmap_addr = (void*)kev.addr;
	k->length = kev.size;
	close(sockfd);
	return sockfd;
}