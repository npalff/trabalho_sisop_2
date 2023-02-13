

dropbox: ui client server

ui: UI.c
	gcc  -c UI.c && mv UI.o ./bin

client: client.c
	gcc -o client_dropbox client.c ./bin/UI.o -pthread

server: server.c
	gcc -o server_dropbox server.c ./bin/UI.o -pthread

clean:
	rm ./bin/UI.o client_dropbox server_dropbox
