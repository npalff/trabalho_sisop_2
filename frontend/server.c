#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MESSAGE 0
#define MAX_CLIENTS 10
#define PORT 8882

typedef struct {
    char nickname[20];
    char ip[INET_ADDRSTRLEN];
    int port;
    int sock;
    struct sockaddr_in addr_in;
} Sock;

typedef struct {
    char nickname[20];
    char ip[INET_ADDRSTRLEN];
    int port;
} Address;

Address clientsAddresses[MAX_CLIENTS] = { "", "", 0 };
Address serversAddresses[MAX_CLIENTS] = { "", "", 0 };

int connectedClients = 0;

void *handle_serve(void *arg);
void *handle_client(void *arg);
int notifyClient(Address clientAddress, Address primaryServerElectedAddress);
void printClientsAddresses();
void addAddressClient(Address address);
void removeAddressClient(Address address);

int main(int argc, char *argv[]) {
    int server_socket, client_socket, read_size;
    struct sockaddr_in server_addr, client_addr;
    char client_message[2000];
    int num_clients = 0;

    char process[20], nicknameServer[20], ipPrimaryServer[INET_ADDRSTRLEN];
    int portPrimaryServer, portServer;

    strcpy(process, argv[0]);
    strcpy(nicknameServer, argv[1]);
    portServer = atoi(argv[2]);
    strcpy(ipPrimaryServer, argv[3]);
    portPrimaryServer = atoi(argv[4]);



    Address addressBackupServer;
    Address addressPrimaryServer;

    if(strstr(nicknameServer, "primary") != NULL) {
        if(argc != 3) {
            printf("> [ERROR]: Use %s <nickname_server> <port_server>\n", process);
            return 1;
        }
        //cria thread alive
    };

    if(strstr(nicknameServer, "backup") != NULL) {
        if(argc != 5) {
            printf("> [ERROR]: Use %s <nickname_server> <port_server> <ip_primary_server> <port_primary_server>\n", process);
            return 1;
        }

        joinGroup(addressPrimaryServer, addressBackupServer);
    }


    // Call frontend thread with port warnning communication
    // pthread_t tid;
    // if (pthread_create(&tid, NULL, backupCommunication, &portBackupCommunication)) {
    //     perror("> [ERROR]: Could not create thread");
    //     return 1;
    // }

    // Call frontend thread with port warnning communication
    // pthread_t tid;
    // if (pthread_create(&tid, NULL, alive)) {
    //     perror("> [ERROR]: Could not create thread");
    //     return 1;
    // }

    
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

    // printf("pre-send\n");
    // sleep(30);
    // printf("sent\n");
    // Address c;
    // strcpy(c.nickname, "richard");
    // strcpy(c.ip, "127.0.0.1");
    // c.port = 8888;

    // Address s;
    // strcpy(s.nickname, "backup");
    // strcpy(s.ip, "127.0.0.1");
    // s.port = 8882;
    // notifyClient(c, s);

    
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

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    
    // Receive address from client
    Address clientAddress;
    if (recv(client_socket, &clientAddress, sizeof(clientAddress), 0) < 0) {
        printf("Receive failed\n");
        return NULL;
    }
    printf("Client connected: [%s] %s:%d\n", clientAddress.nickname, clientAddress.ip, clientAddress.port);

    // Add address client in the list of addresses
    addAddressClient(clientAddress);
    printClientsAddresses();
    
    // Receive incoming messages and broadcast them to all clients
    char client_message[2000];
    int read_size;
    int command;
    while (recv(client_socket, client_message, sizeof(int), 0) > 0) {
        if(command == MESSAGE)
            if((read_size = recv(client_socket, client_message, 2000, 0)) > 0) {
                // Add terminating null byte
                client_message[read_size] = '\0';
                // Print message to command line
                printf("[%s]: %s", clientAddress.nickname, client_message);
            }

        if(command == SIGNAL_ALIVE)
        if(command == NEW_BACKUP)
    }
    
    if (read_size == 0) {
        printf("Client disconnected: %s\n", clientAddress.nickname);
    } else if (read_size == -1) {
        perror("Receive failed");
    }

    // Add address client in the list of addresses
    removeAddressClient(clientAddress);
    
    return NULL;
}

int notifyClient(Address clientAddress, Address primaryServerElectedAddress) {
    Sock clientSock;

    // Create socket
    strcpy(clientSock.ip, clientAddress.ip);
    clientSock.port = clientAddress.port;
    clientSock.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock.sock == -1) {
        printf("> [ERROR]: Could not create socket to notify client %s for new primary server\n", clientAddress.nickname);
        return 1;
    }
    
    // Prepare the sockaddr_in structure
    clientSock.addr_in.sin_family = AF_INET;
    clientSock.addr_in.sin_addr.s_addr = inet_addr(clientSock.ip); // 127.0.0.1
    clientSock.addr_in.sin_port = htons(clientSock.port); // 8888
    
    // Connect to client
    if (connect(clientSock.sock, (struct sockaddr *)&clientSock.addr_in, sizeof(clientSock.addr_in)) < 0) {
        perror("> [ERROR]: Connect failed to notify client for new primary server. Error");
        return 1;
    }
    
    printf("> [SUCCESS]: Connected to client %s to notify new primary server\n", clientAddress.nickname);
    
    // Send new address primary server to client
    if (send(clientSock.sock, &primaryServerElectedAddress, sizeof(primaryServerElectedAddress), 0) < 0) {
        printf("> [ERROR]: Send address to serve failed\n");
        return 1;
    }

    // Close socket
    close(clientSock.sock);
    
    return 0;
}

void printClientsAddresses() {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (!strlen(clientsAddresses[i].nickname) == 0)
            printf("%d: %s %s %d\n", i+1, clientsAddresses[i].nickname, clientsAddresses[i].ip, clientsAddresses[i].port);
}

void addAddressClient(Address address) {
    int freePosition = 0;

    while(strlen(clientsAddresses[freePosition].nickname) > 0)
        freePosition++;

    clientsAddresses[freePosition] = address;
}

void removeAddressClient(Address address) {
    int deletePosition = 0;

    while(strlen(clientsAddresses[deletePosition].nickname) > 0)
        deletePosition++;

    Address defaultAddress = { "", "", 0 };
    clientsAddresses[deletePosition] = defaultAddress;
}