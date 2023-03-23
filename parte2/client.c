#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>



#define MAX_BUFFER_SIZE 1024
#define PORT 9999



typedef struct {
    char ip[15];
    int port;
    int sock;
    struct sockaddr_in server_addr;
} Sock;



typedef struct {
    char ip[INET_ADDRSTRLEN];
    int communication_port;
    char nickname[20];
} client_adress;



Sock primaryServer;



typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Connection;



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

    Sock primary_server_socket;



    int *p_value = (int *) port;

    int port_value = *p_value;

    

    // Create socket

    strcpy(communication_socket.ip, get_local_ip());

    communication_socket.port = port_value;



    communication_socket.sock = socket(AF_INET, SOCK_STREAM, 0);

    if (communication_socket.sock == -1) {

        printf("Could not create warnning communication socket\n");

        return NULL;

    }

    

    // Prepare the sockaddr_in structure

    communication_socket.server_addr.sin_family = AF_INET;

    communication_socket.server_addr.sin_addr.s_addr = INADDR_ANY;

    communication_socket.server_addr.sin_port = htons(communication_socket.port);

    

    // Bind the socket to address and port

    if (bind(communication_socket.sock, (struct sockaddr *)&communication_socket.server_addr, sizeof(communication_socket.server_addr)) < 0) {

        perror("Bind failed for warnning communication. Error");

        return NULL;

    }

    printf("Bind warnning communication done\n");

    

    // Listen for incoming connections

    listen(communication_socket.sock, 3);

    

    // Accept incoming connections and handle each one in a new thread

    printf("Waiting for incoming connections for warnnings communication on %d port...\n", port_value);

    socklen_t client_addr_len = sizeof(primary_server_socket.server_addr);

    while ((primary_server_socket.sock = accept(communication_socket.sock, (struct sockaddr *)&primary_server_socket.server_addr, &client_addr_len))) {        

        // receive the Connection struct from the client

        Connection conn;

        int ret = recv(primary_server_socket.sock, &conn, sizeof(conn), 0);

        if (ret != sizeof(conn)) {

            printf("Failed to receive new primary server from backup\n");

            close(primary_server_socket.sock);

            continue;

        } else {

            printf("WARNNING: New primary server %s:%d", conn.ip, conn.port);

            //reconnect(conn);

            close(primary_server_socket.sock);

        }

    }

    

    if (primary_server_socket.sock < 0) {

        perror("Accept failed for warnning communication");

        return NULL;

    }

    

    return NULL;

}



int main(int argc, char *argv[]) {



    if(argc != 5) {

        printf("Usage: %s <nickname> <ip_primary_server> <port_primary_server> <port_warnning_communication>\n", argv[0]);

        return 1;

    }



    printf("%s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);



    // Call frontend thread with port_warnning_communication

    pthread_t tid;

    int port;



    port = atoi(argv[4]);

    if (pthread_create(&tid, NULL, frontend, &port)) {

        perror("Could not create thread");

        return 1;

    }



    strcpy(primaryServer.ip, argv[2]);

    primaryServer.port = atoi(argv[3]);



    char nickname[20], server_message[2000];



    // Localhost

    char *local_ip;

    local_ip = get_local_ip();

    printf("> Local Host: %s\n", local_ip);

    

    // // Get client nickname

    // printf("Enter your nickname: ");

    // fgets(nickname, 20, stdin);

    // strtok(nickname, "\n"); // Remove trailing newline

    

    // Create socket

    primaryServer.sock = socket(AF_INET, SOCK_STREAM, 0);

    if (primaryServer.sock == -1) {

        printf("Could not create socket\n");

        return 1;

    }

    

    // Prepare the sockaddr_in structure

    primaryServer.server_addr.sin_family = AF_INET;

    primaryServer.server_addr.sin_addr.s_addr = inet_addr(primaryServer.ip); // 127.0.0.1

    primaryServer.server_addr.sin_port = htons(primaryServer.port); // 8888

    

    // Connect to server

    if (connect(primaryServer.sock, (struct sockaddr *)&primaryServer.server_addr, sizeof(primaryServer.server_addr)) < 0) {

        perror("Connect failed. Error");

        return 1;

    }

    

    printf("Connected to server\n");

    

    // Send client nickname to server

    client_adress client;

    strcpy(client.ip, local_ip);

    client.communication_port = port;

    strcpy(client.nickname, argv[1]);



    if (send(primaryServer.sock, &client, sizeof(client), 0) < 0) {

        printf("Send failed\n");

        return 1;

    }

    

    // Send messages to server

    char client_message[2000];

    while (1) {

        fgets(client_message, 2000, stdin);

        if (strcmp(client_message, "exit\n") == 0) {

            break;

        }

        if (send(primaryServer.sock, client_message, strlen(client_message), 0) < 0) {

            printf("Send failed\n");

            break;

        }

    }

    

    // Close socket

    close(primaryServer.sock);

    

    return 0;

}