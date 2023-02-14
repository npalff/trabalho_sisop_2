#include "interface.h"

void user_interface() {
    char request[200];
    char file_name[200];
	int command = 0;

	do {
	    printf("\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\nComandos possíveis:\nupload <path>/<filename> -- upload file to server\ndelete <filename> -- delete file from server\ndownload <filename> -- Download do arquivo para o diretorio local\nlist -- lista arquivos do diretorio\nget_sync_dir -- Sincronizar o diretório manualmente\nexit -- Encerra a conexao com o servidor\n");
		printf("\nType your command: ");
		fgets(request, sizeof(request), stdin);
		command = request_command(request, file_name);

		// What is the client command?
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
			default:
                printf("ERROR: Invalid command\n");
		}
	} while(command != EXIT);
}