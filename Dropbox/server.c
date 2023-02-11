
#include "server.h"

int PORT;

struct client_list* clients;
int writing = 0;

int main(int argc, char* argv[])
{
    if(argc != 1)
    {
        printf("")
    }
    else
    {}


}

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