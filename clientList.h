#ifndef CLIENTLIST_H
#define CLIENTLIST_H

typedef struct clientPortIP{
    uint32_t IP;
    uint16_t port;
}clientPortIP;

typedef struct clientList{
    struct clientNode* head;
    struct clientNode* tail;
    int count;
}clientList;


typedef struct clientNode{
    struct clientPortIP* client;
    struct clientNode* next;
}clientNode;

void clientInit(clientPortIP*, uint32_t, uint16_t);
clientList* clientListInit();
int clientListInsert(clientList*, clientPortIP*);
int clientListDelete(clientList*);
clientNode* inClientList(clientList*, clientPortIP*);
void clientListPrint(clientList*);
int clientListDeleteEntry(clientList*, clientPortIP*);

#endif