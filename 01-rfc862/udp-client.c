#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT_NUMBER 9090

int main(int argc, char *argv[])
{
  char msg[BUFFER_SIZE] = "This is such a cool day";
  char echo[BUFFER_SIZE];
  struct sockaddr_in server_addr;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  /*inet_pton(AF_INET, "192.168.100.44", &(server_addr.sin_addr));*/
  server_addr.sin_port = htons(PORT_NUMBER);

  // Send request
  sendto(fd, msg, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // Receive echo
  recvfrom(fd, echo, BUFFER_SIZE, 0, NULL, NULL);
  printf("echo: %s\n", echo);

  close(fd);
  return 0;
}
