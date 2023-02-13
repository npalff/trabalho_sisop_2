#include "server.h"
#include "../tools/tools.h"


int PORT;
struct client_list *client_list = NULL;;
int writing = 0;

int main(int argc, char* argv[])
{

  PORT = atoi(argv[1]);
  int serverSockfd, newsockfd, thread;
  socklen_t client_length;
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t clientThread, syncThread;

  initialize_clients();

  // abre o socket
  if ((serverSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    printf("ERROR opening socket\n");
    return -1;
  }
  // inicializa estrutura do serv_addr
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serv_addr.sin_zero), 8);

  // associa o descritor do socket a estrutura
  if (bind(serverSockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      printf("ERROR on bindining\n");
      return -1;
    }

    printf("Server online! Waiting for connection.\n\n");
  // espera pela tentativa de conexão de algum cliente
  listen(serverSockfd, 5);

  client_length = sizeof(struct sockaddr_in);

  while(1)
  {
    // socket para atender requisição do cliente
    if((newsockfd = accept(serverSockfd, (struct sockaddr *)&cli_addr, &client_length)) == -1)
    {
      printf("ERROR on accept\n");
      return -1;
    }

    read(newsockfd, &thread, sizeof(thread));

    if (thread)
    {
      // cria thread para atender o cliente
      if(pthread_create(&clientThread, NULL, client_thread, &newsockfd))
      {
        printf("ERROR creating thread\n");
        return -1;
      }
    }
    else
    {
      if(pthread_create(&syncThread, NULL, sync_thread_server, &newsockfd))
      {
        printf("ERROR creating sync thread\n");
        return -1;
      }
    }
  }
}


void *sync_thread_server(void *socket)
{
  int byteCount, connected;
  int *client_socket = (int*)socket;
  char user_id[MAXNAME];
  struct client client;

  // lê os dados de um cliente
  byteCount = read(*client_socket, user_id, MAXNAME);

  // erro de leitura
  if (byteCount < 1)
    printf("ERROR reading from socket\n");

  listen_sync(*client_socket, user_id);
}

void listen_sync(int client_socket, char *user_id)
{
  int byteCount, command;
  struct client_request clientRequest;

  do
  {
      byteCount = read(client_socket, &clientRequest, sizeof(clientRequest));

      switch (clientRequest.command)
      {
        case UPLOAD: receive_file(clientRequest.file, client_socket, user_id); break;
        case DOWNLOAD_ALL: send_all_files(client_socket, user_id); break;
        case DELETE: delete_file_all_devices(clientRequest.file, client_socket, user_id);
        case EXIT: ;break;
        default: break;
      }
  } while(clientRequest.command != EXIT);
}

void *client_thread (void *socket)
{
  int byteCount, connected;
  int *client_socket = (int*)socket;
  char user_id[MAXNAME];
  struct client client;

  // lê os dados de um cliente
  byteCount = read(*client_socket, user_id, MAXNAME);

  // erro de leitura
  if (byteCount < 1)
    printf("ERROR reading from socket\n");

  // inicializa estrutura do client
  if (initialize_client(*client_socket, user_id, &client) > 0)
  {
      // avisamos cliente que conseguiu conectar
      connected = 1;
      byteCount = write(*client_socket, &connected, sizeof(int));
      if (byteCount < 0)
        printf("ERROR sending connected message\n");

      printf("%s connected!\n", user_id);
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

  listen_client(*client_socket, user_id);
}

void listen_client(int client_socket, char *user_id)
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
        case SHOWFILES: send_file_data(client_socket, user_id); break;
        case DOWNLOAD: send_file(clientRequest.file, client_socket, user_id); break;
        case UPLOAD: receive_file(clientRequest.file, client_socket, user_id); break;
        case EXIT: close_client_connection(client_socket, user_id);break;
        case DELETE: delete_file_all_devices(clientRequest.file, client_socket, user_id);break;
  //      default: printf("ERROR invalid command\n");
      }
  } while(clientRequest.command != EXIT);
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

