#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "./server.h"

void level_1_prompt(char *msg) { printf("\x1b[32m\e[1m%s\x1b[0m", msg); }
void level_2_prompt(char *msg) { printf("\x1b[33m\e[3m  %s\x1b[0m", msg); }

int main(int argc, char *argv[]) {
  int server_fd, port;
  char input[5];

  level_1_prompt("Port number[default 2000]: ");

  // ---------- fetch IP Address ------
  fgets(input, sizeof input, stdin);
  if (input[0] == '\n') {
    strcpy(input, "2000");
  }
  port = atoi(input);
  server_fd = create_socket(port);
  if (server_fd < 0) {
    perror("Failure creating socket");
    exit(1);
  }
  printf("socket created on port %d\n", port);

  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(struct sockaddr);
    char msg[BUFFER_SIZE];
    ssize_t pkt_size =
        recvfrom(server_fd, msg, BUFFER_SIZE, 0,
                 (struct sockaddr *)&client_address, &client_length);

    struct HandleRequestArgs args = {
        .msg = msg, .client_address = client_address, .pkt_size = pkt_size};
    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_request_thread, &args) != 0) {
      perror("Unable to create thread");
    }
    pthread_join(thread, NULL);
  }
  return 0;
}

int create_socket(int port) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  // Server address information.
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind Address information to the socket.
  socklen_t addrlen = sizeof(server_address);
  if (bind(fd, (const struct sockaddr *)&server_address, addrlen) < 0) {
    perror("Unable to bind socket");
    return -1;
  }

  return fd;
}

void *handle_request_thread(void *args) {
  struct HandleRequestArgs *thread_args = (struct HandleRequestArgs *)args;

  handle_request(thread_args->msg, thread_args->client_address,
                 thread_args->pkt_size);

  // free memory once done
  /*free(thread_args);*/
  return NULL;
}

void handle_request(char msg[], struct sockaddr_in client_address,
                    ssize_t pkt_size) {
  short block_number = 1;
  FILE *fp;
  char file_buffer[BUFFER_SIZE];
  int read_size;
  char mode[10];
  socklen_t client_length = sizeof(struct sockaddr);
  char file_name[512] = "";
  char buffer[516];
  short opcode;

  // Generate a random port.
  int port = rand() % (65535 - 1024 + 1) + 1024;
  int fd = create_socket(port);

  // Get current operation.
  opcode = ((msg[0] & 0xFF) << 8) | msg[1];
  if (opcode == 1 || opcode == 2) {

    /*
     * -------------------------------------------------------------
     * 2 bytes            | string    | 1 byte | string  | 1 bytes |
     * -------------------------------------------------------------
     *  [RRQ/WRQ] [01/02] | Filename  | 0      | Mode    | 0       |
     *  ------------------------------------------------------------
     *  */
    // Keeps track of how far we have read the msg.
    int msg_pos = 2;

    // Fetch the filename.
    while (msg[msg_pos] != 0x00) {
      file_name[msg_pos - 2] = msg[msg_pos];
      msg_pos++;
    }

    // Skip over the first 0 bit.
    msg_pos++;

    // Mode.
    for (int i = 0; msg_pos < pkt_size - 1; msg_pos++, i++) {
      mode[i] = msg[msg_pos];
      mode[i + 1] = '\0';
    }

    printf("Request for file %s received. Mode %s\n", file_name, mode);

    if (strcmp(mode, "octet") == 0) {
      fp = fopen(file_name, "rb");
    } else if (strcmp(mode, "netascii") == 0) {
      fp = fopen(file_name, "r");
    } else {
      char msg[] = "mode unsupported";
      perror(msg);
      // size of the error message plus OPCODE (2 bytes),
      // ErrorCode(2 bytes) and 1 terminator byte.
      int res_len = sizeof(msg) + 5;
      modify_error_buffer(buffer, msg, res_len);
      sendto(fd, buffer, res_len, 0, (struct sockaddr *)&client_address,
             client_length);
      return;
    }

    // Problem opening the file.
    if (!fp) {
      char msg[] = "Problem opening file";
      perror(msg);

      // size of the error message plus OPCODE (2 bytes),
      // ErrorCode(2 bytes) and 1 terminator byte.
      int res_len = sizeof(msg) + 5;
      modify_error_buffer(buffer, msg, res_len);
      sendto(fd, buffer, res_len, 0, (struct sockaddr *)&client_address,
             client_length);
      return;
    }

    // Size that has been read from the file.
    read_size = fread(file_buffer, 1, BUFFER_SIZE, fp);

    // Modify the response packet to be a DATA response.
    modify_data_buffer(buffer, file_buffer, read_size, block_number);
    sendto(fd, buffer, read_size + 4, 0, (struct sockaddr *)&client_address,
           client_length);

    block_number++;
    memset(&buffer, 0, sizeof(buffer));
    if (read_size < 512) {
      return;
    }
  }

  // If it's an ACK that has been received.
  while (1) {
    pkt_size = recvfrom(fd, msg, BUFFER_SIZE, 0,
                        (struct sockaddr *)&client_address, &client_length);
    if (pkt_size == 0) {
      perror("Packet too small...");
      return;
    }

    /*
     * Received ACK packet.
     *
     * 2 bytes   | 2 bytes |
     * ---------------------
     * OPCODE 04 | Block # |
     * ---------------------
     */
    opcode = ((msg[0] & 0xFF) << 8) | msg[1];

    // if it's not an ACK that has been received
    if (opcode != 4) {
      break;
    }

    // Size that has been read from the file.
    read_size = fread(file_buffer, 1, BUFFER_SIZE, fp);

    // Modify the response packet to be a DATA response.
    modify_data_buffer(buffer, file_buffer, read_size, block_number);
    sendto(fd, buffer, read_size + 4, 0, (struct sockaddr *)&client_address,
           client_length);

    block_number++;
    memset(&buffer, 0, sizeof(buffer));
    if (read_size < 512) {
      return;
    }
  }
}

void modify_data_buffer(char buffer[], char file_buffer[], int read_size,
                        short block_number) {
  /*
   * DATA TFTP PACKET
   *
   * 2 bytes    | 2 bytes | n bytes |
   * --------------------------------
   *  OPCODE 03 | Block # | Data    |
   *  -------------------------------
   */

  // DATA opcode [03].
  buffer[0] = 0x00;
  buffer[1] = 0x03;

  // Block number.
  buffer[2] = (block_number >> 8) & 0xFF;
  buffer[3] = block_number & 0xFF;

  // Data.
  for (int i = 0; i < read_size; i++) {
    buffer[i + 4] = file_buffer[i];
  }
}

void modify_error_buffer(char buffer[], char *msg, int res_len) {
  /*
   * ERROR TFTP PACKET
   * 2 bytes     | 2 bytes   | string | 1 byte |
   * -------------------------------------------
   *  OPCODE  05 | ErrorCode | ErrMsg | 0      |
   *  ------------------------------------------
   *
   */

  // opcode ERROR [05].
  buffer[0] = 0x00;
  buffer[1] = 0x05;

  // ERROR CODE [00] -> not defined.
  buffer[2] = 0x00;
  buffer[3] = 0x00;

  // Error Message.
  for (int i = 0; i < sizeof(msg); i++) {
    buffer[i + 4] = msg[i];
  }

  // Final 1 byte in packet.
  buffer[res_len - 1] = 0;
}
