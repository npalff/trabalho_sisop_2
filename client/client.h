#ifndef CLIENT
#define CLIENT

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
#include <stdint.h>
#include <stdbool.h>

#define DATA 1
#define CMD 2
#define BUFFER_SIZE 256

#include "client.h";
#include "client_communication.h";
#include "client_syncronization.h";
#include "../tools/tools.h";

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