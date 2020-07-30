#ifndef CLIENTFUNCTIONS_H
#define CLIENTFUNCTIONS_H

long getFileCreationTime(const char*); 
int countFiles(char*, int*);
int sendFileList(char*, int);
int sendFile(char*, char*, long, int);
int copyFileSender(char*, int, int);
int copyFileReceiver(int, char*, int, unsigned int);


#endif