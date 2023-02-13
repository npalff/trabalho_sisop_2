#ifndef SYNCRONIZATION_CLIENT
#define SYNCRONIZATION_CLIENT

int create_sync_sock();
void sync_client_inicialization(); //void sync_client_first();
void create_notifier_sync_client_directory(); // initializeNotifyDescription();
void *sync_client_directory_thread(); // void *sync_thread();

#endif