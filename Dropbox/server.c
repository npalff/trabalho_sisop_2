
#include "server.h"
#include "UI.h"

int PORT;

struct client_list* clients;
int writing = 0;

int main(int argc, char* argv[])
{
    if(argc != 1)
    {
        printf("Error: wrong call \n\n To run the server, call: ./server <port number>");
    }
    else
        PORT = atoi(argv[1]);

    int server_socket, new_socket, thread;
    socklen_t client_len;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t client_thread, sync_thread;

    // inicializa lista de clientes do servidor
    newList(clients);

    initializeClientList();

    // abre o socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("ERROR opening socket\n");
        return -1;
    }
    // inicializa estrutura do server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(serv_addr.sin_zero), 8);

    // associa o descritor do socket a estrutura
    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
        printf("ERROR on bindining\n");
        return -1;
        }

    // espera pela tentativa de conexão de algum cliente
    listen(server_socket, 5);

    client_len = sizeof(struct sockaddr_in);

    while(1)
    {
        // socket para atender requisição do cliente
        if((new_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &client_len)) == -1)
        {
        printf("ERROR on accept\n");
        return -1;
        }

        read(new_socket, &thread, sizeof(thread));

        if (thread)
        {
        // cria thread para atender o cliente
        if(pthread_create(&client_thread, NULL, client_thread, &new_socket))
        {
            printf("ERROR creating thread\n");
            return -1;
        }
        }
        else
        {
        if(pthread_create(&sync_thread, NULL, sync_thread_sv, &new_socket))
        {
            printf("ERROR creating sync thread\n");
            return -1;
        }
        }
    }

}

int initialize_client(int client_socket, char *userid, struct client *client)
{
    struct client_list* client_node;
    struct stat sb;
    int i;
    char* directory_name;
    directory_name = strcat("sync_dir_",userid);

    // não encontrou na lista ---- NEW CLIENT
    if (!findNode(userid, clients, &client_node))
    {
      client->devices[0] = client_socket;
      client->devices[1] = -1;
      strcpy(client->userid, userid);

      for(i = 0; i < MAXFILES; i++)
      {
        client->file_data[i].size = -1;
      }
      client->logged = 1;

      // insere cliente na lista de client
      insertList(&clients, *client);
    }
    // encontrou CLIENT na lista, atualiza device
    else
    {
      if(client_node->client.devices[0] == FREEDEV)
      {
        client_node->client.devices[0] = client_socket;
      }
      else if (client_node->client.devices[1] == FREEDEV)
      {
        client_node->client.devices[1] = client_socket;
      }
      // caso em que cliente já está conectado em 2 dipostivos
      else
        return -1;
    }

    if (stat(directory_name, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
      // usuário já tem o diretório com o seu nome
    }
    else
    {
      if (mkdir(directory_name, 0777) < 0)
      {
        // erro
        if (errno != EEXIST)
          printf("ERROR creating directory\n");
      }
      // diretório não existe
      else
      {
        printf("Creating %s directory...\n", directory_name);
      }
    }

    return 1;
}

void initialize_clients()
{
  struct client client;
  DIR *d, *userDir;
  struct dirent *dir, *userDirent;
  int i = 0;
  FILE *file_d;
  struct stat st;
  char folder[MAXNAME], path[200];

  d = opendir(".");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if (dir->d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0)
       {
          client.devices[0] = FREEDEV;
          client.devices[1] = FREEDEV;
          client.logged = 0;

          strcpy(client.userid, dir->d_name);

          userDir = opendir(dir->d_name);

          strcpy(folder, dir->d_name);
          strcat(folder, "/");

          if (userDir)
          {
            for(i = 0; i < MAXFILES; i++)
            {
              client.file_data[i].size = -1;
              if (pthread_mutex_init(&client.file_data[i].file_mutex, NULL) != 0)
              {
                  printf("\n mutex init failed\n");
                  return 1;
              }
            }
            i = 0;
            while((userDirent = readdir(userDir)) != NULL)
            {
              if(userDirent->d_type == DT_REG && strcmp(userDirent->d_name,".")!=0 && strcmp(userDirent->d_name,"..")!=0)
              {
                 strcpy(path, folder);
                 strcat(path, userDirent->d_name);

                 stat(path, &st);

                 strcpy(client.file_info[i].name, userDirent->d_name);

                 client.file_data[i].size = st.st_size;

                 client.file_data[i].timestamp_last_modified = st.st_mtime;

                 strcpy(client.file_data[i].last_modified, ctime(&st.st_mtime));

                 i++;
              }
            }
            insertList(&clients, client);
          }
       }
    }

    closedir(d);
  }
}



void close_client_connection(int socket, char* userid)
{
    struct client_list *client_node;
	int i, fileNum = 0;

  printf("Disconnecting %s\n", userid);

	if (findNode(userid, clients, &client_node))
	{
    if(client_node->client.devices[0] == FREEDEV)
    {
      client_node->client.devices[1] = FREEDEV;
      client_node->client.logged = 0;
    }
    else if (client_node->client.devices[1] == FREEDEV)
    {
      client_node->client.devices[0] = FREEDEV;
      client_node->client.logged = 0;
    }
    else if (client_node->client.devices[0] == socket)
      client_node->client.devices[0] = FREEDEV;
    else
      client_node->client.devices[1] = FREEDEV;
  }
}

