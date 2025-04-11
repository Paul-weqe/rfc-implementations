#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "./client.h"

void level_1_prompt(char *msg) { printf("\x1b[32m\e[1m%s\x1b[0m", msg); }
void level_2_prompt(char *msg) { printf("\x1b[33m\e[3m  %s\x1b[0m", msg); }

int main(int argc, char *argv[]) {
  char filename[512], mode[10], input[512];
  in_addr_t address;
  int port;
  FILE *dest_file;

  level_1_prompt("TFTP Server Address details.\n");
  level_2_prompt("Ip address[default 127.0.0.1]: ");

  // ---------- fetch IP Address ------
  fgets(input, sizeof input, stdin);
  if (input[0] == '\n') {
    strcpy(input, "127.0.0.1");
  }
  address = inet_addr(input);
  if (address == -1) {
    printf("Invalid IP address\n");
    return -1;
  }

  // ----------- fetch port number ---------
  level_2_prompt("Port number[default 2000]: ");
  fgets(input, sizeof input, stdin);
  if (input[0] == '\n') {
    strcpy(input, "2000");
  }
  port = atoi(input);
  if (port == 0) {
    printf("Invalid port number\n");
    return -1;
  }

  level_1_prompt("Filename: ");
  scanf("%s", filename);

  level_1_prompt("Mode[octet/netascii]: ");
  scanf("%s", mode);

  if (strcmp(mode, "octet") == 0) {
    dest_file = fopen(filename, "wb");
  } else if (strcmp(mode, "netascii") == 0) {
    dest_file = fopen(filename, "w");
  } else {
    printf("invalid mode %s\n", mode);
    return -1;
  }

  struct sockaddr_in server_addr;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = address;
  server_addr.sin_port = htons(port);

  /*
   * 2 bytes        | string    | 1 byte  | string | 1 byte
   * ------------------------------------------------------
   *  RRQ/ | 01/02  | Filename  | 0       | Mode   | 0    |
   *  WRQ -------------------------------------------------
   *  2  bytes 2 bytes n bytes
   */
  char buffer[516];
  // keeps track of the position of the buffer that we are adding so far.
  int buffer_pos = 0;

  buffer[0] = 00;
  buffer[1] = 01;
  buffer_pos = 2;

  for (int i = 0; i < strlen(filename); i++, buffer_pos++) {
    buffer[buffer_pos] = filename[i];
    buffer[buffer_pos + 1] = '\0';
  }

  buffer_pos++;

  for (int i = 0; i < strlen(mode); i++, buffer_pos++) {
    buffer[buffer_pos] = mode[i];
    buffer[buffer_pos + 1] = '\0';
  }

  buffer_pos++;

  if (sendto(fd, buffer, buffer_pos, 0, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr)) == -1) {
    printf("Error sending RRQ packet\n");
    return -1;
  }

  short opcode;
  short block_number = 1;
  int data_size = 512;
  socklen_t server_length = sizeof(struct sockaddr);

  printf("fetching file...\n");
  while (data_size > 511) {
    printf("***\n");
    ssize_t pkt_size = recvfrom(
        fd, buffer, 516, 0, (struct sockaddr *)&server_addr, &server_length);
    if (pkt_size == -1) {
      printf("Problem receiving block #%d\n", block_number);
      continue;
    }

    if (pkt_size < 5) {
      printf("Packet size for block #%d is too small\n", block_number);
    }

    /*
     * DATA TFTP PACKET
     *
     * 2 bytes    | 2 bytes | n bytes |
     * --------------------------------
     *  OPCODE 03 | Block # | Data    |
     *  -------------------------------
     */

    // confirm if this is a data packet.
    opcode = ((buffer[0] & 0xFF) << 8) | buffer[1];
    if (opcode != 3) {
      // TODO: add checks for which opcode has been
      // received.
      continue;
    }

    short received_block = ((buffer[2] & 0xFF) << 8) | buffer[3];
    if (received_block != block_number) {
      // TODO: add error returns.
      continue;
    }

    for (int i = 4; i < pkt_size; i++) {
      fputc(buffer[i], dest_file);
    }

    /*
     * ACK TFTP PACKET
     * 2 bytes 2 bytes
     * -------------------
     *| 04 | Block # |
     *  --------------------
     */
    memset(&buffer, 0, sizeof(buffer));
    buffer[0] = 0x00;
    buffer[1] = 0x04;

    buffer[2] = (block_number >> 8) & 0xFF;
    buffer[3] = block_number & 0xFF;
    sendto(fd, buffer, 4, 0, (struct sockaddr *)&server_addr, server_length);

    data_size = pkt_size - 4;
    block_number++;
  }
  printf("successfully fetched %s\n", filename);

  return 0;
}
