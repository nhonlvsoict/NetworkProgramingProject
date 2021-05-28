CFLAGS = -Wall
CC = gcc


all: clean server client

clean:
	rm -f client server

server:
	${CC} tcpServer.c ${CFLAGS} -o server

client:
	${CC} tcpClient.c ${CFLAGS} -o client
