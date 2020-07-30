OBJS = dropbox_client.o dropbox_server.o clientInfo.o clientList.o networkFunctions.o circularBuffer.o workerThreads.o clientFunctions.o
SOURCE = dropbox_client.c dropbox_server.c clientInfo.c clientList.c networkFunctions.c circularBuffer.c workerThreads.c clientFunctions.c
HEADER = clientInfo.h networkFunctions.h clientList.h circlularBuffer.h workerThreads.h clientFunctions.h

CC = gcc
FLAGS = -g -c

all: dropbox_client dropbox_server

dropbox_client : dropbox_client.o clientInfo.o clientList.o circularBuffer.o workerThreads.o clientFunctions.o
	$(CC) -g dropbox_client.o clientInfo.o clientList.o circularBuffer.o workerThreads.o clientFunctions.o -lpthread -o dropbox_client

dropbox_server : dropbox_server.o clientList.o networkFunctions.o
	$(CC) -g dropbox_server.o clientList.o networkFunctions.o -o dropbox_server

dropbox_client.o : dropbox_client.c
	$(CC) $(FLAGS) dropbox_client.c

dropbox_server.o : dropbox_server.c
	$(CC) $(FLAGS) dropbox_server.c

clientInfo.o : clientInfo.c
	$(CC) $(FLAGS) clientInfo.c

clientList.o : clientList.c
	$(CC) $(FLAGS) clientList.c

networkFunctions.o : networkFunctions.c
	$(CC) $(FLAGS) networkFunctions.c

circularBuffer.o : circularBuffer.c
	$(CC) $(FLAGS) circularBuffer.c

workerThreads.o : workerThreads.c
	$(CC) $(FLAGS) workerThreads.c

clientFunctions.o : clientFunctions.c
	$(CC) $(FLAGS) clientFunctions.c

clean :
	rm -f $(OBJS) dropbox_client dropbox_server