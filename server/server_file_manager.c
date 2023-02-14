#include "server_file_manager.h"

void send_file(char *file, int socket, char *user_id)
{
	int byteCount, bytesLeft, fileSize;
	FILE* ptrfile;
	char dataBuffer[KBYTE], path[KBYTE];

  strcpy(path, "sync_dir_");
  strcat(path, user_id);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "rb"))
  {
      fileSize = file_size(ptrfile);

    	// escreve estrutura do arquivo no servidor
    	byteCount = write(socket, &fileSize, sizeof(int));

      while(!feof(ptrfile))
      {
          fread(dataBuffer, sizeof(dataBuffer), 1, ptrfile);

          byteCount = write(socket, dataBuffer, KBYTE);
          if(byteCount < 0)
            printf("ERROR sending file\n");
      }
      fclose(ptrfile);
  }
  // arquivo não existe
  else
  {
    fileSize = -1;
    byteCount = write(socket, &fileSize, sizeof(fileSize));
  }
}



void receive_file(char *file, int socket, char*user_id)
{
  int byteCount, bytesLeft, fileSize;
  FILE* ptrfile;
  char dataBuffer[KBYTE], path[200];
  struct file_data file_data;
  time_t now;

  strcpy(path, "sync_dir_");
  strcat(path, user_id);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "wb"))
  {
      // escreve número de bytes do arquivo
      byteCount = read(socket, &fileSize, sizeof(fileSize));

      if (fileSize == 0)
      {
        fclose(ptrfile);

      	strcpy(file_data.name, file);
        strcpy(file_data.last_modified, ctime(&now));
        file_data.timestamp_last_modified = now;
        file_data.size = fileSize;

      	update_file_data(user_id, file_data);
        return;
      }

      bytesLeft = fileSize;

      while(bytesLeft > 0)
      {
        	// lê 1kbyte de dados do arquivo do servidor
    		byteCount = read(socket, dataBuffer, KBYTE);

    		// escreve no arquivo do cliente os bytes lidos do servidor
    		if(bytesLeft > KBYTE)
    		{
    			byteCount = fwrite(dataBuffer, KBYTE, 1, ptrfile);
    		}
    		else
    		{
    			fwrite(dataBuffer, bytesLeft, 1, ptrfile);
    		}
    		// decrementa os bytes lidos
    		bytesLeft -= KBYTE;
      }
      fclose(ptrfile);

      time (&now);

      strcpy(file_data.name, file);
      strcpy(file_data.last_modified, ctime(&now));
      file_data.timestamp_last_modified = now;
      file_data.size = fileSize;

      update_file_data(user_id, file_data);
  }
}


void send_all_files(int client_socket, char *user_id)
{
  int byteCount, bytesLeft, fileSize, fileNum=0, i;
  FILE* ptrfile;
  char dataBuffer[KBYTE], path[KBYTE];
  struct client_list *client_node;

  if (find_node_list(user_id, client_list, &client_node))
  {
    for(i = 0; i < MAXFILES; i++)
    {
      if (client_node->client.file_data[i].size != -1)
        fileNum++;
    }
  }

  write(client_socket, &fileNum, sizeof(fileNum));

  for(i = 0; i < MAXFILES; i++)
  {
    if (client_node->client.file_data[i].size != -1)
    {
      strcpy(path, user_id);
      strcat(path, "/");
      strcat(path, client_node->client.file_data[i].name);

      write(client_socket, client_node->client.file_data[i].name, MAXNAME);

      if (ptrfile = fopen(path, "rb"))
      {
          fileSize = file_size(ptrfile);

          // escreve estrutura do arquivo no servidor
          byteCount = write(client_socket, &fileSize, sizeof(int));

          if (fileSize > 0)
          {
            while(!feof(ptrfile))
            {
                fread(dataBuffer, sizeof(dataBuffer), 1, ptrfile);

                byteCount = write(client_socket, dataBuffer, KBYTE);
                if(byteCount < 0)
                  printf("ERROR sending file\n");
            }
          }
          fclose(ptrfile);
      }
    }
  }
}

void send_file_data(int socket, char *user_id)
{
	struct client_list *client_node;
	struct client client;
	int i, fileNum = 0;

	if (find_node_list(user_id, client_list, &client_node))
	{
		client = client_node->client;
		for (i = 0; i < MAXFILES; i++)
		{
			if (client.file_data[i].size != -1)
				fileNum++;
		}

		write(socket, &fileNum, sizeof(fileNum));

		for (i = 0; i < MAXFILES; i++)
		{
			if (client.file_data[i].size != -1)
				write(socket, &client.file_data[i], sizeof(client.file_data[i]));
		}
	}
}


void update_file_data(char *user_id, struct file_data file_data)
{
  struct client_list *client_node;
  int i;

  if (find_node_list(user_id, client_list, &client_node))
  {
    for(i = 0; i < MAXFILES; i++)
      if(!strcmp(file_data.name, client_node->client.file_data[i].name))
        {
          client_node->client.file_data[i] = file_data;
          return;
        }
    for(i = 0; i < MAXFILES; i++)
    {
      if(client_node->client.file_data[i].size == -1)
      {
        client_node->client.file_data[i] = file_data;
        break;
      }
    }
  }
}


void delete_file_all_devices(char *file, int socket, char *user_id)
{
  int byteCount;
  FILE *ptrfile;
  char path[200];
  struct file_data file_data;

  strcpy(path, "sync_dir_");
  strcat(path, user_id);
  strcat(path, "/");
  strcat(path, file);

  if(remove(path) != 0)
  {
    printf("Error: unable to delete the %s file\n", file);
  }

  strcpy(file_data.name, file);
  file_data.size = -1;

  update_file_data(user_id, file_data);
}
