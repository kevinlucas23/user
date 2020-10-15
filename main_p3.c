#include "user_project3.h"

extern struct info_mem kev;
int main(int argc, char *argv[])
{
  int sfd = 0;
  // char buff[80];
  if(argc != 3) {
    fprintf(stderr, "Usage: %s port remote_port\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  printf("[*] Pairing...\n");
  sfd = connect_client(atoi(argv[2]));
  if(sfd > 0){
      
  }
  else{
     sfd = connect_server(atoi(argv[1]));
  }
  printf("Exiting\n");
  return 0;
}
