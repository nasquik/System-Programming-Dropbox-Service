#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <linux/limits.h>
#include "clientFunctions.h"

long getFileCreationTime(const char* path){
    // get last modification time for a file
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtime;
}

int sendFileList(char* dirName, int fileDesc){
    // send <pathname, version> for every file in this directory

    DIR *dir;
    struct dirent *entry;
    struct stat st;
    uint32_t size;
     
    if (!(dir = opendir(dirName)))
        return -1;

    char currDir[PATH_MAX];

    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_DIR){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            sprintf(currDir, "%s/%s", dirName, entry->d_name);
            if(sendFileList(currDir, fileDesc) < 0)
                return -1;
        }
        else{

            char fileName[PATH_MAX];
            sprintf(fileName, "%s/%s", dirName, entry->d_name);

            char* removeDirName;
            removeDirName = strchr(fileName, '/');
            removeDirName = removeDirName + 1;
            removeDirName = strchr(removeDirName, '/');
            removeDirName = removeDirName + 1;

            if(write(fileDesc, removeDirName, strlen(removeDirName) + 1) < 0){
                perror("write");
                return -1;
            }

            long version = getFileCreationTime(fileName);

            char versionString[21];
            sprintf(versionString, "%ld", version);

            if(write(fileDesc, versionString, strlen(versionString) + 1) < 0){
                perror("write");
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

int countFiles(char *dirName, int* count){
    // count files in this directory

    DIR *dir;
    struct dirent *entry;
    char currDir[PATH_MAX];

    if (!(dir = opendir(dirName)))
        return -1;

    while((entry = readdir(dir)) != NULL){

        if(entry->d_type == DT_DIR){

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            sprintf(currDir, "%s/%s", dirName, entry->d_name);
            countFiles(currDir, count);
        }
        else
            *count += 1;
    }

    closedir(dir);
    return 1;
}

int sendFile(char* dirName, char* pathname, long version, int fileDesc){
    // send update for a file to client

    struct stat st;
    int size;

    char path[PATH_MAX];
    sprintf(path, "%s/%s", dirName, pathname);

    if(access(path, F_OK) != -1){
        //path is valid

        long currentVersion = getFileCreationTime(path);
        
        if(currentVersion <= version){
            // file hasn't been updated
            if(write(fileDesc, "FILE_UP_TO_DATE", 16) < 0){
                perror("write");
                return -1;
            }
            return 0;
        }
        else{
            // send the updated file to client
            if(write(fileDesc, "FILE_SIZE", 10) < 0){
                perror("write");
                return -1;
            }

            char versionString[21];
            sprintf(versionString, "%ld", currentVersion);

            if(write(fileDesc, versionString, strlen(versionString) + 1) < 0){
                perror("write");
                return -1;
            }

            if(stat(path, &st) == 0)
                size = st.st_size;

            char sizeBuffer[11];
            sprintf(sizeBuffer, "%d", size);

            if(write(fileDesc, sizeBuffer, strlen(sizeBuffer) + 1) < 0){
                perror("write");
                return -1;
            }

            if(copyFileSender(path, fileDesc, 200) == 0){
                return 1;
            }
            else{
                return -1;
            }
        }
    }
    else{
        // path not valid
        if(write(fileDesc, "FILE_NOT_FOUND", 15) < 0){
            perror("write");
            return -1;
        }
        return 0;
    }
    
}

int copyFileSender(char* name1, int outfile, int bufferSize){
    // sending end of a file exchange

    int infile;
    ssize_t nread;
    char buffer [bufferSize];

    if((infile = open(name1, O_RDONLY)) == -1)
        return -1;

    while((nread = read(infile , buffer, bufferSize)) > 0){
        if (write(outfile , buffer, nread) < nread){
            close(infile);
            return -3;
        }
    }

    close(infile);

    if(nread == -1){
        perror("nread");
        return -4;
    }
    else
        return 0;
}

int copyFileReceiver(int infile, char* name2, int bufferSize, unsigned int size){
    // receiving end of file exchange

    int outfile;
    ssize_t nread;
    char buffer[bufferSize];
    int counter = 0;

    if ((outfile = open(name2, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
        perror("outfile");
        return -1;
    }

    int times = size / bufferSize;
    int left = size % bufferSize;

    for(int i = 0; i < times; i++){
        nread = read(infile, buffer, bufferSize);

        counter += nread;

        if(write(outfile, buffer, nread) < nread){
            close (outfile);
            return -2;
        }
    }

    nread = read(infile, buffer, left);
    counter += nread;
    
    if(write(outfile, buffer, nread) < nread){
        close (outfile);
        return -3;
    }

    if(counter != size){
        close(outfile);
        return -4;
    }

    close (outfile);

    if(nread == -1)
        return -5;
    else
        return 0;
}