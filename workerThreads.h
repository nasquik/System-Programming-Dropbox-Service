#ifndef WORKERTHREADS_H
#define WORKERTHREADS_H

typedef struct sharedInfo{
    struct clientList* client_list;
    struct circularBuffer* cBuffer;
    pthread_mutex_t listLock;
}sharedInfo;

void perror_exit(char*);
sharedInfo* sharedInfoInit(struct clientList*, struct circularBuffer*);
void sharedInfoDelete(sharedInfo*);
void workerThreadsFunction(sharedInfo*);

#endif