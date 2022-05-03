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

#include <map>

#include "definitions.hpp"
#include "utility.hpp"
#include "update.hpp"
#include "userOperations.hpp"

using namespace std;

map<int, User*> users;
map<int, Room*> rooms;
int userCounter;

int epollDescriptor;

sem_t epollSemaphor;

char flushBuffer[FLUSH_BUFFER_SIZE];
char roomInfoBuffer[68102];
char userInfoBuffer[16897];
char roomUpdateBuffer[136];

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

                    int usernameLength, id;
                    char *username;

                    usernameLength = 0;

                    bytesRead = read(events[c].data.fd, &usernameLength, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading username length.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                        break;
                    }

                    username = (char*) malloc(usernameLength + 1);

                    bytesRead = read(events[c].data.fd, username, usernameLength);
                    if(bytesRead != usernameLength) {
                        printf("Error occurred while reading username.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                        break;
                    }

                    username[usernameLength] = '\0';
                    id = loginUser(events[c].data.fd, username, usernameLength + 1);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 0;
                    memcpy(responseBuffer + 2, &id, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    printf("ID: %d\nUsername: %s\n", id, username);

                    free(username);
                    break;
                }

                case 1: {
                    printf("Logout\n");

                    logoutUser(events[c].data.fd);

                    break;
                }

                case 2: {
                    printf("Create room\n");

                    int roomNameLength, userLimit, roomId;
                    char *roomName;

                    roomNameLength = userLimit = 0;

                    bytesRead = read(events[c].data.fd, &roomNameLength, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading room name length.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    roomName = (char*) malloc(roomNameLength + 1);

                    bytesRead = read(events[c].data.fd, roomName, roomNameLength);
                    if(bytesRead != roomNameLength) {
                        printf("Error occurred while reading room name.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    roomName[roomNameLength] = '\0';

                    bytesRead = read(events[c].data.fd, &userLimit, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading room limit.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    if(userLimit < 2) {
                        printf("Invalid room limit.\n");

                        responseBuffer[0] = 0;
                        responseBuffer[1] = 2;
                        roomId = -4;
                        memcpy(responseBuffer + 2, &roomId, 4);

                        write(events[c].data.fd, responseBuffer, 6);
                        break;
                    }

                    roomId = createRoom(roomName, roomNameLength + 1, userLimit);
                    roomId = joinRoom(events[c].data.fd, roomId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 2;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    free(roomName);

                    break;
                }

                case 3: {
                    printf("Join room.\n");
                    int roomId;

                    bytesRead = read(events[c].data.fd, &roomId, 4);
                    if(bytesRead != 4) {
                        printf("Error occurred while reading room id.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 3, responseBuffer);
                        break;
                    }

                    roomId = joinRoom(events[c].data.fd, roomId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 3;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    break;
                }

                case 4: {
                    printf("Send message.\n");

                    int messageLength = 0;
                    char status;
                    char *messageContent;

                    bytesRead = read(events[c].data.fd, &messageLength, 2);
                    if(bytesRead != 2) {
                        printf("Error occurred while reading message length.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 4, responseBuffer);
                        break;
                    }

                    messageContent = (char*) malloc(messageLength);

                    bytesRead = read(events[c].data.fd, messageContent, messageLength);
                    if(bytesRead != messageLength) {
                        printf("Error occurred while reading message content.\n");
                        printf("%d\n", bytesRead);
                        printf("%d\n", messageLength);
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 4, responseBuffer);
                        break;
                    }

                    status = sendMessage(events[c].data.fd, messageContent, messageLength);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 4;
                    memcpy(responseBuffer + 2, &status, 1);

                    write(events[c].data.fd, responseBuffer, 3);

                    free(messageContent);

                    break;
                }

                case 5: {
                    printf("Leave room\n");
                    int roomId = leaveRoom(events[c].data.fd);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 5;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    break;
                }

                case 6: {
                    printf("Change info.\n");
                    int pictureId = 0;

                    bytesRead = read(events[c].data.fd, &pictureId, 1);
                    if(bytesRead != 1) {
                        printf("Error occurred while reading picture id.\n");
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 6, responseBuffer);
                        break;
                    }

                    pictureId = changeInfo(events[c].data.fd, pictureId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 6;
                    responseBuffer[2] = (char) pictureId;

                    write(events[c].data.fd, responseBuffer, 3);

                    break;
                }

                case 7: {
                    printf("Get rooms\n");
                    int bytes = getRooms();

                    roomInfoBuffer[0] = 0;
                    roomInfoBuffer[1] = 7;

                    write(events[c].data.fd, roomInfoBuffer, bytes);

                    break;
                }

                case 8: {
                    printf("Get users\n");
                    int bytes = getUsers(events[c].data.fd);

                    userInfoBuffer[0] = 0;
                    userInfoBuffer[1] = 8;

                    write(events[c].data.fd, userInfoBuffer, bytes);

                    break;
                }

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

    pthread_t userOperationsThread;

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

    sem_init(&epollSemaphor, 0, 1);

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

            sem_wait(&epollSemaphor);

            epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, clientDescriptor, event);

            sem_post(&epollSemaphor);

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