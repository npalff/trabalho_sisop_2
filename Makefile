dropbox: tools sv_comm sv_file sv_sync cl_comm cl_sync cl_inte client server



tools: ./tools/tools.c
	gcc  -c ./tools/tools.c && mv ./tools/tools.o ./tools



sv_comm: ./server/server_communication.c
	gcc -c ./server/server_communication.c && mv ./server/server_communication.o ./server

sv_file: ./server/server_file_manager.c
	gcc -c ./server/server_file_manager.c && mv ./server/server_file_manager.o ./server

sv_sync: ./server/server_syncronization.c
	gcc -c ./server/server_syncronization.c && mv ./server/server_syncronization.o ./server



cl_comm: ./client/communication.c
	gcc -c ./client/client_communication.c && mv ./client/client_communication.o ./client

cl_sync: ./client/client_syncronization.c
	gcc -c ./client/client_syncronization.c && mv ./client/client_syncronization.o ./client

cl_inte: ./client/interface.c
	gcc -c ./client/interface.c && mv ./client/interface.o ./client



client: ./client/client.c
	gcc -o client_dropbox ./client/client.c ./tools/tools.o ./client/client_communication.o ./client/client_syncronization.o ./client/client_interface.o -pthread

server: ./server/server.c
	gcc -o server_dropbox ./server/server.c ./tools/tools.o ./server/server_communication.o ./server/server_file_manager.o ./server/server_syncronization.o -pthread

clean:
	rm ./tools/*.o ./server/*.o ./client/*.o client_dropbox server_dropbox