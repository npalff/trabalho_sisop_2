#include "server_communication.h"

int initialize_client(int client_socket, char *user_id, struct client *client)
{
  struct client_list *client_node;
  struct stat sb;
  int i;

  // não encontrou na lista ---- NEW CLIENT
  if (!find_node_list(user_id, client_node, &client_node))
  {
    client->devices[0] = client_socket;
    client->devices[1] = -1;
    strcpy(client->user_id, user_id);

    for(i = 0; i < MAXFILES; i++)
    {
      client->file_data[i].size = -1;
    }
    client->logged = 1;

    // insere cliente na lista de client
    insert_list(&client_node, *client);
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

  if (stat(user_id, &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    // usuário já tem o diretório com o seu nome
  }
  else
  {
    if (mkdir(user_id, 0777) < 0)
    {
      // erro
      if (errno != EEXIST)
        printf("ERROR creating directory\n");
    }
    // diretório não existe
    else
    {
      printf("Creating %s directory...\n", user_id);
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
