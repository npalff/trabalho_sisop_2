#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MESSAGE 0
#define SIGNAL_ALIVE 1
#define NEW_BACKUP 2
#define ELECTION 3

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
Sock clientsSocks[MAX_CLIENTS];
Address serversAddresses[MAX_CLIENTS] = { "", "", 0 };
Sock serverSocks[MAX_CLIENTS];

int connectedClients = 0;
Sock primaryServer;
int serverIsAlive = 1;

void *handle_serve(void *arg);
void *handle_client(void *arg);
int notifyClient(Sock clientAddress, Address primaryServerElectedAddress);
void printClientsAddresses();
int addAddressClient(Address address);
int removeAddressClient(Address address);
void *aliveServer();
char* get_local_ip();

int clientOpenConnection(int freePosition, Address address);
int connectPrimaryServer(Address clientAddress, Address addressPrimaryServer);

int main(int argc, char *argv[]) {
    int server_socket, client_socket, read_size;
    struct sockaddr_in server_addr, client_addr;
    char client_message[2000];
    int num_clients = 0;

    char process[20], nicknameServer[20], ipPrimaryServer[INET_ADDRSTRLEN];
    int portPrimaryServer, portServer;

    Address addressBackupServer;
    Address addressPrimaryServer;

    strcpy(nicknameServer, argv[1]);
    if(strstr(nicknameServer, "primary") != NULL) {
        if(argc != 3) {
            printf("> [ERROR]: Use %s <nickname_server> <port_server>\n", process);
            return 1;
        }
        strcpy(nicknameServer, argv[1]);
        strcpy(process, argv[0]);
        portServer = atoi(argv[2]);
        //cria thread alive
    };

    if(strstr(nicknameServer, "backup") != NULL) {
        if(argc != 5) {
            printf("> [ERROR]: Use %s <nickname_server> <port_server> <ip_primary_server> <port_primary_server>\n", process);
            return 1;
        }

        strcpy(process, argv[0]);
        strcpy(addressBackupServer.nickname, argv[1]);
        strcpy(addressBackupServer.ip, get_local_ip());
        addressBackupServer.port = atoi(argv[2]);

        strcpy(addressPrimaryServer.nickname, "primary");
        strcpy(addressPrimaryServer.ip, argv[3]);
        addressPrimaryServer.port = atoi(argv[4]);

        sleep(3);
        connectPrimaryServer(addressPrimaryServer, addressBackupServer);
        
        //Call frontend thread with port warnning communication
        pthread_t tid;
        if (pthread_create(&tid, NULL, alive)) {
            perror("> [ERROR]: Could not create thread");
            return 1;
        }
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Could not create socket\n");
        return 1;
    }
    
    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portServer);
    
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
    int freePosition = addAddressClient(clientAddress);
    clientOpenConnection(freePosition, clientAddress);
    printClientsAddresses();
    
    // Receive incoming messages and broadcast them to all clients
    char client_message[2000];
    int read_size;
    int command;
    while ((read_size = recv(client_socket, &command, sizeof(int), 0)) > 0) {
        printf("> [LOG]: Command %d from %s\n", command, clientAddress.nickname);
        if(command == MESSAGE) {
            if((read_size = recv(client_socket, client_message, 2000, 0)) > 0) {
                // Add terminating null byte
                client_message[read_size] = '\0';
                // Print message to command line
                printf("> [%s]: %s", clientAddress.nickname, client_message);

                //replicate();
            }
        }

        if(command == SIGNAL_ALIVE) {
            int commandAlive = SIGNAL_ALIVE;
            if (send(clientsSocks[freePosition].sock, &commandAlive, sizeof(commandAlive), 0) < 0) {
                printf("> [ERROR]: Send messages command to serve failed\n");
                
            }
            printf("> [LOG]: Signal alive\n");
        }

        if(command == NEW_BACKUP) {
            printf("> [LOG]: New backup\n");
        }
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

int notifyClient(Sock clientSock, Address primaryServerElectedAddress) {
    
    // Send election command
    int command = ELECTION;
    if (send(clientSock.sock, &command, sizeof(command), 0) < 0) {
        printf("> [ERROR]: Send ELECTION command to client failed\n");
        return 1;
    }

    // Send new address primary server to client
    if (send(clientSock.sock, &primaryServerElectedAddress, sizeof(primaryServerElectedAddress), 0) < 0) {
        printf("> [ERROR]: Send address to client failed\n");
        return 1;
    }

    // Close socket
    // close(clientSock.sock);
    
    return 0;
}

void printClientsAddresses() {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (!strlen(clientsAddresses[i].nickname) == 0)
            printf("%d: %s %s %d\n", i+1, clientsAddresses[i].nickname, clientsAddresses[i].ip, clientsAddresses[i].port);
}

void clientCloseConnection(int deletePosition) {
    close(clientsSocks[deletePosition].sock);
}

int clientOpenConnection(int freePosition, Address address) {

    printf("Client connected: %d [%s] %s:%d\n", freePosition, address.nickname, address.ip, address.port);
    // Create socket
    strcpy(clientsSocks[freePosition].nickname, address.nickname);
    strcpy(clientsSocks[freePosition].ip, address.ip);
    clientsSocks[freePosition].port = address.port;
    clientsSocks[freePosition].sock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientsSocks[freePosition].sock == -1) {
        printf("> [ERROR]: Could not create socket to notify client %s for new primary server\n", address.nickname);
        return 1;
    }
    
    // Prepare the sockaddr_in structure
    clientsSocks[freePosition].addr_in.sin_family = AF_INET;
    clientsSocks[freePosition].addr_in.sin_addr.s_addr = inet_addr(address.ip); // 127.0.0.1
    clientsSocks[freePosition].addr_in.sin_port = htons(address.port); // 8888
    
    printf("%s %s %d %d\n", clientsSocks[freePosition].nickname, clientsSocks[freePosition].ip, clientsSocks[freePosition].port, clientsSocks[freePosition].sock);
    // Connect to client
    if (connect(clientsSocks[freePosition].sock, (struct sockaddr *)&clientsSocks[freePosition].addr_in, sizeof(clientsSocks[freePosition].addr_in)) < 0) {
        perror("> [ERROR]: Connect failed to notify client for new primary server. Error");
        return 1;
    }
    
    printf("> [SUCCESS]: Connected to client %s to notify new primary server\n", clientsSocks[freePosition].nickname);
}

