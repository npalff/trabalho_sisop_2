#include "client.h"
#include "communication.h"
#include "syncronization.h"
#include "interface.h"

void main(int argc, char *argv[]) {
	if (argc < 3) {
        printf("ERROR: Insufficient arguments\n");
        exit(0);
	}

	// User Name
	if (strlen(argv[1]) <= MAXNAME)
		strcpy(user_id, argv[1]);

	// Host
	host = malloc(sizeof(argv[2]));
	strcpy(host, argv[2]);

	// Port
	port = atoi(argv[3]);

	if (open_connection_with_server(host, port) > 0) {
		sync_client_inicialization();
		user_interface();
	}

	return;
}