void sync_server(int socket, char *userid);
void listen_sync(int client_socket, char *userid)
{
  int byteCount, command;
  struct client_request clientRequest;

  do
  {
      byteCount = read(client_socket, &clientRequest, sizeof(clientRequest));

      switch (clientRequest.command)
      {
        case UPLOAD: receive_file(clientRequest.file, client_socket, userid); break;
        case DOWNLOAD_ALL: send_all_files(client_socket, userid); break;
        case DELETE: delete_file(clientRequest.file, client_socket, userid);
        case EXIT: ;break;
        default: break;
      }
  } while(clientRequest.command != EXIT);
}


void *client_thread (void *socket)
{
  int byteCount, connected;
  int *client_socket = (int*)socket;
  char userid[MAXNAME];
  struct client client;

  // lê os dados de um cliente
  byteCount = read(*client_socket, userid, MAXNAME);

  // erro de leitura
  if (byteCount < 1)
    printf("ERROR reading from socket\n");

  // inicializa estrutura do client
  if (initializeClient(*client_socket, userid, &client) > 0)
  {
      // avisamos cliente que conseguiu conectar
      connected = 1;
      byteCount = write(*client_socket, &connected, sizeof(int));
      if (byteCount < 0)
        printf("ERROR sending connected message\n");

      printf("%s connected!\n", userid);
  }
  else
  {
    // avisa cliente que não conseguimos conectar
    connected = 0;
    byteCount = write(*client_socket, &connected, sizeof(int));
    if (byteCount < 0)
      printf("ERROR sending connected message\n");

    return NULL;
  }

  listen_client(*client_socket, userid);
}

void *sync_thread_sv(void *socket)
{
  int byteCount, connected;
  int *client_socket = (int*)socket;
  char userid[MAXNAME];
  struct client client;

  // lê os dados de um cliente
  byteCount = read(*client_socket, userid, MAXNAME);

  // erro de leitura
  if (byteCount < 1)
    printf("ERROR reading from socket\n");

  listen_sync(*client_socket, userid);
}

void listen_client(int client_socket, char *userid)
{
  int byteCount, command;
  struct client_request clientRequest;

  do
  {
      byteCount = read(client_socket, &clientRequest, sizeof(clientRequest));

      if (byteCount < 0)
        printf("ERROR listening to the client\n");

      switch (clientRequest.command)
      {
        case SHOWFILES: send_file_info(client_socket, userid); break;
        case DOWNLOAD: send_file(clientRequest.file, client_socket, userid); break;
        case UPLOAD: receive_file(clientRequest.file, client_socket, userid); break;
        case EXIT: close_client_connection(client_socket, userid);break;
  //      default: printf("ERROR invalid command\n");
      }
  } while(clientRequest.command != EXIT);
}

void send_file(char *file, int socket, char *userid)
{
    int byteCount, bytesLeft, fileSize;
	FILE* ptrfile;
	char dataBuffer[KBYTE], path[KBYTE];

  strcpy(path, "sync_dir_");
  strcat(path, userid);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "rb"))
  {
      fileSize = getFileSize(ptrfile);

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
void receive_file(char *file, int socket, char*userid)
{
  int byteCount, bytesLeft, fileSize;
  FILE* ptrfile;
  char dataBuffer[KBYTE], path[200];
  struct file_data file_info;
  time_t now;

  strcpy(path, "sync_dir_");
  strcat(path, userid);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "wb"))
  {
      // escreve número de bytes do arquivo
      byteCount = read(socket, &fileSize, sizeof(fileSize));

      if (fileSize == 0)
      {
        fclose(ptrfile);

      	strcpy(file_info.name, file);
        strcpy(file_info.last_modified, ctime(&now));
        file_info.timestamp_last_modified = now;
        file_info.size = fileSize;

      	updateFileInfo(userid, file_info);
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

      strcpy(file_info.name, file);
      strcpy(file_info.last_modified, ctime(&now));
      file_info.timestamp_last_modified = now;
      file_info.size = fileSize;

      updateFileInfo(userid, file_info);
  }

}

void send_all_files(int client_socket, char *userid)
{
  int byteCount, bytesLeft, fileSize, fileNum=0, i;
  FILE* ptrfile;
  char dataBuffer[KBYTE], path[KBYTE];
  struct client_list *client_node;

  if (findNode(userid, clients, &client_node))
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
      strcpy(path, userid);
      strcat(path, "/");
      strcat(path, client_node->client.file_data[i].name);

      write(client_socket, client_node->client.file_data[i].name, MAXNAME);

      if (ptrfile = fopen(path, "rb"))
      {
          fileSize = getFileSize(ptrfile);

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

void send_file_info(int socket, char *userid)
{
    struct client_list *client_node;
	struct client client;
	int i, fileNum = 0;

	if (findNode(userid, clients, &client_node))
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
void update_file_info(char *userid, struct file_data file_data)
{
    struct client_list *client_node;
    int i;

    if (findNode(userid, clients, &client_node))
    {
        for(i = 0; i < MAXFILES; i++)
        if(!strcmp(file_info.name, client_node->client.file_data[i].name))
            {
            client_node->client.file_data[i] = file_info;
            return;
            }
        for(i = 0; i < MAXFILES; i++)
        {
        if(client_node->client.file_data[i].size == -1)
        {
            client_node->client.file_data[i] = file_info;
            break;
        }
        }
    }
}

void delete_file(char *file, int socket, char *userid)
{
  int byteCount;
  FILE *ptrfile;
  char path[200];
  struct file_data file_info;
  
  strcpy(path, "sync_dir_");
  strcat(path, userid);
  strcat(path, "/");
  strcat(path, file);

  if(remove(path) != 0)
  {
    printf("Error: unable to delete the %s file\n", file);
  }

  strcpy(file_info.name, file);
  file_info.size = -1;

  updateFileInfo(userid, file_info);
  return;
}


