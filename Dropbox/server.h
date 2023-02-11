#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXNAME 64
#define MAXFILES 30
#define FREEDEV -1

#ifndef SERVIDOR
#define SERVIDOR

struct file_data
{
    char name[MAXNAME];
    char extension[MAXNAME];
    char last_modified[MAXNAME];
    time_t timestamp_last_modified;
    int size;
    pthread_mutex_t file_mutex;
}file_info;

struct client
{
    int devices[2];
    char user_id[MAXNAME];
    struct file_data file_data[MAXFILES];
    int logged;
};

struct client_list
{
    struct client client;
    struct client_list* next;
};

struct client_request
{
    char file[200];
    int command;
};



int initialize_client(int client_socket, char *userid, struct client *client);
void initialize_client_list();
void close_client_connection();

void sync_server(int socket, char *userid);
void listen_sync(int client_socket, char *userid);


void *client_thread (void *socket);
void *sync_thread_sv(void *socket);
void listen_client(int client_socket, char *userid);

void send_file(char *file, int socket, char *userid);
void receive_file(char *file, int socket, char*userid);

void send_all_files(int client_socket, char *userid);
void send_file_info(int socket, char *userid);
void update_file_info(char *userid, struct file_data file_data);

void delete_file(char *file, int socket, char *userid);






#endif