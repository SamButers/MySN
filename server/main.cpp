#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>

#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>

#include <map>

#define MAX_USERS   256
#define MAX_ROOMS   256
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define FLUSH_BUFFER_SIZE 64000

typedef struct sockaddr_in sockaddr_in;

typedef struct User {
    int id, pictureId, descriptor;
    char* name;
    char isInRoom;
} User;

typedef struct Room {
    int id, userLimit;
    char* name;
    User **users;
} Room;

typedef struct Message {
    int userId;
    char *content;
} Message;

std::map<int, User*> users;
std::map<int, Room*> rooms;
int userCounter;

int epollDescriptor;

sem_t usersSemaphor, roomsSemaphor;

char flushBuffer[FLUSH_BUFFER_SIZE];

//Utility functions
void clearDescriptor(int descriptor) {
    int readBytes = read(descriptor, &flushBuffer, FLUSH_BUFFER_SIZE);

    if(readBytes < 0)
        printf("Error on flushing.\n");
    else {
        printf("Flushed bytes: %d\n", readBytes);
        getchar();
    }
}

int sendErrorResponse(int descriptor, char functionId, char *buffer) {
    int writtenBytes;

    int integerResponse;
    char byteResponse;

    buffer[0] = 0;
    buffer[1] = functionId;

    switch(functionId) {
        case -1:
            return write(descriptor, buffer, 2) != 2;

        case 0:
            integerResponse = -1;
            memcpy(buffer + 2, &integerResponse, 4);

            return write(descriptor, buffer, 6) != 6;

        default:
            return 1;
    }
}

// User operation functions
int loginUser(int descriptor, char *displayName, int displayNameSize) {
    if(users.find(descriptor) == users.end()) {
        User *newUser;

        users[descriptor] = new User();
        newUser = users[descriptor];

        newUser->id = userCounter;
        newUser->pictureId = 0;
        newUser->descriptor = descriptor;
        newUser->isInRoom = 0;
        newUser->name = (char*) malloc(displayNameSize);

        printf("Copying username...\n");
        //getchar();
        
        memcpy(newUser->name, displayName, displayNameSize);

        //getchar();

        return userCounter++;
    }

    else
        return users[descriptor]->id;
}

void logoutUser(int descriptor) {
    epoll_ctl(epollDescriptor, EPOLL_CTL_DEL, descriptor, NULL);

    User *user = users[descriptor];
    users.erase(descriptor);

    free(user->name);
    delete user;
}

void *userOperationsHandler(void *params) {
    int updatedDescriptors, bytesRead;
    char functionNumber;
    char responseBuffer[18];
    struct epoll_event events[MAX_USERS];

    while(1) {
        printf("Epoll waiting...\n");
        updatedDescriptors = epoll_wait(epollDescriptor, events, MAX_USERS, -1);
        printf("Descriptors updated.\n");
        
        for(int c = 0; c < updatedDescriptors; c++) {
            bytesRead = read(events[c].data.fd, &functionNumber, 1);
            if(bytesRead != 1) {
                if(bytesRead == 0) {
                    printf("User sudden disconnection.\n");
                    logoutUser(events[c].data.fd);
                    continue;
                }

                printf("Error occurred while reading target function information.\n");
                
                clearDescriptor(events[c].data.fd);
                sendErrorResponse(events[c].data.fd, -1, responseBuffer);

                continue;
            }

            switch(functionNumber) {
                case 0: {
                    printf("Login\n");
                    int usernameLength = 0;

                    bytesRead = read(events[c].data.fd, &usernameLength, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading username length.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                    }

                    char *username = (char*) malloc(usernameLength + 1);

                    bytesRead = read(events[c].data.fd, username, (int) usernameLength);
                    if(bytesRead != usernameLength) {
                        printf("Error occurred while reading username.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                    }

                    username[usernameLength] = '\0';
                    int id = loginUser(events[c].data.fd, username, usernameLength + 1);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 0;
                    memcpy(responseBuffer + 2, &id, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    printf("ID: %d\nUsername: %s\n", id, username);

                    free(username);
                    break;
                }

                case 1: {
                    // Logout
                    printf("Logout\n");

                    //responseBuffer[0] = 0;
                    //responseBuffer[1] = 0;
                    //responseBuffer[2] = 0;

                    //write(events[c].data.fd, responseBuffer, 3);

                    logoutUser(events[c].data.fd);

                    break;
                }

                case 2:
                    // Create room
                    break;

                case 3:
                    // Join room
                    break;

                case 4:
                    // Send message
                    break;

                case 5:
                    // Change info
                    break;

                default:
                    printf("Invalid function called: %d\n", (int) functionNumber);
            }
        }

        //updateClients(updates);
    }

    printf("Broke from infinite while?\n");
}

int main() {
    sockaddr_in serverAddress;
    sockaddr_in clientAddress;

    int serverDescriptor, clientDescriptor;
    int addressSize;
    int connectionsCounter;

    pthread_t userOperationsThread, chatroomThreads[MAX_ROOMS];

    struct epoll_event *event;

    addressSize = sizeof(clientAddress);
    userCounter = 0;
    connectionsCounter = 0;

    memset((char*) &serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons(SERVER_PORT);

    if((serverDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error during socket creation.\n");
        return 1;
    }

    if(bind(serverDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        printf("Error during binding.\n");
        return 1;
    }

    if(listen(serverDescriptor, MAX_USERS) < 0) {
        printf("Error during port listening.\n");
        return 1;
    }

    if((epollDescriptor = epoll_create1(0)) < 0) {
        printf("Error during epoll descriptor creation.\n");
        return 1;
    }

    sem_init(&usersSemaphor, 0, 1);
    sem_init(&roomsSemaphor, 0, 1);

    pthread_create(&userOperationsThread, NULL, &userOperationsHandler, NULL);
    
    while(1) {
        printf("Accepting connections...\n");
        if((clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddress, (unsigned int*) &addressSize)) < 0) {
            printf("Error accepting client connection.\n");
            printf("Client info:\n\tClient IP: %s\n\tClient Port: %d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            continue;
        }

        if(connectionsCounter < MAX_USERS) {
            event = (struct epoll_event*) malloc(sizeof(epoll_event));
            event->events = EPOLLIN;
            event->data.fd = clientDescriptor;

            epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, clientDescriptor, event);

            // pingNewUserDescriptor();
            connectionsCounter++;
        }

        else {
            printf("Maximum users reached. Connection refused.\n");
            close(clientDescriptor);
        }
    }

    //freeEvents(epollDescriptor);
    close(epollDescriptor);

    return 0;
}