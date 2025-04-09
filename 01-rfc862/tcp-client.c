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

  if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    printf("Error....\n");
  }
  else {
    char msg[255];
    printf("Your message: ");
    scanf("%19[^\n]", msg);
    char recv_msg[255];
    send(sock, msg, sizeof(msg), 0);
    recv(sock, recv_msg, sizeof(recv_msg), 0);
    printf("Server echo: %s\n", recv_msg);
  }

  return 0;
}
