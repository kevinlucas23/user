#include "user_project3.h"

struct mmap_info all_page[100]; // Max number of pages
extern unsigned long num_pages;
int main(int argc, char* argv[])
{
	int out = 0, data;
	struct mmap_info k;
	void* handl;
	pthread_t thr;
	char user_in[40];
	FILE* fptr;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s port remote_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((fptr = fopen("data.txt", "r")) == NULL) {
		errExit("Error! opening file");
		// Program exits if the file pointer returns NULL.
	}
	out = fscanf(fptr, "%d", &data);
	fclose(fptr);

	if ((fptr = fopen("data.txt", "w")) == NULL) {
		errExit("Error! opening file");
		// Program exits if the file pointer returns NULL.
	}
	out = data + 1;
	fprintf(fptr, "%d", out);
	fclose(fptr);

	printf(" [*] Pairing...\n");

	if (data % 2 == 0) {
		out = connect_server(atoi(argv[1]), &k);
	}
	if (data % 2 == 1) {
		out = connect_client(atoi(argv[2]), &k);
		mmap(k.mmap_addr, k.length, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}

	all_pages();

	fault_region(&k, &handl, &thr);
	assign_addr_to_pages((uint64_t)k.mmap_addr, num_pages);

	for (;;) {
		printf("\nWhich command should I run? (r:read, w:write, or x:exit): ");
		if (!fgets(user_in, 40, stdin))
			errExit("fgets error");
		if (!strncmp(user_in, "r", 1)) {
			to_read();
		}
		else if (!strncmp(user_in, "w", 1)) {
			to_write();
		}
		else if (!strncmp(user_in, "x", 1)) {
			break;
		}
	}

	printf("Exiting\n");
	return 0;
}