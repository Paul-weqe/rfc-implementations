#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define PORT_NUMBER 9090
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
  time_t t;
  int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  char msg[BUFFER_SIZE];
  struct sockaddr_in address, client_address;
  socklen_t client_length = sizeof(struct sockaddr);

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(PORT_NUMBER);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  socklen_t addrlen = sizeof(address);
  if (bind(server_fd, (const struct sockaddr *)&address, addrlen) < 0) {
    printf("unable to bind\n");
    return -1;
  }
  printf("listening on port %d\n", PORT_NUMBER);

  while(1) {
    // receive request. 
    recvfrom(server_fd, msg, 0, 0, (struct sockaddr *)&client_address, &client_length);

    // send response
    printf("received conn\n");
    time(&t);
    if (sendto(server_fd, &t, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, client_length) < 0)
    {
      printf("error sending time\n");
    } else {
      printf("send time\n");
    }
  }
  close(server_fd);
  return 0;
}
