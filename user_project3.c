#include "user_project3.h"
  
void delay(int secs)
{
  int milli_seconds = 1000 * secs;
  clock_t start_time = clock();
  
  while(clock() < start_time + milli_seconds);
}

void* fault_handler_thread(void *arg)
{
  static struct uffd_msg msg;   /* Data read from userfaultfd */
  long uffd;                    /* userfaultfd file descriptor */
  struct uffdio_copy uffdio_copy;
  ssize_t nread;
  static int fault_cnt = 0;
  struct user_args* kev = (struct user_args*)arg;
  char* page = (char*)kev->pre_addr;
  uffd = kev->uffd;
  // static char *page = NULL;
  // uffd = (long)arg;
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
    fault_cnt++;
    uffdio_copy.src = (unsigned long) page;
    uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
			~(sysconf(_SC_PAGE_SIZE)- 1);
    uffdio_copy.len = sysconf(_SC_PAGE_SIZE);
    uffdio_copy.mode = 0;
    uffdio_copy.copy = 0;

    if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
	errExit("ioctl-UFFDIO_COPY");
    printf("\n[%p]PAGEFAULT\n", (void *)msg.arg.pagefault.address);
  }
}

long fault_region(struct map_info* k, void** start_handle, pthread_t* thr)
{
  long uffd;          /* userfaultfd file descriptor */
  struct uffdio_api uffdio_api;
  struct uffdio_register uffdio_register;
  int s;
  struct user_args* kev = (struct user_args*)malloc(sizeof(struct user_args));

  uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
  if (uffd == -1){
  errExit("userfaultfd");
  }

  *start_handle = mmap(NULL, k->length, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (*start_handle == MAP_FAILED)
	errExit("mmap");
  memset(*start_handle, 0, k->length);

  kev->pre_addr = (uint64_t)*start_handle;
  kev->uffd = uffd;

  uffdio_api.api = UFFD_API;
  uffdio_api.features = 0;
  if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
    errExit("ioctl-UFFDIO_API");

  uffdio_register.range.start = (unsigned long) k->mem_addr;
  uffdio_register.range.len = k->length;
  uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
  if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
 	errExit("ioctl-UFFDIO_REGISTER");

  s = pthread_create(thr, NULL, fault_handler_thread, (void *) kev);
  if (s != 0) {
	errno = s;
        errExit("pthread_create");
  }
  return uffd;
}

void* socket_handler_thread(void *arg)
{
  /*struct socket_args* sargs = arg;
  int s;

  if(!sargs)
    errExit("Socket Null");

  pthread_cleanup_push(socket_handler_thread, &sargs->soc);

  for(;;){

  }*/
  return (void *)0;
}

int connect_server(int port)
{
    int sockfd, connfd, length, reaa;
  struct sockaddr_in saddr;
  char buff[100];
    struct info_mem kev;
  void* map_t;
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sockfd == -1){
    errExit("Socket creation failed....\n"); 
  }
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(port);
  if((bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr))) != 0){
    errExit("Socket server bind failed...\n");
  }
  
  if((listen(sockfd, 12)) != 0){
    errExit("Socket server listen failed...\n");
  }
  
  printf("Waiting for connections...\n");
  
  connfd = accept(sockfd, NULL, NULL);
  if(connfd < 0){
    errExit("Socket server accept failed..\n");
  }
  
  printf("Connected Successfully.\n");
  printf("How many pages do you want to mmap?: ");
  if(!fgets(buff, 100, stdin)){
    errExit("error getting input");
  }
  
  length = strtoul(buff, NULL, 0) * sysconf(_SC_PAGE_SIZE);
  map_t = mmap(NULL, length, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (map_t == MAP_FAILED)
	errExit("mmap");

  printf("mmap addr: %p, and length: %d\n", map_t, length);
  kev.addr = (uint64_t)map_t;
  kev.size = length;
  reaa = write(connfd, &kev, sizeof(kev));
  if (reaa < 0)
      errExit("Can't write");
  close(sockfd);
  
  return connfd;
}

int connect_client(int port)
{
  int sockfd, reaa;
  struct sockaddr_in saddr;
  struct info_mem kev;
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sockfd == -1){
    errExit("Socket client creation failed....\n"); 
  }
  
  printf("Connecting to: %d\n", port);
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  saddr.sin_port = htons(port);
  
  /*if(connect(sockfd,(struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
    printf("[-] Connecting to server\n");
    return -1;
  }*/
  for(;;){
   if(connect(sockfd,(struct sockaddr *)&saddr, sizeof(saddr)) == 0){
     break;
   }
   printf("[-] Connection Failed\n\n");
   delay(1000);
  }
  
  printf("Waiting for inputs\n");
  reaa = read(sockfd, &kev, sizeof(kev));
  if(reaa < 0)
    errExit("Can't read");
  printf("Request received addr: 0x%lx, and length: %lu\n", kev.addr, kev.size);
  close(sockfd);
  return sockfd;
}
