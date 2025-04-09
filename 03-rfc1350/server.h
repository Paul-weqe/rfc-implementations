#ifndef COMMON_1350_H
#define COMMON_1350_H

#define DEFAULT_SERVER_TID 2000
#define BUFFER_SIZE 512

struct HandleRequestArgs {
  char *msg;
  struct sockaddr_in client_address;
  ssize_t pkt_size;
};

int create_socket(int port);
void *handle_request_thread(void *args);
void handle_request(char msg[], struct sockaddr_in client_address,
                    ssize_t pkt_size);
void modify_data_buffer(char buffer[], char file_buffer[], int read_size,
                        short block_number);
void modify_error_buffer(char buffer[], char *msg, int res_len);

#endif
