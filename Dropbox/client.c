void *sync_client_directory() {

    char file_path[200]; // path

    char buffer[BUFFER_LENGTH]; //buffer
    int notify_read_length = 0; //length
    int notify_reading = 0; // i

    //create_sync_sock();
    //get_all_files();

    while(TRUE) {
        // Reading of directory events
        notify_read_length = read(notifier, buffer, BUFFER_LENGTH);
        if(notify_read_length < 0)
            perror("ERROR: Monitoring client directory");

        while(notify_reading < notify_read_length) {
            struct inotify_event *event = (struct inotify_event*) &buffer[notify_reading];
            if(event->len) {
                // Concatenation file path
                strcpy(path, directory); strcat(path, "/"); strcat(path, event->name);
                if(event->mask & IN_CLOSE_WRITE || event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
					if(exists(path) && (event->name[0] != '.'))
						//upload_file(path, sync_socket);
                    else
                        perror("ERROR: Sync client directory - IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO");
                }
                if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM){
					if(event->name[0] != '.')
					    //delete_file_request(path, sync_socket);
                    else
                        perror("ERROR: Sync client directory - IN_DELETE | IN_MOVED_FROM");
				}
            }
            notify_reading += EVENT_SIZE + event->len;
        }
        notify_reading = 0;
        sleep(10);
    }
}