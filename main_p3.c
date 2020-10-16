#include "user_project3.h"

struct mmap_info all_page[100]; // Max number of pages

int main(int argc, char* argv[])
{
	int sfd = 0, data;
	struct mmap_info k;
	void* handl;
	pthread_t thr;
	char user_input[40];
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

	all_pages();

	printf("[*] Pairing...\n");

	if (data % 2 == 0) {
		sfd = connect_server(atoi(argv[1]), &k);
	}
	if (data % 2 == 1) {
		sfd = connect_client(atoi(argv[2]), &k);
		mmap(k.mmap_addr, k.length, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}

	fault_region(&k, &handl, &thr);
	assign_addr_to_pages((uint64_t)k.mmap_addr);

	for (;;) {
		printf("\nWhich command should I run? (r:read, w:write, or x:exit): ");
		if (!fgets(user_input, 40, stdin))
			errExit("fgets error");
		if (!strncmp(user_input, "r", 1)) {
			to_read();
		}
		else if (!strncmp(user_input, "w", 1)) {
			to_write();
		}
		else if (!strncmp(user_input, "x", 1)) {
			break;
		}
	}

	printf("Exiting\n");
	return 0;
}