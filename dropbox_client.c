#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <linux/limits.h>
#include "clientInfo.h"
#include "clientList.h"
#include "networkFunctions.h"
#include "circularBuffer.h"
#include "workerThreads.h"
#include "clientFunctions.h"

int handle_request(int fileDesc, clientInfo*, sharedInfo*);

int terminate = 0;

void terminationHandler(int signum){
    terminate = 1;
}

int main(int argc, char* argv[]){

    // Get arguments

    struct clientInfo* clInfo = NULL;

    if(argc < 13 || argc > 13){
        printf("Incorrect Input!\n");
        return -1;
    }
    else{
        clInfo = clientInfoInit(&argc, argv);
        if(clInfo == NULL)
            return -1;
    }

    // Check if <dirName> directory exists //
    
    DIR* dir;
    
    dir = opendir(clInfo->dirName);

    if(dir){
        closedir(dir);
    }
    else if (ENOENT == errno){
        perror("Error!");
    }
    else{
        perror("Error!");
    }

    // Set up Signal Handler for SIGINT/SIGQUIT signals //

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

    // Get client's IP address //

    char hostbuffer[256]; 
    char* IPbuffer;
    struct hostent* myClient;
    int hostname;
  
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    myClient = gethostbyname(hostbuffer); 
    IPbuffer = inet_ntoa(*((struct in_addr*)myClient->h_addr));

    //printf("%s\n", IPbuffer);

    struct in_addr myIP;

    if(inet_aton(IPbuffer, &myIP) == 0){
        printf("Error! Invalid client IP address!\n");
        free(clInfo);
        return -1;
    }

    // Create threads and initialize shared info between threads //
    
    clientList* client_list = clientListInit();
    circularBuffer* cBuffer = circularBufferInit(clInfo->bufferSize);
    sharedInfo* shInfo = sharedInfoInit(client_list, cBuffer);

    pthread_t threadsArray[clInfo->workerThreads];

    for(int i = 0; i < clInfo->workerThreads; i++){
        pthread_create(&threadsArray[i], NULL, (void*) workerThreadsFunction, shInfo);
    }

    // Bind a socket for communucation with other clients or the server //
    int select_sock;
    fd_set active_fd_set, read_fd_set;

    struct sockaddr_in thisClient, newClient;
    socklen_t clientlen = sizeof(newClient);

    struct sockaddr* newClientptr = (struct sockaddr*) &newClient;
    struct sockaddr* thisClientptr = (struct sockaddr*) &thisClient;

    if ((select_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    thisClient.sin_family = AF_INET;
    thisClient.sin_addr.s_addr = htonl(INADDR_ANY);
    thisClient.sin_port = htons(clInfo->portNum);

    if (bind(select_sock, thisClientptr, sizeof(thisClient)) < 0)
        perror_exit("bind");

    if (listen(select_sock, 15) < 0)
        perror_exit("listen");

    // Initialize the set of active sockets
    FD_ZERO(&active_fd_set);
    FD_SET(select_sock, &active_fd_set);

    // Create a mirror directory to save received data into //
    
    char mirrorPath[PATH_MAX];
    sprintf(mirrorPath, "./%s_mirror", myClient->h_name);
    dir = opendir(mirrorPath);

    if(dir){
        printf("Error! Mirror Directory already exists!\n");
        closedir(dir);
        return -1;
    }
    else{
        if(mkdir(mirrorPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0){
            printf("Created mirror directory %s.\n", mirrorPath);
        }
    }

    // LOG_ON to server //

    int server_sock;

    struct sockaddr_in server;
    struct sockaddr* serverptr = (struct sockaddr*) &server;

    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");

    server.sin_family = AF_INET;
    if(inet_aton(clInfo->serverIP, &server.sin_addr) == 0){
        printf("Invalid server IP address!\n");
        free(clInfo);
        return -1;
    }
    server.sin_port = htons(clInfo->serverPort);

    if (connect(server_sock, serverptr, sizeof(server)) < 0)
	    perror_exit("connect");

    uint32_t IP = myIP.s_addr;
    uint16_t port = htons(clInfo->portNum);

    if(write(server_sock, "LOG_ON", 7) < 0)
        perror_exit("write");

    if(write(server_sock, &IP, 4) < 0)
        perror_exit("write");
    
    if(write(server_sock, &port, 2) < 0)
        perror_exit("write");

    close(server_sock);

    printf("Client is logged on to the server.\n");

    // GET_CLIENTS from server //

    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");

    server.sin_family = AF_INET;
    if(inet_aton(clInfo->serverIP, &server.sin_addr) == 0){
        printf("Invalid server IP address!\n");
        free(clInfo);
        return -1;
    }
    server.sin_port = htons(clInfo->serverPort);

    if (connect(server_sock, serverptr, sizeof(server)) < 0)
	    perror_exit("connect");

    if(write(server_sock, "GET_CLIENTS", 12) < 0)
        perror_exit("write");

    int number;
    uint32_t clientIP;
    uint16_t clientPort;
    clientPortIP* client;
    IP = ntohl(IP);

    // receive client list

    char buffer[11];
    char character;
    int index = 0;
    
    do{
        if(read(server_sock, &character, 1) < 0)
            perror_exit("read");
        buffer[index] = character;
        index++;
    }while(character != '\0');

    if(strcmp(buffer, "CLIENT_LIST") != 0){
        printf("Unexpected read: %s\n", buffer);
        return -1;
    }

    char numberString[11];
    index = 0;
    
    do{
        if(read(server_sock, &character, 1) < 0)
            perror_exit("read");
        numberString[index] = character;
        index++;
    }while(character != '\0');

    number = atoi(numberString);

    printf("Received protocol %s %u\n", buffer, number);

    for(int i = 0; i < number; i++){
        // Insert every <IP, Port> item read into client list and circular buffer
        
        if(read(server_sock, &clientIP, 4) < 0)
            perror_exit("read");

        if(read(server_sock, &clientPort, 2) < 0)
            perror_exit("read");

        clientIP = ntohl(clientIP);
        clientPort = ntohs(clientPort);

        if(clientIP != IP || clientPort != clInfo->portNum){
            bufNode newNode;
            client = malloc(sizeof(clientPortIP));
            clientInit(client, clientIP, clientPort);
            bufNodeInitNewClient(&newNode, clientIP, clientPort);
            clientListInsert(client_list, client);
            circularBufferInsertItem(cBuffer, newNode);
            pthread_cond_signal(&shInfo->cBuffer->nonEmpty);
        }
    }

    printf("Client list read.\n");
    clientListPrint(client_list);

    close(server_sock);

    //////////// OPEN SOCKET FOR COMMUNICATION ////////////

    int newsock;

    printf("Listening for connections to port %d.\n", clInfo->portNum);

    while(1){

        // Block until input arrives on one or more active sockets.
        read_fd_set = active_fd_set;

        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0 && errno != EINTR){
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(terminate)
            break;

        // Service all the sockets with input pending.
        for (int i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET (i, &read_fd_set)){
                if (i == select_sock){
                    // Connection request on original socket
                    if ((newsock = accept(select_sock, newClientptr, &clientlen)) < 0)
                        perror_exit("accept");
                    FD_SET (newsock, &active_fd_set);
                }
                else{
                    // Data arriving on an already-connected socket.
                    if (handle_request(i, clInfo, shInfo) == 0){
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
        }
    }
    // loop interrupted
    close(select_sock);

    // LOG_OFF of server //

    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
    
    server.sin_family = AF_INET;
    if(inet_aton(clInfo->serverIP, &server.sin_addr) == 0){
        printf("Invalid server IP address!\n");
        free(clInfo);
        return -1;
    }
    server.sin_port = htons(clInfo->serverPort);

    if (connect(server_sock, serverptr, sizeof(server)) < 0)
	    perror_exit("connect");

    IP = myIP.s_addr;
    port = htons(clInfo->portNum);

    if(write(server_sock, "LOG_OFF", 8) < 0)
        perror_exit("write");

    if(write(server_sock, &IP, 4) < 0)
        perror_exit("write");
    
    if(write(server_sock, &port, 2) < 0)
        perror_exit("write");

    close(server_sock);
    printf("Logged off of Server.\n");
    printf("Exiting...\n");

    // free memory, cancel threads
    for(int i = 0; i < clInfo->workerThreads; i++){
        if (pthread_cancel(threadsArray[i]) != 0) {                                              
            perror_exit("cancel thread");                                                         
        }
        printf("Cancelled threads.\n");           
    }
    for(int i = 0; i < clInfo->workerThreads; i++){
        if (pthread_join(threadsArray[i], NULL) != 0) {                                          
            perror_exit("thread join");                                                                  
        }
        printf("All threads ended.\n");           
    }     
    circularBufferDelete(cBuffer);
    clientListDelete(client_list);
    sharedInfoDelete(shInfo);
    free(clInfo);
    return 0;
}

int handle_request(int fileDesc, clientInfo* clInfo, sharedInfo* shInfo){
    // function used to handle protocols sent by the server or other clients
    
    char buffer[15];
    char character;
    int index = 0;

    do{
        if(read(fileDesc, &character, 1) < 0)
            perror_exit("read");
        buffer[index] = character;
        index++;
    }while(character != '\0');

    printf("Received protocol ");

    if(strcmp(buffer, "USER_ON") == 0){
        // another client connected to the server -> add to client list and circular buffer

        uint32_t clientIP;
        uint16_t clientPort;

        if(read(fileDesc, &clientIP, 4) < 0)
            perror_exit("read");

        if(read(fileDesc, &clientPort, 2) < 0)
            perror_exit("read");

        clientIP = ntohl(clientIP);
        clientPort = ntohs(clientPort);

        clientPortIP* temp = malloc(sizeof(clientPortIP));
        bufNode newNode;
        clientInit(temp, clientIP, clientPort);
        bufNodeInitNewClient(&newNode, clientIP, clientPort);

        printf("%s %u %u\n", buffer, temp->IP, temp->port);
        
        pthread_mutex_lock(&shInfo->listLock);
        clientListInsert(shInfo->client_list, temp);
        clientListPrint(shInfo->client_list);
        pthread_mutex_unlock(&shInfo->listLock);
        circularBufferInsertItem(shInfo->cBuffer, newNode);
        pthread_cond_signal(&shInfo->cBuffer->nonEmpty);
    }
    else if(strcmp(buffer, "USER_OFF") == 0){
        // client disconnected from server, delete from client list

        uint32_t clientIP;
        uint16_t clientPort;

        if(read(fileDesc, &clientIP, 4) < 0)
            perror_exit("read");

        if(read(fileDesc, &clientPort, 2) < 0)
            perror_exit("read");

        clientIP = ntohl(clientIP);
        clientPort = ntohs(clientPort);

        clientPortIP temp;
        clientInit(&temp, clientIP, clientPort);
        printf("%s %u %u\n", buffer, temp.IP, temp.port);
        
        pthread_mutex_lock(&shInfo->listLock);
        clientListDeleteEntry(shInfo->client_list, &temp);
        clientListPrint(shInfo->client_list);
        pthread_mutex_unlock(&shInfo->listLock);
    }
    else if(strcmp(buffer, "GET_FILE_LIST") == 0){
        // request to send pathname and version for all files

        printf("%s\n", buffer);
                
        // count files and send number
        int count = 0;
        countFiles(clInfo->dirName, &count);

        char countString[11];
        sprintf(countString, "%d", count);

        if(write(fileDesc, "FILE_LIST", 10) < 0)
            perror_exit("write");

        if(write(fileDesc, countString, strlen(countString) + 1) < 0)
            perror_exit("write");

        // send list
        sendFileList(clInfo->dirName, fileDesc);

    }
    else if(strcmp(buffer, "GET_FILE") == 0){
        // request to send update on a specific file
        
        char pathname[128];
        char versionString[21];
        character = '\0';
        index = 0;

        do{
            if(read(fileDesc, &character, 1) < 0)
                perror_exit("read");
            pathname[index] = character;
            index++;
        }
        while(character != '\0');

        index = 0;

        do{
            if(read(fileDesc, &character, 1) < 0)
                perror_exit("read");
            versionString[index] = character;
            index++;
        }
        while(character != '\0');

        long version = atol(versionString);

        printf("%s %s %ld\n", buffer, pathname, version);

        sendFile(clInfo->dirName, pathname, version, fileDesc);
    }
    else{
        printf("Unexpected read: %s\n", buffer);
        return -1;
    }

    return 0;
}