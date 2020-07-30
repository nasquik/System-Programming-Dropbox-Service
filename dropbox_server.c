#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>		
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <sys/select.h>
#include "networkFunctions.h"
#include "clientList.h"

int terminate = 0;

void terminationHandler(int signum){
    terminate = 1;
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int service_client(int fileDesc, clientList*);

int main(int argc, char* argv[]){

    // Get arguments

    int portNum;

    if(argc < 3 || argc > 3){
        printf("Incorrect Input!\n");
        return -1;
    }
    else{
        if(strcmp(argv[1], "-p") == 0)
            portNum = atoi(argv[2]);
        else{
            printf("Incorrect Input!\n");
            return -1;
        }
    }
    
    // Set up signal handler //

    struct sigaction terminator = {};
    
    terminator.sa_handler = terminationHandler;
    terminator.sa_flags   = SA_NODEFER;
    
    if(sigaction(SIGINT, &terminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}
    
    if(sigaction(SIGQUIT, &terminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}

    // Print HostName and IP //

    getHostNameIP();

    // Bind socket for communication with clients //

    int sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);
    fd_set active_fd_set, read_fd_set;

    struct sockaddr* serverptr = (struct sockaddr*) &server;
    struct sockaddr* clientptr = (struct sockaddr*) &client;
    struct hostent* rem;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portNum);

    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");

    if (listen(sock, 15) < 0)
        perror_exit("listen");

    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    // Initialize client list //

    clientList* client_list = clientListInit();

    printf("Listening for connections to port %d.\n", portNum);

    while(1){

        // block until input arrives on one or more active sockets.
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0&& errno != EINTR)
            perror_exit("select");

        if(terminate)
            break;

        // Service all the sockets with input pending.
        for (int i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET (i, &read_fd_set)){
                if (i == sock){
                    // Connection request on original socket.
                    if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
                        perror_exit("accept");
                    FD_SET (newsock, &active_fd_set);
                }
                else{
                    // Data arriving on an already-connected socket.
                    if (service_client(i, client_list) == 0){
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
        }
    }
    // loop was interrupted
    close(sock);

    printf("Server is exiting.\n");
    clientListDelete(client_list);
    return 0;
}

int service_client(int fileDesc, clientList* client_list){
    // function used to service requests made by clients

    char buffer[15];
    char character;
    int nbytes;
    int index = 0;

    do{
        if(read(fileDesc, &character, 1) < 0)
            perror_exit("read");
        buffer[index] = character;
        index++;
    }while(character != '\0');

    printf("Received request ");

    if(strcmp(buffer, "LOG_ON") == 0){
        // new client logged on -> add to client list, notify other clients

        uint32_t clientIP;
        uint16_t clientPort;

        if(read(fileDesc, &clientIP, 4) < 0)
            perror_exit("read");

        if(read(fileDesc, &clientPort, 2) < 0)
            perror_exit("read");

        clientIP = ntohl(clientIP);
        clientPort = ntohs(clientPort);

        printf("%s %u %u\n", buffer, clientIP, clientPort);

        clientPortIP* temp = malloc(sizeof(clientPortIP));
        clientInit(temp, clientIP, clientPort);

        int newsock;
        clientNode* curr = client_list->head;
        
        struct sockaddr_in listClient;
        struct sockaddr* listClientptr = (struct sockaddr*) &listClient;
        struct in_addr sin_addr = {};
        listClient.sin_addr = sin_addr;
        
        clientIP = htonl(clientIP);
        clientPort = htons(clientPort);

        while(curr != NULL){
            
            if((newsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	        perror_exit("socket");

            listClient.sin_family = AF_INET;
            listClient.sin_addr.s_addr = htonl(curr->client->IP);
            listClient.sin_port = htons(curr->client->port);

            if (connect(newsock, listClientptr, sizeof(listClient)) < 0)
                perror_exit("connect");

            if(write(newsock, "USER_ON", 8) < 0)
                perror_exit("write");

            if(write(newsock, &clientIP, 4) < 0)
                perror_exit("write");
            
            if(write(newsock, &clientPort, 2) < 0)
                perror_exit("write");

        curr = curr->next;
        }
                
        clientListInsert(client_list, temp);
        printf("Client inserted in list.\n");
        
        clientListPrint(client_list);
    }
    else if(strcmp(buffer, "LOG_OFF") == 0){
        // client logged off -> delete from client list, notify other clients

        uint32_t clientIP;
        uint16_t clientPort;

        if(read(fileDesc, &clientIP, 4) < 0)
            perror_exit("read");

        if(read(fileDesc, &clientPort, 2) < 0)
            perror_exit("read");

        clientIP = ntohl(clientIP);
        clientPort = ntohs(clientPort);

        printf("%s %u %u\n", buffer, clientIP, clientPort);

        clientPortIP temp;
        clientInit(&temp, clientIP, clientPort);

        int newsock;
        
        struct sockaddr_in listClient;
        struct sockaddr* listClientptr = (struct sockaddr*) &listClient;
        struct in_addr sin_addr = {};
        listClient.sin_addr = sin_addr;

        printf("Client deleted from list.\n");

        if(!clientListDeleteEntry(client_list, &temp))
            printf("ERROR_IP_PORT_NOT_FOUND_IN_LIST\n");

        clientNode* curr = client_list->head;

        clientIP = htonl(clientIP);
        clientPort = htons(clientPort);

        while(curr != NULL){

            if((newsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                perror_exit("socket");

            listClient.sin_family = AF_INET;
            listClient.sin_addr.s_addr = htonl(curr->client->IP);
            listClient.sin_port = htons(curr->client->port);

            if (connect(newsock, listClientptr, sizeof(listClient)) < 0)
                perror_exit("connect");

            if(write(newsock, "USER_OFF", 9) < 0)
                perror_exit("write");

            if(write(newsock, &clientIP, 4) < 0)
                perror_exit("write");
            
            if(write(newsock, &clientPort, 2) < 0)
                perror_exit("write");

        curr = curr->next;
        }
        clientListPrint(client_list);
    }
    else if(strcmp(buffer, "GET_CLIENTS") == 0){
        // request for client list
        
        printf("%s\n", buffer);

        int number = client_list->count;
        char numberString[11];
        sprintf(numberString, "%d", number);

        if(write(fileDesc, "CLIENT_LIST", 12) < 0)
            perror_exit("write");        

        if(write(fileDesc, numberString, strlen(numberString) + 1) < 0)
            perror_exit("write");

        clientNode* curr = client_list->head;
        uint32_t clientIP;
        uint16_t clientPort;

        while(curr != NULL){

            clientIP = htonl(curr->client->IP);
            clientPort = htons(curr->client->port);

            if(write(fileDesc, &clientIP, 4) < 0)
                perror_exit("write");

            if(write(fileDesc, &clientPort, 2) < 0)
                perror_exit("write");

            curr = curr->next;
        }
    }
    else{
        printf("Error! Wrong Input");
    }

    return 0;
}