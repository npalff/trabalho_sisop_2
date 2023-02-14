#ifndef FILE_MANAGER_SERVER
#define FILE_MANAGER_SERVER

#include "server.h"
#include "../tools/tools.h"

void send_file(char *file, int socket, char *userid);
void receive_file(char *file, int socket, char*userid);

void send_all_files(int client_socket, char *userid);
void send_file_data(int socket, char *userid);
void update_file_data(char *userid, struct file_data file_data);

void delete_file_all_devices(char *file, int socket, char *userid);

#endif