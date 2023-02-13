#ifndef COMMUNICATION_SERVER
#define COMMUNICATION_SERVER

int initialize_client(int client_socket, char *userid, struct client *client);
void initialize_clients();
void close_client_connection(int socket, char* userid);

#endif