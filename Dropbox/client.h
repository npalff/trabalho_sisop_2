// Libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <pwd.h>
#include <sys/inotify.h>

// Contants
#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define BUFFER_LENGTH ( 1024 * ( EVENT_SIZE + 16 ) )

// Headers
#ifndef CLIENTE
#define CLIENTE

#define TRUE 1
#define FALSE 0

int create_sync_sock(); // x
void download_file(char *file); // void get_file(char *file)
int open_connection_with_server(char *host, int port); //int connect_server (char *host, int port)x
void sync_client_inicialization(); //void sync_client_first(); x
void close_connection_with_server(); // void close_connection(); x
void create_notifier_sync_client_directory(); // initializeNotifyDescription(); x
void *sync_client_directory_thread(); // void *sync_thread(); x
void download_files(); // void get_all_files(); x
void upload_file(char *client_file_path, char *file_name, int socket); // void upload_file(char *file, int socket);x
void delete_file(char *file_name, int socket); // void delete_file_request(char* file, int socket);x
void list_files(); //void show_files()x
void user_interface(); // void client_interface() x

#endif