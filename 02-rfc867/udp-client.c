#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define PORT_NUMBER 9090

int main(int argc, char *argv[])
{
  char msg[0] = "";
  time_t echo;
  struct sockaddr_in server_addr;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT_NUMBER);

  // Send request
  sendto(fd, msg, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // Receive echo
  recvfrom(fd, &echo, sizeof(echo), 0, NULL, NULL);
  printf("Time: %s", asctime(localtime(&echo)));

  close(fd);
  return 0;
}
