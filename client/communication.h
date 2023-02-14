#ifndef COMMUNICATION_CLIENT
#define COMMUNICATION_CLIENT

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define DATA 1
#define CMD 2
#define BUFFER_SIZE 256

int open_connection_with_server(char *host, int port);
void close_connection_with_server();

void download_files();
void download_file(char *file);
void delete_file(char *file_name, int socket);
void list_files();

#endif
