#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  int port = 9090;

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
  {
    printf("unable to bind to port %d\n", port);
    return -1;
  }
  printf("listening on port %d\n", port);

  listen(sock, 1);
  int client_sock;

  int client_fd;
  char msg[255];

  while(1) 
  {
    client_fd = accept(sock, NULL, NULL);
    recv(client_fd, msg, sizeof(msg), 0);

    printf("received: %s\n", msg);
    send(client_fd, msg, sizeof(msg), 0);
    close(client_fd);
  }

  return 0;
}
