

dropbox: tool client server

tool: tools.c
	gcc  -c tools.c && mv tools.o ./bin

client: client.c
	gcc -o client_dropbox client.c ./bin/tools.o -pthread

server: server.c
	gcc -o server_dropbox server.c ./bin/tools.o -pthread

clean:
	rm ./bin/tools.o client_dropbox server_dropbox
