#ifndef SYNCRONIZATION_SERVER
#define SYNCRONIZATION_SERVER



void sync_server(int socket, char *userid);
void listen_sync(int client_socket, char *userid);


void *client_thread (void *socket);
void *sync_thread_server(void *socket);
void listen_client(int client_socket, char *userid);


#endif