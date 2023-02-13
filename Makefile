

dropbox: tools client server

tools: tools.c
	gcc  -c tools.c && mv tools.o ./tools

client: client.c
	gcc -o client_dropbox client.c ./tools/tools.o -pthread

server: server.c
	gcc -o server_dropbox server.c ./tools/tools.o -pthread

clean:
	rm ./tools/tools.o client_dropbox server_dropbox
