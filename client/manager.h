#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define DATA 1
#define CMD 2
#define BUFFER_SIZE 256

#ifndef COMMUNICATION_MANAGEMENT
#define COMMUNICATION_MANAGEMENT

typedef struct __packet {
    uint16_t type;          //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;          //Número de sequência
    uint16_t length;        //Comprimento do payload 
    time_t timestamp;       // Timestamp do dado
    char* _payload;         //Dados da mensagem
} packet;

typedef struct __client_thread_args {
    int sockfd;                 // Socket de comunicação
} client_thread_args;

// metodos comuns a servidor e cliente
int read_packet(int socketID, packet* pack, char *buffer); // 
int send_packet(int socketID, packet* pack);


// metodos do cliente
int connect_to_server (int socketID, char* port_number)

#endif