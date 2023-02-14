#include "syncronization.h"

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

	// Initialize server
	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_port = htons(port);
	sockaddr_in.sin_addr = *((struct in_addr *) server->h_addr);

	bzero(&(sockaddr_in.sin_zero), 8);

	// Try connect to socket
	if (connect(sync_socket,(struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in)) < 0)
		return -1;

	write(sync_socket, &client_thread, sizeof(client_thread));

	// Send identifier user to server
	number_bytes_writed = write(sync_socket, user_id, sizeof(user_id));
}

void sync_client_inicialization()
{
    pthread_t thread;

    char file_name[MAXNAME + 10] = "sync_dir_";
	char *home_directory;

	if ((home_directory = getenv("HOME")) == NULL)
        home_directory = getpwuid(getuid())->pw_dir;
	
    // File name
	strcat(file_name, user_id);
	// Syncronization client directory path
	strcpy(sync_client_directory, home_directory); strcat(sync_client_directory, "/"); strcat(sync_client_directory, file_name);

	if (mkdir(sync_client_directory, 0777) < 0)
		if (errno != EEXIST)
			printf("ERROR: Create %s directory\n", sync_client_directory);
	else
		printf("SUCCESS INICIALIZATION: Creating %s directory in your home\n", file_name);

	create_notifier_sync_client_directory();

	// Syncronization thread client directory 
	if(pthread_create(&thread, NULL, sync_client_directory_thread, NULL))
		printf("ERROR: Create syncronization thread client directory\n");
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
        // Reading directory events
        notify_read_length = read(notifier, buffer, BUFFER_LENGTH);
        if(notify_read_length < 0)
            perror("ERROR: Syncronization client directory");

        // While events to read
        while(notify_reading < notify_read_length) {
            struct inotify_event *event = (struct inotify_event*) &buffer[notify_reading];
            if(event->len) {
                // Concatenation client file path
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