int addAddressClient(Address address) {
    int freePosition = 0;

    while(strlen(clientsAddresses[freePosition].nickname) > 0)
        freePosition++;

    clientsAddresses[freePosition] = address;

    return freePosition;
}

int removeAddressClient(Address address) {
    int deletePosition = 0;

    while(strlen(clientsAddresses[deletePosition].nickname) > 0)
        deletePosition++;

    Address defaultAddress = { "", "", 0 };
    clientsAddresses[deletePosition] = defaultAddress;

    return deletePosition;
}

int connectPrimaryServer(Address clientAddress, Address addressPrimaryServer) {
    // Create socket
    strcpy(primaryServer.nickname, addressPrimaryServer.nickname);
    strcpy(primaryServer.ip, addressPrimaryServer.ip);
    primaryServer.port = addressPrimaryServer.port;
    primaryServer.sock = socket(AF_INET, SOCK_STREAM, 0);

    if (primaryServer.sock == -1) {
        printf("> [ERROR]: Could not create socket\n");
        return 1;
    }
    
    // Prepare the sockaddr_in structure
    primaryServer.addr_in.sin_family = AF_INET;
    primaryServer.addr_in.sin_addr.s_addr = inet_addr(primaryServer.ip); // 127.0.0.1
    primaryServer.addr_in.sin_port = htons(primaryServer.port); // 8888
    
    // Connect to server
    if (connect(primaryServer.sock, (struct sockaddr *)&primaryServer.addr_in, sizeof(primaryServer.addr_in)) < 0) {
        perror("> [ERROR]: Connect failed to primary server. Error");
        return 1;
    }
    printf("> [SUCCESS]: Connected to primary server - [%s] %s:%d\n", primaryServer.nickname, primaryServer.ip, primaryServer.port);

    // Send client address to server
    if (send(primaryServer.sock, &clientAddress, sizeof(clientAddress), 0) < 0) {
        printf("> [ERROR]: Send address to serve failed\n");
        return 1;
    }

    // Call server alive thread for check connection is working
    pthread_t tidAliveServer;
    if (pthread_create(&tidAliveServer, NULL, aliveServer, NULL)) {
        perror("> [ERROR]: Could not alive server thread");
        return 1;
    }
    
}

void *aliveServer() {
    while(1) { 
        // Send to server alive command to check server is working
        int command = SIGNAL_ALIVE;
        if (send(primaryServer.sock, &command, sizeof(int), 0) < 0) {
            printf("> [ERROR]: Send messages command to serve failed\n");
            break;
        } else {
            printf("> [LOG]: Send signal alive to server\n");
        }

        sleep(5);

        printf("> [LOG]: Check signal alive from server\n");
        if(serverIsAlive == 0) {
            printf("> [LOG]: We need to elect a new primary server\n");
            printf("> [LOG]: Election started");
            while(!serverIsAlive) printf("> [LOG]: Electing");; 
            // election();
            //printf("> [LOG]: Election done");
        } else {
            printf("> [LOG]: Server is alive\n");
        }
        serverIsAlive = 0;
    }
}

char* get_local_ip() {
    struct ifaddrs *addrs, *tmp;
    char* ip = NULL;

    // Get a list of network interface addresses
    if (getifaddrs(&addrs) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    // Iterate over the list of addresses and look for an IPv4 address
    for (tmp = addrs; tmp != NULL; tmp = tmp->ifa_next) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)tmp->ifa_addr;
            if (strcmp(tmp->ifa_name, "lo") == 0 && (addr->sin_addr.s_addr & htonl(0xff000000)) == htonl(0x7f000000)) {
                ip = strdup(inet_ntoa(addr->sin_addr));
                break;
            }
        }
    }

    freeifaddrs(addrs);
    return ip;
}