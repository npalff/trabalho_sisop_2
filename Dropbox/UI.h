#include "/server.h"
#include <time.h>

#define UPLOAD 0
#define DOWNLOAD 1
#define DOWNLOADALL 2
#define DELETE 3
#define SHOWFILES 4
#define SYNC 5
#define EXIT 6
#define KBYTE 1024
#define LINE 5
#define COLUMN 13

void create_list(struct client_list *client_list);
void insert_list(struct client_list **client_list, struct client client);
int is_empty_list(struct client_list *client_list);
int find_node_list(char *user_id, struct client_list *client_list, struct client_list **node);
int file_size(FILE *ptr_file);
int request_command(char *request, char *file);
void file_name(char *path_name, char *fname);
time_t last_modified_date(char *path);
int exist(const char *fname);