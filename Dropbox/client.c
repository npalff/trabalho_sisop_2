#include "client.h"
#include "tools.h"

// Global variables
char user_id[MAXNAME];
char sync_client_directory[MAXNAME + 50];
char *host;

int port;
int socket_fd = -1;
int sync_socket = -1;
int notifier;
int watch;

void main(int argc, char *argv[]) {
	if (argc < 3) {
        printf("Insufficient arguments\n");
        exit(0);
	}

	if (strlen(argv[1]) <= MAXNAME)
		strcpy(user_id, argv[1]);

	host = malloc(sizeof(argv[2]));
	strcpy(host, argv[2]);

	port = atoi(argv[3]);

	if ( open_connection_with_server(host, port) > 0) {
		sync_client_inicialization();
		user_interface();
	}

	return;
}

void user_interface() {
    char request[200];
    char file_name[200];
	int command = 0;

	do {
	    printf("\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\nComandos possíveis:\nupload <path>/<filename> -- upload file to server\ndelete <filename> -- delete file from server\ndownload <filename> -- Download do arquivo para o diretorio local\nlist -- lista arquivos do diretorio\nget_sync_dir -- Sincronizar o diretório manualmente\nexit -- Encerra a conexao com o servidor\n");
		printf("\ntype your command: ");
		fgets(request, sizeof(request), stdin);
		command = request_command(request, file_name);

		switch (command) {
            case SYNC:
                download_files();
                break;
            case DOWNLOAD:
                download_file(file_name);
                break;
            case UPLOAD: 
                upload_file(file_name, file_name,socket_fd);
                //sleep(5);
                //download_files();
                break;
			case SHOWFILES:
                list_files();
                break;
            case EXIT:
                close_connection_with_server();
                break;
            case DELETE:
                delete_file(file_name, socket_fd);
                break;


			default: printf("ERROR: Invalid command\n");
		}
	} while(command != EXIT);
}

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

void close_connection_with_server() {
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


int create_sync_socket() {
    int number_bytes_writed;
    char buffer[256];
	int connected;
    int client_thread = 0;

	struct sockaddr_in sockaddr_in;
	struct hostent *server;

	server = gethostbyname(host);
	if (server == NULL)
  	    return -1;

	if ((sync_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_port = htons(port);
	sockaddr_in.sin_addr = *((struct in_addr *) server->h_addr);

	bzero(&(sockaddr_in.sin_zero), 8);

	if (connect(sync_socket,(struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in)) < 0)
		return -1;

	write(sync_socket, &client_thread, sizeof(client_thread));

	number_bytes_writed = write(sync_socket, user_id, sizeof(user_id));
}

void create_notifier_sync_client_directory() {
	notifier = inotify_init();
	watch = inotify_add_watch(notifier, sync_client_directory, IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
}

void *sync_client_directory_thread() {
    char client_file_path[200];

    char buffer[BUFFER_LENGTH];
    int notify_read_length = 0;
    int notify_reading = 0;

    create_sync_socket();
    download_files();

    while(TRUE) {
        notify_read_length = read(notifier, buffer, BUFFER_LENGTH);
        if(notify_read_length < 0)
            perror("ERROR: Syncronization client directory");

        while(notify_reading < notify_read_length) {
            struct inotify_event *event = (struct inotify_event*) &buffer[notify_reading];
            if(event->len) {
                strcpy(client_file_path, sync_client_directory); strcat(client_file_path, "/"); strcat(client_file_path, event->name);

                if(event->mask & IN_CLOSE_WRITE || event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
					if(exist(client_file_path) && (event->name[0] != '.'))
					    upload_file(client_file_path, event->name, sync_socket);
                }

                if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM){
					if(event->name[0] != '.')
					    delete_file(event->name, sync_socket);
				}
            }
            notify_reading += EVENT_SIZE + event->len;
        }
        notify_reading = 0;
        sleep(10);
    }
}

void sync_client_inicialization()
{
    pthread_t thread;

    char file_name[MAXNAME + 10] = "sync_dir_";
	char *home_directory;

	if ((home_directory = getenv("HOME")) == NULL)
        home_directory = getpwuid(getuid())->pw_dir;
	
	strcat(file_name, user_id);
	strcpy(sync_client_directory, home_directory); strcat(sync_client_directory, "/"); strcat(sync_client_directory, file_name);

	if (mkdir(sync_client_directory, 0777) < 0)
		if (errno != EEXIST)
			printf("ERROR: Create %s directory\n", sync_client_directory);
	else
		printf("SUCCESS INICIALIZATION: Creating %s directory in your home\n", file_name);

	create_notifier_sync_client_directory();

	if(pthread_create(&thread, NULL, sync_client_directory_thread, NULL))
		printf("ERROR: Create syncronization thread client directory\n");
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

                        number_missing_bytes_read -= KBYTE;
                    }

                fclose(file);
            } else
                printf("ERROR: Receive file name %d\n", file_index);
        } 
    } else
        printf("ERROR: Download all files\n");
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
		printf("Error sending SHOWFILES message to server\n");

	number_bytes = read(socket_fd, &number_files_server, sizeof(number_files_server));
	if (number_bytes < 0)
		printf("Error receiving filesize\n");

	if(number_files_server == 0) {
		printf("Empty directory\n\n\n");
		return;
	}

	for(file_index = 0; file_index < number_files_server; file_index++) {
		number_bytes = read(socket_fd, &file_data, sizeof(file_data));
		printf("File: %d\tName: %s\tLast Modified:%s\tSize: %d bytes\n", file_index, file_data.name, file_data.last_modified, file_data.size);
	}
}

void download_file(char *file_name) {
	int number_bytes;
    int number_missing_bytes_read;
    int file_size;
    
	struct client_request client_request;
	FILE* file;
	char buffer[KBYTE];

	strcpy(client_request.file, file_name);
	client_request.command = DOWNLOAD;

	number_bytes = write(socket_fd, &client_request, sizeof(client_request));
	if (number_bytes < 0)
		printf("ERROR: Download message to server\n");

	number_bytes = read(socket_fd, &file_size, sizeof(file_size));
	if (number_bytes < 0)
		printf("ERROR: Receiving file_size\n");

	if (file_size < 0) {
		printf("File do not exist\n\n\n");
		return;
	}
	file = fopen(file_name, "wb");

	number_missing_bytes_read = file_size;

	while(number_missing_bytes_read > 0) {
		number_bytes = read(socket_fd, buffer, KBYTE);

		if(number_missing_bytes_read > KBYTE) {
			number_bytes = fwrite(buffer, KBYTE, 1, file);
		} else {
			fwrite(buffer, number_missing_bytes_read, 1, file);
		}

		number_missing_bytes_read -= KBYTE;
	}

	fclose(file);
	printf("File %s has been downloaded\n\n", file_name);
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

        write(socket, &client_request, sizeof(client_request));
        
        f_size = file_size(file);
        write(socket, &f_size, sizeof(f_size));

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

        fclose(file);

	} else
        printf("ERROR: Open file %s in upload_file(...)\n\n", file_name);
}

void delete_file(char* file_name, int socket) {
	int number_bytes_writed;
	struct client_request client_request;

	strcpy(client_request.file, file_name);
	client_request.command = DELETE;

	number_bytes_writed = write(socket, &client_request, sizeof(client_request));

	if (number_bytes_writed < 0)
		printf("ERROR: Delete %s file\n", file_name);
}
