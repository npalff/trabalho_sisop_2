#ifndef SERVIDOR
#define SERVIDOR

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

struct file_data
{
    char name[MAXNAME];
    char extension[MAXNAME];
    char last_modified[MAXNAME];
    time_t timestamp_last_modified;
    int size;
    pthread_mutex_t file_mutex;
};

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


#endif