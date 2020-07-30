#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "circularBuffer.h"

void bufNodeInitFileRequest(bufNode* newNode, uint32_t IP, uint16_t port, char* pathname, long version){
    
    newNode->IP = IP;
    newNode->port = port;
    newNode->version = version;
    strcpy(newNode->pathname, pathname);
}

void bufNodeInitNewClient(bufNode* newNode, uint32_t IP, uint16_t port){

    newNode->IP = IP;
    newNode->port = port;
    newNode->pathname[0] = '\0';
    newNode->version = -1;
}

circularBuffer* circularBufferInit(int buffersize){

    circularBuffer* cBuffer = malloc(sizeof(circularBuffer));
    cBuffer->buffer = malloc(buffersize * sizeof(bufNode));

    for(int i = 0; i < buffersize; i++){
        bufNode* temp = &cBuffer->buffer[i];
        temp->IP = 0;
        temp->port = 0;
        temp->version = -1;
        temp->pathname[0] = '\0';
    }
    pthread_mutex_init(&cBuffer->bufferLock, 0);
    pthread_cond_init(&cBuffer->nonFull, 0);
    pthread_cond_init(&cBuffer->nonEmpty, 0);
    cBuffer->bufin = 0;
    cBuffer->bufout = 0;
    cBuffer->count = 0;
    cBuffer->bufferSize = buffersize;

    return cBuffer;
}

int circularBufferGetItem(circularBuffer* cBuffer, bufNode* item){
    
    int error;
    int erroritem = 0;

    if((error = pthread_mutex_lock(&cBuffer->bufferLock)))
        return error;

    while(cBuffer->count <= 0) {
        //printf("The Buffer is Empty.\n");
        pthread_cond_wait(&cBuffer->nonEmpty, &cBuffer->bufferLock);
    }

    *item = cBuffer->buffer[cBuffer->bufout];
    cBuffer->bufout = (cBuffer->bufout + 1) % cBuffer->bufferSize;
    cBuffer->count -= 1;
    
    if((error = pthread_mutex_unlock(&cBuffer->bufferLock)))
        return error;
    
    return erroritem;
}

int circularBufferInsertItem(circularBuffer* cBuffer, bufNode item) {

    int error;
    int erroritem = 0;

    if((error = pthread_mutex_lock(&cBuffer->bufferLock)))
        return error;

    while(cBuffer->count >= cBuffer->bufferSize) {
        //printf("The Buffer is Full.\n");
        pthread_cond_wait(&cBuffer->nonFull, &cBuffer->bufferLock);
    }
    
    cBuffer->buffer[cBuffer->bufin] = item;
    cBuffer->bufin = (cBuffer->bufin + 1) % cBuffer->bufferSize;
    cBuffer->count += 1;
    
    if((error = pthread_mutex_unlock(&cBuffer->bufferLock)))
        return error;
    
    return erroritem;
}

void circularBufferDelete(circularBuffer* cBuffer){
    
    free(cBuffer->buffer);
    pthread_cond_destroy(&cBuffer->nonEmpty);
    pthread_cond_destroy(&cBuffer->nonFull);
    pthread_mutex_destroy(&cBuffer->bufferLock);
    free(cBuffer);
}