#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAX_MSG_SIZE 1024
#define TIMEOUT_SEC 5

typedef enum {
    ELECTION,
    ANSWER,
    COORDINATOR
} message_type;

typedef struct {
    message_type type;
    int sender_id;
    int coordinator_id;
} message;

int id, num_processes, leader_id, socket_fd;
struct sockaddr_in *addresses;

// Enviar uma mensagem para um processo
void send_message(message msg, int dest_id) {
    if (dest_id == id) {
        // ignorar auto mensagens
        return;
    }

    struct sockaddr_in dest_address = addresses[dest_id];
    socklen_t addr_len = sizeof(dest_address);

    int bytes_sent = sendto(socket_fd, &msg, sizeof(message), 0, (struct sockaddr *) &dest_address, addr_len);
    if (bytes_sent == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

// Receba uma mensagem e retorne o id do remetente
int receive_message(message *msg) {
    struct sockaddr_in sender_address;
    socklen_t addr_len = sizeof(sender_address);

    int bytes_received = recvfrom(socket_fd, msg, sizeof(message), 0, (struct sockaddr *) &sender_address, &addr_len);
    if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // timeout, nenhuma mensagem recebida
            return -1;
        } else {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }
    }

    return ntohs(sender_address.sin_port) - 10000;
}

// Aguarde mensagens de todos os processos com id maior
int wait_for_answers() {
    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(socket_fd, &fds);

    int max_fd = socket_fd;
    for (int i = id + 1; i < num_processes; i++) {
        FD_SET(socket_fd, &fds);
        if (socket_fd > max_fd) {
            max_fd = socket_fd;
        }
    }

    int num_received = 0;
    while (num_received < num_processes - id - 1) {
        int ready = select(max_fd + 1, &fds, NULL, NULL, &timeout);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready == 0) {
            // timeout, nem todas as mensagens recebidas
            return -1;
        } else {
            message msg;
            int sender_id = receive_message(&msg);
            if (sender_id == -1) {
                // timeout, not all messages received
                return -1;
            } else {
                if (msg.type == ANSWER) {
                    num_received++;
                }
            }
        }
    }

    return 0;
}

//Transmite uma mensagem de eleição para todos os processos com id maior
void broadcast_election() {
    message msg = {ELECTION, id, -1};
    for (int i = id + 1; i < num_processes; i++) {
        send_message(msg, i);
    }
}

// Processar uma mensagem de eleição
void process_election(message msg) {
    // Recebeu uma mensagem election de outro processo
    printf("Process %d received an ELECTION message from process %d.\n", my_id, msg.from);

    // Verifica se o processo atual já realizou uma eleição
    if (!election_in_progress) {
        // Inicia uma nova eleição
        election_in_progress = true;
        printf("Process %d starts a new election.\n", my_id);
        send_election_msg();
    } else {
        // Eleição já em andamento
        printf("Process %d is already in an election.\n", my_id);
    }

    // Compara o id do remetente com o id do processo atual
    if (msg.from > my_id) {
        // Se o id do remetente for maior, responde com uma mensagem answer
        printf("Process %d sends an ANSWER message to process %d.\n", my_id, msg.from);
        send_answer_msg(msg.from);
    } else if (msg.from < my_id) {
        // Se o id do remetente for menor, não responde e inicia sua própria eleição
        printf("Process %d received an ELECTION message from process %d with a lower id. Starting a new election...\n", my_id, msg.from);
        election_in_progress = true;
        send_election_msg();
    } else {
        // Se o id do remetente for igual ao do processo atual, ele é o novo coordenador
        printf("Process %d has been elected as the new coordinator!\n", my_id);
        election_in_progress = false;
        coordinator_id = my_id;
        send_coordinator_msg();
    }
}

void process_answer(message msg) {
    // Recebeu uma mensagem answer de outro processo
    printf("Process %d received an ANSWER message from process %d.\n", my_id, msg.from);

    // Verifica se o processo atual está em uma eleição e se a mensagem é do coordenador atual
    if (election_in_progress && msg.from == coordinator_id) {
        // Atualiza o tempo limite para a próxima mensagem do coordenador
        coordinator_timeout_timer = time(NULL) + COORDINATOR_TIMEOUT;
    }

}