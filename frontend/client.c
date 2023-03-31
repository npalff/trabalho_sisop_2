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

typedef struct {
    int action;
    char payload[8000];
} Message;

Sock primaryServer;

Address addressPrimaryServer;
Address clientAddress;

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

void *frontend(void *port) {
    Sock communication_socket;
    Sock elected_server_socket;

    int *p_value = (int *) port;
    int port_value = *p_value;
    
    // Create socket
    strcpy(communication_socket.ip, get_local_ip());
    communication_socket.port = port_value;

    communication_socket.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (communication_socket.sock == -1) {
        printf("> [ERROR]: Could not create warnning communication socket\n");
        return NULL;
    }
    
    // Prepare the sockaddr_in structure
    communication_socket.addr_in.sin_family = AF_INET;
    communication_socket.addr_in.sin_addr.s_addr = INADDR_ANY;
    communication_socket.addr_in.sin_port = htons(communication_socket.port);
    
    // Bind the socket to address and port
    if (bind(communication_socket.sock, (struct sockaddr *)&communication_socket.addr_in, sizeof(communication_socket.addr_in)) < 0) {
        perror("> [ERROR]: Bind failed for warnning communication. Error");
        return NULL;
    }
    printf("> [SUCCESS]: Bind warnning communication done\n");
    
    // Listen for incoming connections
    listen(communication_socket.sock, 3);
    
    // Accept incoming connections and handle each one in a new thread
    printf("> [SUCCESS]: Waiting for incoming connections for warnnings communication on %d port...\n", port_value);
    socklen_t client_addr_len = sizeof(elected_server_socket.addr_in);
    while ((elected_server_socket.sock = accept(communication_socket.sock, (struct sockaddr *)&elected_server_socket.addr_in, &client_addr_len))) {        
        // receive the Address struct from the new primary server
        int ret = recv(elected_server_socket.sock, &addressPrimaryServer, sizeof(addressPrimaryServer), 0);
        if (ret != sizeof(addressPrimaryServer)) {
            printf("> [ERROR]: Failed to receive new primary server from backup\n");
            close(elected_server_socket.sock);
            continue;
        } else {
            // Close old primary server socket
            close(primaryServer.sock);

            // Close elected server socket
            close(elected_server_socket.sock);

            printf("> [WARNNING]: Close connection of current primary server - [%s] %s:%d\n", primaryServer.nickname, primaryServer.ip, primaryServer.port);
            printf("> [WARNNING]: New primary server address received - [%s] %s:%d\n", addressPrimaryServer.nickname, addressPrimaryServer.ip, addressPrimaryServer.port);

            connectPrimaryServer(clientAddress, addressPrimaryServer);
        }
    }
    
    if (elected_server_socket.sock < 0) {
        perror("> [ERROR]: Accept failed for warnning communication");
        return NULL;
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {

    char process[20], nickname[20], nicknameServer[20], ipPrimaryServer[INET_ADDRSTRLEN];
    int portPrimaryServer, portWarnningCommunication;

    strcpy(process, argv[0]);
    if(argc != 6) {
        printf("> [ERROR]: Use %s <nickname_user> <port_warnning_communication> <nickname_primary_server> <ip_primary_server> <port_primary_server>\n", process);
        return 1;
    }

    strcpy(nickname, argv[1]);
    portWarnningCommunication = atoi(argv[2]);
    strcpy(nicknameServer, argv[3]);
    strcpy(ipPrimaryServer, argv[4]);
    portPrimaryServer = atoi(argv[5]);

    printf("> [LOG]: %s %s %s %d %d\n", process, nickname, ipPrimaryServer, portPrimaryServer, portWarnningCommunication);
    
    // Show local host
    char localHost[INET_ADDRSTRLEN];
    strcpy(localHost, get_local_ip());
    printf("> [LOG]: Local host %s\n", localHost);

    // Call frontend thread with port warnning communication
    pthread_t tid;
    if (pthread_create(&tid, NULL, frontend, &portWarnningCommunication)) {
        perror("> [ERROR]: Could not create warnning communication thread");
        return 1;
    }
    
    strcpy(addressPrimaryServer.nickname, nicknameServer);
    strcpy(addressPrimaryServer.ip, ipPrimaryServer);
    addressPrimaryServer.port = portPrimaryServer;

    strcpy(clientAddress.ip, localHost);
    clientAddress.port = portWarnningCommunication;
    strcpy(clientAddress.nickname, nickname);

    connectPrimaryServer(clientAddress, addressPrimaryServer);
    
    // Send messages to server
    char client_message[2000];
    while (1) {
        fgets(client_message, 2000, stdin);
        if (strcmp(client_message, "exit\n") == 0) {
            break;
        }

        int command = MESSAGE;
        if (send(primaryServer.sock, &command, sizeof(int), 0) < 0) {
            printf("> [ERROR]: Send messages command to serve failed\n");
            break;
        }

        if (send(primaryServer.sock, client_message, strlen(client_message), 0) < 0) {
            printf("> [ERROR]: Send messages to serve failed\n");
            break;
        }
    }
    
    // Close socket
    close(primaryServer.sock);
    
    return 0;
}
