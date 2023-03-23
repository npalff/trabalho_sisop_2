#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N_SERVERS 5
#define TIMEOUT 5

int servers[N_SERVERS];
int n_servers;
int leader;

void init_servers() {
    int i;
    for (i = 0; i < n_servers; i++) {
        servers[i] = i+1;
    }
}

void print_servers() {
    int i;
    printf("Servers: ");
    for (i = 0; i < n_servers; i++) {
        printf("%d ", servers[i]);
    }
    printf("\n");
}

int send_message(int src, int dst, int msg) {
    printf("Server %d sending message %d to server %d\n", src, msg, dst);
    if (servers[dst-1] > 0) {
        return 1;
    } else {
        return 0;
    }
}

int receive_message(int dst, int *msg) {
    int i;
    for (i = 0; i < n_servers; i++) {
        if (servers[i] == dst) {
            printf("Server %d receiving message %d\n", dst, *msg);
            return 1;
        }
    }
    return 0;
}

void start_election(int server_id) {
    int i;
    for (i = server_id+1; i <= n_servers; i++) {
        if (send_message(server_id, i, 0)) {
            int response;
            if (receive_message(i, &response)) {
                if (response == 1) {
                    printf("Server %d lost election to server %d\n", server_id, i);
                    return;
                }
            }
        }
    }
    leader = server_id;
    printf("Server %d is the new leader!\n", leader);
    for (i = 1; i <= n_servers; i++) {
        if (i != leader) {
            send_message(server_id, i, 1);
        }
    }
}

int main() {
    int i;
    srand(time(NULL));
    n_servers = N_SERVERS;
    init_servers();
    print_servers();
    leader = rand() % n_servers + 1;
    printf("Initial leader: %d\n", leader);
    for (i = 1; i <= n_servers; i++) {
        if (i != leader) {
            if (!send_message(leader, i, 1)) {
                printf("Server %d is not responding. Starting election...\n", i);
                start_election(i);
            }
        }
    }
    return 0;
}
