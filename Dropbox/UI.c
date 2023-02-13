
#include "UI.h"

char command[LINE][COLUMN] = {"upload", "download", "list", "get_sync_dir", "exit"};

void insert_list(struct client_list **client_list, struct client client) {
	struct client_list *node;
	struct client_list *list_aux = *client_list;

	node = malloc(sizeof(struct client_list));

	node->client = client;
	node->next = NULL;

	if (*client_list == NULL)
		*client_list = node;

	else {
		while(list_aux->next != NULL)
			list_aux = list_aux->next;

		list_aux->next = node;
	}
}

int is_empty_list(struct client_list *client_list) {
	return client_list == NULL;
}

int find_node_list(char *user_id, struct client_list *client_list, struct client_list **node) {
	struct client_list *list_aux = client_list;

	while(list_aux != NULL) {
		if (strcmp(user_id, list_aux->client.user_id) == 0) {
			*node = list_aux;
			return 1;
		}
		else
			list_aux = list_aux->next;
	}
	return 0;
}

int file_size(FILE *ptr_file) {
	int size;

	fseek(ptr_file, 0L, SEEK_END);
	size = ftell(ptr_file);

	rewind(ptr_file);

	return size;
}

int request_command(char *request, char *file) {
	char *aux_request, *aux_file;
	int strLen;

	strLen = strlen(request);

	if ((strLen > 0) && (request[strLen-1] == '\n'))
		  request[strLen-1] = '\0';

	if (!strcmp(request, command[SHOWFILES]))
		return SHOWFILES;

	else if (!strcmp(request, command[SYNC]))
		return SYNC;

    else if (!strcmp(request, command[EXIT]))
		return EXIT;

	aux_request = strtok(request, " ");
	aux_file = strtok(NULL, "\n");

	if (aux_file != NULL)
		strcpy(file, aux_file);

	else
		return -1;

	if (file != NULL) {
		if (!strcmp(aux_request, command[DOWNLOAD]))
			return DOWNLOAD;

		else if (!strcmp(aux_request, command[UPLOAD]))
                return UPLOAD;
	}
	
	return -1;
}

void file_name(char *path_name, char *fname) {
	char *aux_file_name;

	aux_file_name = strtok(path_name, "/");

	strcpy(fname, aux_file_name);

	while(aux_file_name != NULL) {
		strcpy(fname, aux_file_name);

		aux_file_name = strtok(NULL, "/");
	}
}

time_t last_modified_date(char *path) {
    struct stat attr;

    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }

    return 0;
}

int exist(const char *fname) {
    FILE *file;

    if ((file = fopen(fname, "rb"))) {
        fclose(file);
        return 1;
    }

    return 0;
}
