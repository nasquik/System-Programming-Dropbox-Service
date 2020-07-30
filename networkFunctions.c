#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "networkFunctions.h"

void checkHostName(int hostname){
    // Returns hostname for the local computer 
 
    if(hostname == -1) 
    { 
        perror("gethostname"); 
        exit(1); 
    } 
}

void checkHostEntry(struct hostent * hostentry){
    // Returns host information corresponding to host name 
 
    if(hostentry == NULL) 
    { 
        perror("gethostbyname"); 
        exit(1); 
    } 
} 
  
void checkIPbuffer(char *IPbuffer){
    // Converts space-delimited IPv4 addresses to dotted-decimal format

    if (NULL == IPbuffer) 
    { 
        perror("inet_ntoa"); 
        exit(1); 
    }
}

void getHostNameIP(){
    // Returns hostname for the local computer 
 
    char hostbuffer[256]; 
    char* IPbuffer;
    struct hostent* host_entry; 
    int hostname;
  
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    checkHostName(hostname); 
  
    // To retrieve host information 
    host_entry = gethostbyname(hostbuffer); 
    checkHostEntry(host_entry);
  
    // To convert an Internet network address into ASCII string 
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr));
  
    printf("Hostname: %s\n", hostbuffer); 
    printf("Host IP: %s\n", IPbuffer);
}

