#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define PORT 8881

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int communication_port;
    char nickname[20];
} client_adress;

typedef struct {
    int sock;
    char nickname[20];
    char *ip;
} client_info;

client_info clients[MAX_CLIENTS];
int num_clients = 0;

void *handle_client(void *arg);
char* get_address(int client_socket);

void send_connection();

int main(int argc, char *argv[]) {
    int server_socket, client_socket, read_size;
    struct sockaddr_in server_addr, client_addr;
    char client_message[2000];
    client_info clients[MAX_CLIENTS];
    int num_clients = 0;
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Could not create socket\n");
        return 1;
    }
    
    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind the socket to address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed. Error");
        return 1;
    }
    printf("Bind done\n");
    
    // Listen for incoming connections
    listen(server_socket, 3);

    
    // Accept incoming connections and handle each one in a new thread
    printf("Waiting for incoming connections...\n");
    socklen_t client_addr_len = sizeof(client_addr);
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len))) {        
        // Create a new thread to handle the client
        pthread_t tid;
        int *client_sock = malloc(sizeof(int));
        *client_sock = client_socket;
        if (pthread_create(&tid, NULL, handle_client, (void *)client_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
    }
    
    if (client_socket < 0) {
        perror("Accept failed");
        return 1;
    }
    
    return 0;
}

void send_connection() {

}

char* get_address(int client_socket) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(client_socket, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getpeername");
        exit(1);
    }

    char *client_ip = inet_ntoa(addr.sin_addr);
    return client_ip;
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    
    // Show ip Adress
    // char *client_ip = get_address(client_socket);
    // printf("Client IP address: %s\n", client_ip);
    
    // Get client nickname
    // char nickname[20];
    client_adress client;
    if (recv(client_socket, &client, sizeof(client), 0) < 0) {
        printf("Receive failed\n");
        return NULL;
    }
    printf("Client connected: [%s] %s:%d\n", client.nickname, client.ip, client.communication_port);
    
    // Add client to list
    // client_info client;
    // client.sock = client_socket;
    // strcpy(client.nickname, nickname);
    // client.ip = client_ip;
    
    // Receive incoming messages and broadcast them to all clients
    char client_message[2000];
    int read_size;
    while ((read_size = recv(client_socket, client_message, 2000, 0)) > 0) {
        // Add terminating null byte
        client_message[read_size] = '\0';
        
        // Print message to command line
        printf("[%s]: %s", client.nickname, client_message);
    }
    
    if (read_size == 0) {
        printf("Client disconnected: %s\n", client.nickname);
    } else if (read_size == -1) {
        perror("Receive failed");
    }
    
    // Remove client from list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == client_socket) {
            clients[i].sock = 0;
            break;
        }
    }
    
    return NULL;
}