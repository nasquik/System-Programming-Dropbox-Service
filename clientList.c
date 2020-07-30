#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "clientList.h"
#include <assert.h>

void clientInit(clientPortIP* Client, uint32_t IP, uint16_t port){

    Client->IP = IP;
    Client->port = port;
}

clientList* clientListInit(){

    clientList* List = malloc(sizeof(clientList));

    List->head = NULL;
    List->tail = NULL;
    List->count = 0;

    return List;
}

clientNode* inClientList(clientList* List, clientPortIP* Client){

    clientNode* temp = List->head;

    while(temp != NULL){
        if(temp->client->IP == Client->IP && temp->client->port == Client->port)
            return temp;
        temp = temp->next;
    }

    return NULL;
}

int clientListInsert(clientList* List, clientPortIP* Client){

    clientNode* temp = inClientList(List, Client);

    if(temp == NULL){
        temp = malloc(sizeof(clientNode));
        temp->client = Client;
        temp->next = NULL;

        if(List->head == NULL && List->tail == NULL){
            List->head = temp;
            List->tail = temp;
        }
        else{
            List->tail->next = temp;
            List->tail = temp;
        }
        List->count++;
        return 1;
    }
    else
        return 0;
}

int clientListDelete(clientList* List){

    clientNode* delNode = List->head;
    clientNode* temp;

    while(delNode != NULL){
        temp = delNode->next;
        free(delNode->client);
        free(delNode);
        List->count--;
        delNode = temp;
    }

    free(List);

    return 0;
}

void clientListPrint(clientList* List){

    clientNode* curr = List->head;

    if(curr == NULL){
        printf("\nThis Client List is empty.\n");
        return;
    }

    printf("\nThis Client List contains the following clients:\n\n");

    int i = 1;

    while(curr != NULL){
        printf("%d. Client's IP: %u, Client's port: %u\n", i, curr->client->IP, curr->client->port);
        curr = curr->next;
        i++;
    }
    printf("\n");
}

int clientListDeleteEntry(clientList* List, clientPortIP* Client){

    clientNode* delNode = List->head;
    clientNode* previous = NULL;
    clientNode* current = NULL;
    clientNode* temp;

    while(delNode != NULL){
        if(delNode->client->IP == Client->IP && delNode->client->port == Client->port){

            if(delNode == List->head){
                if(delNode == List->tail){
                    List->head = NULL;
                    List->tail = NULL;
                    free(delNode->client);
                    free(delNode);
                    delNode = NULL;
                }
                else{
                    List->head = delNode->next;
                    free(delNode->client);
                    free(delNode);
                    delNode = List->head;
                }
            }
            else if(delNode == List->tail && delNode != List->head){
                previous->next = NULL;
                List->tail = previous;
                free(delNode->client);
                free(delNode);
                delNode = NULL;                
            }
            else{
                current = delNode->next;
                previous->next = current;
                free(delNode->client);
                free(delNode);
                delNode = current; 
            }

            List->count--;
            return 1;
        }
        else{
            previous = delNode;
            delNode = delNode->next;
        }
    }
    return 0;
};