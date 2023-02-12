

dropbox: ui client server

ui: UI.c
	gcc  -c UI.c && mv UI.o ./bin

client: client.c
	gcc -o client_dropbox client.c ./bin/ui.o -pthread

server: server.c
	gcc -o server_dropbox server.c ./bin/ui.o -pthread