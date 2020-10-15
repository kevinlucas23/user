#include "user_project3.h"

extern struct info_mem kev;
int main(int argc, char* argv[])
{
	int sfd = 0, data;
	struct map_info k;
	void* handl;
	pthread_t thr;
	FILE* fptr;
	if ((fptr = fopen("data.txt", "r")) == NULL) {
		errExit("Error! opening file");
		// Program exits if the file pointer returns NULL.
	}
	sfd = fscanf(fptr, "%d", &data);
	fclose(fptr);

	if ((fptr = fopen("data.txt", "w")) == NULL) {
		errExit("Error! opening file");
		// Program exits if the file pointer returns NULL.
	}
	sfd = data + 1;
	fprintf(fptr, "%d", sfd);
	fclose(fptr);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s port remote_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("[*] Pairing...\n");

	if (data % 2 == 0) {
		sfd = connect_server(atoi(argv[1]));
	}
	if (data % 2 == 1) {
		sfd = connect_client(atoi(argv[2]), &k);
		mmap(k.mem_addr, k.length, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}
	
	fault_region(k, &handl, &thr);
	printf("Exiting\n");
	return 0;
}