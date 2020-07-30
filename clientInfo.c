#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "clientInfo.h"

clientInfo* clientInfoInit(int* argc, char** argv){

    clientInfo* clInfo = malloc(sizeof(clientInfo));
    
    for(int i = 1; i <= *argc - 1; i += 2){

        if(strcmp(argv[i], "-d") == 0)
            clInfo->dirName = argv[i+1];
        else if(strcmp(argv[i], "-p") == 0)
            clInfo->portNum = atoi(argv[i+1]);
        else if(strcmp(argv[i], "-w") == 0)
            clInfo->workerThreads = atoi(argv[i+1]);
        else if(strcmp(argv[i],"-b") == 0)
            clInfo->bufferSize = atoi(argv[i+1]);
        else if(strcmp(argv[i], "-sp") == 0)
            clInfo->serverPort = atoi(argv[i+1]);
        else if(strcmp(argv[i], "-sip") == 0)
            clInfo->serverIP = argv[i+1];
        else{
            printf("Incorrect Input!\n");
            free(clInfo);
            return NULL;
        }
    }

    return clInfo;
}