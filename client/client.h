#ifndef CLIENT
#define CLIENT

#define DATA 1
#define CMD 2
#define MAXNAME 64
#define BUFFER_SIZE 256

#include "client.h"
#include "communication.h"
#include "syncronization.h"
#include "../tools/tools.h"

// Contants
#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define BUFFER_LENGTH ( 1024 * ( EVENT_SIZE + 16 ) )

#define TRUE 1
#define FALSE 0

// Global variables
char user_id[MAXNAME];
char sync_client_directory[MAXNAME + 50];
char *host;

int port;
int socket_fd = -1;
int sync_socket = -1;
int notifier;
int watch;

#endif