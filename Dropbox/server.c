#include "server.h"
#include "tools.h"


int PORT;
struct client_list *client_list = NULL;;
int writing = 0;

int main(int argc, char* argv[])
{

  PORT = atoi(argv[1]);
  int serverSockfd, newsockfd, thread;
  socklen_t cliLen;
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t clientThread, syncThread;

  initialize_clients();

  if ((serverSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    printf("ERROR opening socket\n");
    return -1;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serv_addr.sin_zero), 8);

  
  if (bind(serverSockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      printf("ERROR on bindining\n");
      return -1;
    }

    printf("\nServer online! Waiting for connection.\n");
  listen(serverSockfd, 5);

  cliLen = sizeof(struct sockaddr_in);

  while(1)
  {
    if((newsockfd = accept(serverSockfd, (struct sockaddr *)&cli_addr, &cliLen)) == -1)
    {
      printf("ERROR on accept\n");
      return -1;
    }

    read(newsockfd, &thread, sizeof(thread));

    if (thread)
    {
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

int initialize_client(int client_socket, char *user_id, struct client *client)
{
  struct client_list *client_node;
  struct stat sb;
  int i;

  if (!find_node_list(user_id, client_list, &client_node))
  {
    client->devices[0] = client_socket;
    client->devices[1] = -1;
    strcpy(client->user_id, user_id);

    for(i = 0; i < MAXFILES; i++)
    {
      client->file_data[i].size = -1;
    }
    client->logged = 1;
    insert_list(&client_list, *client);
  }
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
    else
      return -1;
  }

  if (stat(user_id, &sb) == 0 && S_ISDIR(sb.st_mode))
  {
  }
  else
  {
    if (mkdir(user_id, 0777) < 0)
    {
      if (errno != EEXIST)
        printf("ERROR creating directory\n");
    }
    else
    {
      printf("Creating %s directory...\n", user_id);
    }
  }

  return 1;
}

void *sync_thread_server(void *socket)
{
  int byteCount, connected;
  int *client_socket = (int*)socket;
  char user_id[MAXNAME];
  struct client client;

  byteCount = read(*client_socket, user_id, MAXNAME);

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

  byteCount = read(*client_socket, user_id, MAXNAME);

  if (byteCount < 1)
    printf("ERROR reading from socket\n");

  if (initialize_client(*client_socket, user_id, &client) > 0)
  {
      connected = 1;
      byteCount = write(*client_socket, &connected, sizeof(int));
      if (byteCount < 0)
        printf("ERROR sending connected message\n");

      printf("%s connected!\n", user_id);
  }
  else
  {
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
      }
  } while(clientRequest.command != EXIT);
}

void close_client_connection(int socket, char *user_id)
{
  struct client_list *client_node;
	int i, fileNum = 0;

  printf("Disconnecting %s\n", user_id);

	if (find_node_list(user_id, client_list, &client_node))
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

  strcpy(path, user_id);
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

  strcpy(path, user_id);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "wb"))
  {
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
    		byteCount = read(socket, dataBuffer, KBYTE);

    		if(bytesLeft > KBYTE)
    		{
    			byteCount = fwrite(dataBuffer, KBYTE, 1, ptrfile);
    		}
    		else
    		{
    			fwrite(dataBuffer, bytesLeft, 1, ptrfile);
    		}
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

  strcpy(path, user_id);
  strcat(path, "/");
  strcat(path, file);

  if (ptrfile = fopen(path, "rb"))
  {
      fileSize = file_size(ptrfile);

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
  else
  {
    fileSize = -1;
    byteCount = write(socket, &fileSize, sizeof(fileSize));
  }
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
          client.logged  = 0;

          strcpy(client.user_id, dir->d_name);

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
                  return;
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

                 strcpy(client.file_data[i].name, userDirent->d_name);

                 client.file_data[i].size = st.st_size;

                 client.file_data[i].timestamp_last_modified = st.st_mtime;

                 strcpy(client.file_data[i].last_modified, ctime(&st.st_mtime));

                 i++;
              }
            }
            insert_list(&client_list, client);
          }
       }
    }

    closedir(d);
  }
}
