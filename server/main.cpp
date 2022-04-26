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

//fd_set userSet;
//int maxDescriptor;

sem_t usersSemaphor, roomsSemaphor;
//sem_t maxDescriptorSemaphor;

//Utility functions
void clearDescriptor(int descriptor) {
    static int devNull = open("/dev/null", O_WRONLY);

    int splicedBytes = splice(descriptor, NULL, devNull, NULL, INT_MAX, SPLICE_F_MOVE);
    if(splicedBytes < 0)
        printf("Error on splicing to /dev/null.\n");
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

        memcpy(newUser->name, displayName, displayNameSize);

        return userCounter++;
    }

    else
        return users[descriptor]->id;
}

void *userOperationsHandler(void *params) {
    int updatedDescriptors, bytesRead;
    char functionNumber;
    struct epoll_event events[MAX_USERS];

    while(1) {
        printf("Epoll waiting...\n");
        updatedDescriptors = epoll_wait(epollDescriptor, events, MAX_USERS, -1);
        
        for(int c = 0; c < updatedDescriptors; c++) {
            bytesRead = read(events[c].data.fd, &functionNumber, 1);
            if(bytesRead != 1) {
                printf("Error occurred while reading target function information.\n");
                clearDescriptor(events[c].data.fd);
                continue;
            }

            switch(functionNumber) {
                case 0: {
                    int usernameLength = 0;

                    bytesRead = read(events[c].data.fd, &usernameLength, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading username length.\n");
                        clearDescriptor(events[c].data.fd);
                    }

                    char *username = (char*) malloc(usernameLength + 1);

                    bytesRead = read(events[c].data.fd, username, (int) usernameLength);
                    if(bytesRead != usernameLength) {
                        printf("Error occurred while reading username.\n");
                        clearDescriptor(events[c].data.fd);
                    }

                    username[usernameLength] = '\0';
                    int id = loginUser(events[c].data.fd, username, usernameLength + 1);

                    write(events[c].data.fd, &id, 4);

                    free(username);
                    break;
                }

                case 1:
                    // Logout
                    break;

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
    //maxDescriptor = 0;
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

    //FD_ZERO(userSet);

    sem_init(&usersSemaphor, 0, 1);
    sem_init(&roomsSemaphor, 0, 1);
    //sem_init(&maxDescriptorSemaphor, 0, 1);

    pthread_create(&userOperationsThread, NULL, &userOperationsHandler, NULL);
    
    while(1) {
        printf("Accepting connections...\n");
        if((clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddress, (unsigned int*) &addressSize)) < 0) {
            printf("Error accepting client connection.\n");
            printf("Client info:\n\tClient IP: %s\n\tClient Port: %d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            continue;
        }

        if(connectionsCounter < MAX_USERS) {
            //sem_wait(&maxDescriptorSemaphor);

            //if(maxDescriptor < clientDescriptor + 1)
            //    maxDescriptor = clientDescriptor + 1;

            //sem_post(&maxDescriptorSemaphor);

            event = (struct epoll_event*) malloc(sizeof(epoll_event));
            event->events = EPOLLIN;
            event->data.fd = clientDescriptor;

            epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, clientDescriptor, event);
            //FD_SET(clientDescriptor, &userSet);
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