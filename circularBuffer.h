#ifndef BUFFER_H
#define BUFFER_H

typedef struct bufNode{
    char pathname[128];
    long version;
    uint32_t IP;
    uint16_t port;
}bufNode;

typedef struct circularBuffer{
    bufNode* buffer;
    pthread_mutex_t bufferLock;
    pthread_cond_t nonFull;
    pthread_cond_t nonEmpty;
    int bufin;
    int bufout;
    int count;
    int bufferSize;
}circularBuffer;

void bufNodeInitFileRequest(bufNode*, uint32_t, uint16_t, char*, long);
void bufNodeInitNewClient(bufNode*, uint32_t, uint16_t);
circularBuffer* circularBufferInit(int);
int circularBufferGetItem(circularBuffer*, bufNode*);
int circularBufferInsertItem(circularBuffer*, bufNode);
void circularBufferDelete(circularBuffer*);

#endif