#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
  time_t t;
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

  while(1) 
  {
    client_fd = accept(sock, NULL, NULL);
    printf("connected\n");
    time(&t);
    if (send(client_fd, &t, 1024, 0) < 0) {
      printf("unable to send time\n");
    } else {
      printf("time sent successfully\n");
    }
    close(client_fd);
  }

  return 0;
}
