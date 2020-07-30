#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
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
#include "circularBuffer.h"
#include "workerThreads.h"
#include "clientFunctions.h"

extern int terminate;

void cleanup_handler(void* arg){
    // Function to be called when a thread is cancelled

    circularBuffer* cBuffer = (circularBuffer*)arg;
    pthread_cond_signal(&cBuffer->nonFull);
    pthread_cond_signal(&cBuffer->nonEmpty);
    pthread_mutex_unlock(&cBuffer->bufferLock);

}

void perror_exit(char* message){
    perror(message);
    exit(EXIT_FAILURE);
}

sharedInfo* sharedInfoInit(clientList* client_list, circularBuffer* cBuffer){
    
    sharedInfo* shInfo = malloc(sizeof(sharedInfo));

    shInfo->client_list = client_list;
    shInfo->cBuffer = cBuffer;
    pthread_mutex_init(&shInfo->listLock, 0);

    return shInfo;
}

void sharedInfoDelete(sharedInfo* shInfo){
    
    shInfo->client_list = NULL;
    shInfo->cBuffer = NULL;
    pthread_mutex_destroy(&shInfo->listLock);
    free(shInfo);
}

void workerThreadsFunction(sharedInfo* shInfo){
    // main function used by worker threads

    // Initialise cleanup function that is called automatically when a thread is cancelled
    circularBuffer* cleanup_push_arg = shInfo->cBuffer;
    pthread_cleanup_push(cleanup_handler, (void*)cleanup_push_arg);

    bufNode item;
    int sock;
    struct sockaddr_in newClient;

    struct sockaddr* newClientptr = (struct sockaddr*) &newClient;
    
    while(1){
        // constantly get items from circular buffer

        circularBufferGetItem(shInfo->cBuffer, &item);
        
        if(terminate){
            printf("Thread cancelled!\n");
            break;
        }

        pthread_cond_signal(&shInfo->cBuffer->nonFull);

        printf("Popped item from buffer.\n");

        clientPortIP client;
        clientInit(&client, item.IP, item.port);
        pthread_mutex_lock(&shInfo->listLock);
        clientNode* temp = inClientList(shInfo->client_list, &client);
        pthread_mutex_unlock(&shInfo->listLock);

        if(temp == NULL){
            printf("The popped item's <IP, Port> combination is not in the Client List.\n");
            continue;
        }

        struct hostent *thisClient, *host;
        struct in_addr inAddr;
        inAddr.s_addr = htonl(item.IP);

        char hostbuffer[256]; 
        int hostname;
  
        hostname = gethostname(hostbuffer, sizeof(hostbuffer));
        thisClient = gethostbyname(hostbuffer);

        char mirrorDir[PATH_MAX];
        sprintf(mirrorDir, "./%s_mirror", thisClient->h_name);

        host = gethostbyaddr(&inAddr, sizeof(inAddr), AF_INET);

        char dirName[50];
        sprintf(dirName, "%s_%u", host->h_name, item.port);

        newClient.sin_family = AF_INET;
        newClient.sin_addr.s_addr = htonl(item.IP);
        newClient.sin_port = htons(item.port);

        if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            perror_exit("socket");

        if(connect(sock, newClientptr, sizeof(newClient)) < 0)
            perror_exit("connect");

        if(item.pathname[0] == '\0' && item.version == -1){
            // if only the IP and port fields are initialized

            printf("Popped item: <%u, %u>\n", item.IP, item.port);

            // request file list

            if(write(sock, "GET_FILE_LIST", 14) < 0)
                perror_exit("write");

            printf("Requested file list...\n");
            // receive file list

            char buffer[15];
            char character = '\0';
            int index = 0;

            do{
                if(read(sock, &character, 1) < 0)
                    perror_exit("read");
                buffer[index] = character;
                index++;
            }while(character != '\0');

            if(strcmp(buffer, "FILE_LIST") != 0){
                printf("Unexpected read: %s\n", buffer);
                close(sock);
                continue;
            }

            printf("Received protocol ");

            char numberString[11];
            int number;
            index = 0;
            
            do{
                if(read(sock, &character, 1) < 0)
                    perror_exit("read");
                numberString[index] = character;
                index++;
            }while(character != '\0');

            number = atoi(numberString);

            printf("%s %d\n", buffer, number);

            for(int i = 0; i < number; i++){
                // insert each <pathname, version> item read into circular buffer

                char pathname[128];
                char versionString[21];
                long version;
                index = 0;
                
                do{
                    if(read(sock, &character, 1) < 0)
                        perror_exit("read");
                    pathname[index] = character;
                    index++;
                }while(character != '\0');

                index = 0;
                
                do{
                    if(read(sock, &character, 1) < 0)
                        perror_exit("read");
                    versionString[index] = character;
                    index++;
                }while(character != '\0');

                version = atol(versionString);

                bufNode newNode;
                bufNodeInitFileRequest(&newNode, item.IP, item.port, pathname, version);
                circularBufferInsertItem(shInfo->cBuffer, newNode);
                pthread_cond_signal(&shInfo->cBuffer->nonEmpty);

                if(terminate){
                    close(sock);
                    break;
                }
            }
            if(terminate)
                break;
            printf("Done reading file list.\n");
        }
        else{
            // every field in the item is initialized

            printf("Popped item: <%u, %u, %s, %ld>\n", item.IP, item.port, item.pathname, item.version);

            char path[PATH_MAX];
            sprintf(path, "%s/%s/%s", mirrorDir, dirName, item.pathname);

            // check if item exists locally

            if(access(path, F_OK) != -1){
                // if it exists, send pathname and current version in order to possibly get an updated version

                printf("File exists in the mirror directory.\n");
                printf("Sending pathname and version to client...\n");

                if(write(sock, "GET_FILE", 9)< 0)
                    perror_exit("write");

                if(write(sock, item.pathname, strlen(item.pathname) + 1) < 0)
                    perror_exit("write");

                long version = getFileCreationTime(path);

                char versionString[21];
                sprintf(versionString, "%ld", version);

                if(write(sock, versionString, strlen(versionString) + 1) < 0)
                    perror_exit("write");

                // read client's reply

                char buffer[15];
                char character;
                int index = 0;
                
                do{
                    if(read(sock, &character, 1) < 0)
                        perror_exit("read");
                    buffer[index] = character;
                    index++;
                }while(character != '\0');
                
                printf("Received protocol ");

                if(strcmp(buffer, "FILE_SIZE") == 0){
                    // get updated file

                    char versionString[21];
                    index = 0;

                    do{
                        if(read(sock, &character, 1) < 0)
                            perror_exit("read");
                        versionString[index] = character;
                        index++;
                    }while(character != '\0');

                    long version = atol(versionString);

                    char sizeString[21];
                    index = 0;

                    do{
                        if(read(sock, &character, 1) < 0)
                            perror_exit("read");
                        sizeString[index] = character;
                        index++;
                    }while(character != '\0');

                    int size = atoi(sizeString);
                    printf("Size of file: %d\n", size);

                    printf("%s %s %s\n", buffer, versionString, sizeString);
                    printf("Copying file...\n");

                    if(copyFileReceiver(sock, path, 200, size) == 0){
                        printf("File copied successfully.\n");
                    }
                    else{
                        printf("File not copied successfully.\n");
                        close(sock);
                        continue;
                    }
                }
                else if(strcmp(buffer, "FILE_UP_TO_DATE") == 0){
                    printf("%s\n", buffer);
                    close(sock);
                    continue;
                }
                else if(strcmp(buffer, "FILE_NOT_FOUND") == 0){
                    printf("%s\n", buffer);
                    close(sock);
                    continue;
                }
                else{
                    printf("Unexpected read: %s\n", buffer);
                    close(sock);
                    continue;
                }
            }
            else{
                // if it doesn't exist, create a path to the file and copy it there

                printf("File doesn't exist in the mirror directory.\n");
                printf("Requesting file from client...\n");

                if(write(sock, "GET_FILE", 9)< 0)
                    perror_exit("write");

                if(write(sock, item.pathname, strlen(item.pathname) + 1) < 0)
                    perror_exit("write");

                // to ensure client sends the file
                long version = -1;

                char versionString[3];
                sprintf(versionString, "%ld", version);

                if(write(sock, versionString, strlen(versionString) + 1) < 0)
                    perror_exit("write");

                // read client's reply

                char buffer[15];
                char character;
                int index = 0;
                
                do{
                    if(read(sock, &character, 1) < 0)
                        perror_exit("read");
                    buffer[index] = character;
                    index++;
                }while(character != '\0');

                printf("Received protocol ");

                if(strcmp(buffer, "FILE_SIZE") == 0){
                    // receive file

                    char versionString[21];
                    index = 0;

                    do{
                        if(read(sock, &character, 1) < 0)
                            perror_exit("read");
                        versionString[index] = character;
                        index++;
                    }while(character != '\0');

                    long version = atol(versionString);

                    char sizeString[21];
                    index = 0;

                    do{
                        if(read(sock, &character, 1) < 0)
                            perror_exit("read");
                        sizeString[index] = character;
                        index++;
                    }while(character != '\0');

                    int size = atoi(sizeString);

                    printf("%s %s %s\n", buffer, versionString, sizeString);
                    printf("Creating path for the new file...\n");

                    char newPath[PATH_MAX];
                    //create path in mirror dir for this client if necessary
                    sprintf(newPath, "%s/%s/", mirrorDir, dirName);
                    mkdir(newPath,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    
                    char name[strlen(item.pathname)+2];
                    strcpy(name, item.pathname);

                    ///used only to differentiate between (empty) directories and files
                    strcat(name, ".");
                    
                    char* token;
                    char* rest = name;
                    token = strtok_r(rest, "/", &rest);

                    while(token != NULL){

                        if(strcmp(token, ".") != 0 && token[strlen(token) - 1] == '.'){

                            token[strlen(token) - 1] = '\0';

                            strcat(newPath, token);

                            int error;

                            if((error = copyFileReceiver(sock, newPath, 200, size)) == 0)
                                printf("File %s copied successfully.\n", item.pathname);
                            else{
                                printf("File %s not copied successfully. Error Code: %d\n", item.pathname, error);
                            }
                        }
                        else{
                            strcat(newPath, token);
                            strcat(newPath, "/");
                            mkdir(newPath,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        }
                        token = strtok_r(rest, "/", &rest);
                    }
                }
                else if(strcmp(buffer, "FILE_NOT_FOUND") == 0){
                    printf("%s\n", buffer);
                    close(sock);
                    continue;
                }
                else{
                    printf("Unexpected read: %s\n", buffer);
                    close(sock);
                    continue;
                }
            }
        }
    }
    pthread_cleanup_pop(1);
}