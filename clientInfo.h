#ifndef CLIENTINFO_H
#define CLIENTINFO_H

typedef struct clientInfo{

    char* dirName;
    uint32_t portNum;
    uint32_t  workerThreads;
    uint32_t  bufferSize;
    uint32_t  serverPort;
    char* serverIP;

}clientInfo;

clientInfo* clientInfoInit(int*, char**);

#endif