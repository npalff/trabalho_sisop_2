#include "communication.h"

// Global variables
char user_id[MAXNAME];
char sync_client_directory[MAXNAME + 50];
char *host;

int port;
int socket_fd = -1;
int sync_socket = -1;
int notifier;
int watch;

int open_connection_with_server(char *host, int port) {
    int number_bytes;
    int connected;
    struct sockaddr_in sockaddr_in;
    struct hostent *server;
    int client_thread = 1;
    char buffer[256];

    server = gethostbyname(host);

    if(server == NULL) {
        printf("ERROR: No host\n");
        return -1;
    }

    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR: Open socket\n");
        return -1;
    }

    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(port);
    sockaddr_in.sin_addr = *((struct in_addr *)server->h_addr);

    bzero(&(sockaddr_in.sin_zero), 8);

    if(connect(socket_fd,(struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in)) < 0) {
        printf("ERROR: Connect to socket\n");
        return -1;
    }

    write(socket_fd, &client_thread, sizeof(client_thread));

    number_bytes = write(socket_fd, user_id, sizeof(user_id));
    if(number_bytes < 0) {
        printf("ERROR: Send user id %s to server\n", user_id);
        return -1;
    }

    number_bytes = read(socket_fd, &connected, sizeof(int));
    if(number_bytes < 0) {
        printf("ERROR: Receive connected message\n");
        return -1;
    } else if (connected == 1) {
        printf("You are connected with server!\n");
        return 1;
    } else {
        printf("You are connected with two devices!\n");
        return -1;
    }
}

void close_connection_with_server(){
    int number_bytes;

    struct client_request client_request;
    client_request.command = EXIT;

    number_bytes = write(socket_fd, &client_request, sizeof(client_request));
    if (number_bytes < 0)
        printf("ERROR: Close connection\n");

    number_bytes = write(sync_socket, &client_request, sizeof(client_request));
    if (number_bytes < 0)
        printf("ERROR: Close connection\n");

    close(socket_fd);
    printf("Connection closed!\n");
}

void download_files() {
    struct client_request client_request;

   	int number_bytes_writed;
    int number_bytes_read;
    int number_missing_bytes_read;
    int number_files_server;

	FILE* file;
    int file_size;
    int file_index;
    char file_name[MAXNAME];
    char sync_dir_path_client[KBYTE];
	char buffer[KBYTE];

	client_request.command = DOWNLOAD_ALL;
	number_bytes_writed = write(sync_socket, &client_request, sizeof(client_request));

	if (number_bytes_writed >= 0) {
        read(sync_socket, &number_files_server, sizeof(number_files_server));

        for(file_index = 0; file_index < number_files_server; file_index++) {
            // Read file name from server
            number_bytes_read = read(sync_socket, file_name, sizeof(file_name));
            if (number_bytes_read >= 0) {
                strcpy(sync_dir_path_client, sync_client_directory); strcat(sync_dir_path_client, "/"); strcat(sync_dir_path_client, file_name);

                // Create file in client directory
                file = fopen(sync_dir_path_client, "wb");
                // Read file size from server
                read(sync_socket, &file_size, sizeof(int));
                // Missing bytes to read
                number_missing_bytes_read = file_size;

                if (file_size > 0)
                    while(number_missing_bytes_read > 0) {
                        // Read Kbyte from server
                        read(sync_socket, buffer, KBYTE);

                        // Write Kbyte to client
                        if(number_missing_bytes_read > KBYTE)
                            fwrite(buffer, KBYTE, 1, file);
                        else
                            fwrite(buffer, number_missing_bytes_read, 1, file);

                        // decrementa os bytes lidos
                        number_missing_bytes_read -= KBYTE;
                    }

                fclose(file);
            } else
                printf("ERROR: Receive file name %s\n", file_name);
        }
    } else
        printf("ERROR: Download all files\n");
}

void download_file(char *file){
	int number_bytes;
    int number_missing_bytes_read;
    int file_size;

	struct client_request client_request;
	FILE* file;
	char buffer[KBYTE];

	strcpy(client_request.file, file_name);
	client_request.command = DOWNLOAD;

    // Download request
	number_bytes = write(socket_fd, &client_request, sizeof(client_request));
	if (number_bytes < 0)
		printf("ERROR: Download\n");

	// Get file size
	number_bytes = read(socket_fd, &file_size, sizeof(file_size));
	if (number_bytes < 0)
		printf("ERROR: Receiving %s\n", file_name);

	if (file_size < 0) {
		printf("ERROR: File %s do not exist\n", file_name);
		return;
	}

	// Create new file in user local
	file = fopen(file_name, "wb");

	// Missing bytes to read
	number_missing_bytes_read = file_size;

	while(number_missing_bytes_read > 0) {
		// Read one Kbyte from server and write the read bytes from server in the user file
		number_bytes = read(socket_fd, buffer, KBYTE);
		if(number_missing_bytes_read > KBYTE)
			number_bytes = fwrite(buffer, KBYTE, 1, file);
		else
			fwrite(buffer, number_missing_bytes_read, 1, file);

		number_missing_bytes_read -= KBYTE;
	}

	fclose(file);
	printf("SUCCESS: File %s downloaded\n\n", file_name);
}

void upload_file(char *client_file_path, char *file_name, int socket) {
    FILE* file;
    int f_size = 0;
    int number_bytes_writed = 1;
    char buffer[KBYTE];

    struct client_request client_request;

    if(file = fopen(client_file_path, "rb")) {
        strcpy(client_request.file, file_name);
        client_request.command = UPLOAD;

        // Send file name and upload command
        write(socket, &client_request, sizeof(client_request));

        // Send file size
        f_size = file_size(file);
        write(socket, &f_size, sizeof(f_size));

        // Send file data
        if(f_size > 0) {
            while(!feof(file) && number_bytes_writed >= 0) {
                fread(buffer, sizeof(buffer), 1, file);
                number_bytes_writed = write(socket, buffer, KBYTE);
            }

            if(number_bytes_writed < 0)
                printf("ERROR: Upload %s file\n", file_name);

            if (socket != sync_socket)
                printf("SUCCESS: File %s uploaded!\n", file_name);
        }

        // Ever close the files on error ou success
        fclose(file);

    } else
        printf("ERROR: Open file %s in upload_file(...)\n\n", file_name);
}

void delete_file(char *file_name, int socket) {
	int number_bytes_writed;
	struct client_request client_request;

	strcpy(client_request.file, file_name);
	client_request.command = DELETE;

	number_bytes_writed = write(socket, &client_request, sizeof(client_request));

	if (number_bytes_writed < 0)
		printf("ERROR: Delete %s file\n", file_name);
}

void list_files() {
	int number_bytes;
    int number_files_server;
    int file_index;
	struct client_request client_request;
	struct file_data file_data;

	client_request.command = SHOWFILES;
	number_bytes = write(socket_fd, &client_request, sizeof(client_request));
	if(number_bytes < 0)
		printf("ERROR: Connect with server to list files\n");

	// Get number files from server
	number_bytes = read(socket_fd, &number_files_server, sizeof(number_files_server));
	if (number_bytes < 0)
		printf("ERROR: Get files number from server\n");

	if(number_files_server == 0) {
		printf("Empty server directory!\n");
		return;
	}

	for(file_index = 0; file_index < number_files_server; file_index++) {
		number_bytes = read(socket_fd, &file_data, sizeof(file_data));
		printf("\nFile %d: %s \nDate: %ssize: %d\n", file_index, file_data.name, file_data.last_modified, file_data.size);
	}
}
