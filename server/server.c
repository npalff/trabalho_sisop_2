
#include "server.h"
#include "server_communication.h"
#include "server_syncronization.h"
#include "server_file_manager.h"
#include "../tools/tools.h"

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



















