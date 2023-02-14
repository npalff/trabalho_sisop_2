#ifndef SYNCRONIZATION_CLIENT
#define SYNCRONIZATION_CLIENT

int create_sync_socket();
void sync_client_inicialization();
void create_notifier_sync_client_directory();
void *sync_client_directory_thread();

#endif