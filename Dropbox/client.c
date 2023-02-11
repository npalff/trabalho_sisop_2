
void *monitoringClientDirectory() {

    char file_path[200];

    char buffer[BUFFER_LENGTH];
    int notify_read_length = 0;
    int reading_notify = 0;

    //create_sync_sock();
    //get_all_files();

    while(TRUE) {
        notify_read_length = read(notifier, buffer, BUFFER_LENGTH);
        if(notify_read_length < 0)
            perror("ERROR: Monitoring client directory");

        while(reading_notify < notify_read_length) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if(event->len) {
            }
        }

    }
}