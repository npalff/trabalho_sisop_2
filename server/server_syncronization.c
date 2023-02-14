#include "server_syncronization.h";

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

