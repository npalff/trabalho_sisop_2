void *sync_client_directory() {
    char client_file_path[200]; // path

    char buffer[BUFFER_LENGTH]; //buffer
    int notify_read_length = 0; //length
    int notify_reading = 0; // i

    //create_sync_sock();
    download_files(); //get_all_files();

    while(TRUE) {
        // Reading directory events
        notify_read_length = read(notifier, buffer, BUFFER_LENGTH);
        if(notify_read_length < 0)
            perror("ERROR: Syncronization client directory");

        // While events to read
        while(notify_reading < notify_read_length) {
            struct inotify_event *event = (struct inotify_event*) &buffer[notify_reading];
            if(event->len) {
                // Concatenation client file path
                strcpy(client_file_path, directory); strcat(client_file_path, "/"); strcat(client_file_path, event->name);

                if(event->mask & IN_CLOSE_WRITE || event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
					if(exists(path) && (event->name[0] != '.'))
					    upload_file(client_file_path, file_name, sync_socket);
                }

                if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM){
					if(event->name[0] != '.')
					    delete_file(file_name, sync_socket);
				}
            }
            notify_reading += EVENT_SIZE + event->len;
        }
        notify_reading = 0;
        sleep(10);
    }
}

void download_files() {
    struct client_request client_request; // clientRequest

   	int number_bytes_writed; // byteCount
    int number_bytes_read; // byteCount
    int number_missing_bytes_read; //  bytesLeft
    int number_files_server; // fileNum

	FILE* file; // ptrfile
    int file_size; // fileSize
    int file_index; // i
    char file_name[MAXNAME]; // file
    char sync_dir_path_client[KBYTE]; // path
	char buffer[KBYTE]; // dataBuffer

	// copia nome do arquivo e comando para enviar para o servidor
	client_request.command = DOWNLOAD_ALL;
	// avisa servidor que será feito um download
	number_bytes_writed = write(sync_socket, &client_request, sizeof(client_request));

	if (number_bytes_writed >= 0) {
        read(sync_socket, &number_files_server, sizeof(number_files_server));

        for(file_index = 0; file_index < number_files_server; file_index++) {
            // Read file name from server
            number_bytes_read = read(sync_socket, file_name, sizeof(file_name));
            if (number_bytes_read >= 0) {
                strcpy(sync_dir_path_client, directory); strcat(sync_dir_path_client, "/"); strcat(sync_dir_path_client, file_name);

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
                printf("ERROR: Receive file name %d\n", file_index);
        } 
    } else
        printf("ERROR: Download all files\n");
}

void upload_file(char *client_file_path, char *file_name, int socket) {
    FILE* file;
    int file_size = 0;
	int number_bytes_writed = 1;
	char buffer[KBYTE];

	struct client_request client_request;

	if(file = fopen(client_file_path, "rb")) {
        client_request.file = file_name;
        client_request.command = UPLOAD;

        // Send file name and upload command
        write(socket, &client_request, sizeof(client_request));
        
        // Send file size
        file_size = file_size(file);
        write(socket, &file_size, sizeof(file_size));

        // Send file data
        if(file_size > 0) {
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

void delete_file(char* file_name, int socket) {
	int number_bytes_writed;
	struct client_request client_request;

	client_request.file = file_name;
	client_request.command = DELETE;

	number_bytes_writed = write(socket, &client_request, sizeof(client_request));

	if (number_bytes_writed < 0)
		printf("ERROR: Delete %s file\n", file_name);
}
