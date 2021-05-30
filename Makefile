CFLAGS = -c -Wall
CC = gcc


all: clean server

clean:
	rm -f server

server: tcpServer.o childProcess.o serverProps.o
	${CC} tcpServer.o childProcess.o serverProps.o -o server

tcpServer.o: tcpServer.c
	${CC} ${CFLAGS} tcpServer.c

childProcess.o: childProcess.c
	${CC} ${CFLAGS} childProcess.c
serverProps.o: serverProps.c
	${CC} ${CFLAGS} serverProps.c

client:
	${CC} tcpClient.c ${CFLAGS} -o client

cleanall:
	rm -f *.o *~

