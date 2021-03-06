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
#include <signal.h>

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

int serverDescriptor;
pthread_t userOperationsThread;

//User operations thread handler
void *userOperationsHandler(void *params) {
    int updatedDescriptors, bytesRead;
    char functionNumber;
    char responseBuffer[18];
    struct epoll_event events[MAX_USERS];

    while(1) {
        updatedDescriptors = epoll_wait(epollDescriptor, events, MAX_USERS, -1);
        
        for(int c = 0; c < updatedDescriptors; c++) {
            bytesRead = recv(events[c].data.fd, &functionNumber, 1, MSG_DONTWAIT);

            if(bytesRead != 1) {
                if(bytesRead == 0) {
                    logoutUser(events[c].data.fd);
                    continue;
                }
                
                clearDescriptor(events[c].data.fd);
                sendErrorResponse(events[c].data.fd, -1, responseBuffer);

                continue;
            }

            switch(functionNumber) {
                // Login message parsing
                case 0: {
                    int usernameLength, id;
                    char *username;

                    usernameLength = 0;

                    bytesRead = recv(events[c].data.fd, &usernameLength, 1, MSG_DONTWAIT);
                    if(bytesRead != 1) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                        break;
                    }

                    username = (char*) malloc(usernameLength + 1);

                    bytesRead = recv(events[c].data.fd, username, usernameLength, MSG_DONTWAIT);
                    if(bytesRead != usernameLength) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 0, responseBuffer);
                        break;
                    }

                    username[usernameLength] = '\0';
                    id = loginUser(events[c].data.fd, username, usernameLength + 1);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 0;
                    memcpy(responseBuffer + 2, &id, 4);

                    send(events[c].data.fd, responseBuffer, 6, MSG_DONTWAIT);

                    free(username);
                    break;
                }


                // Logoff message parsing
                case 1: {
                    logoutUser(events[c].data.fd);

                    break;
                }

                // Create room message parsing
                case 2: {
                    int roomNameLength, roomId;
                    char userLimit;
                    char *roomName;

                    roomNameLength = userLimit = 0;

                    bytesRead = recv(events[c].data.fd, &roomNameLength, 1, MSG_DONTWAIT);
                    if(bytesRead != 1) {
                        
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    roomName = (char*) malloc(roomNameLength + 1);

                    bytesRead = recv(events[c].data.fd, roomName, roomNameLength, MSG_DONTWAIT);
                    if(bytesRead != roomNameLength) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    roomName[roomNameLength] = '\0';

                    bytesRead = recv(events[c].data.fd, &userLimit, 1, MSG_DONTWAIT);
                    if(bytesRead != 1) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 2, responseBuffer);
                        break;
                    }

                    if(userLimit < 2) {
                        responseBuffer[0] = 0;
                        responseBuffer[1] = 2;
                        roomId = -4;
                        memcpy(responseBuffer + 2, &roomId, 4);

                        send(events[c].data.fd, responseBuffer, 6, MSG_DONTWAIT);
                        break;
                    }

                    roomId = createRoom(roomName, roomNameLength + 1, userLimit);
                    roomId = joinRoom(events[c].data.fd, roomId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 2;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    send(events[c].data.fd, responseBuffer, 6, MSG_DONTWAIT);

                    free(roomName);

                    break;
                }

                // Join room message parsing
                case 3: {
                    int roomId;

                    bytesRead = recv(events[c].data.fd, &roomId, 4, MSG_DONTWAIT);
                    if(bytesRead != 4) {
                        
                        
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 3, responseBuffer);
                        break;
                    }

                    roomId = joinRoom(events[c].data.fd, roomId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 3;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    send(events[c].data.fd, responseBuffer, 6, MSG_DONTWAIT);

                    break;
                }

                // Message message parsing
                case 4: {
                    int messageLength = 0;
                    char status;
                    char *messageContent;

                    bytesRead = recv(events[c].data.fd, &messageLength, 2, MSG_DONTWAIT);
                    if(bytesRead != 2) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 4, responseBuffer);
                        break;
                    }

                    messageContent = (char*) malloc(messageLength);

                    bytesRead = recv(events[c].data.fd, messageContent, messageLength, MSG_DONTWAIT);
                    if(bytesRead != messageLength) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 4, responseBuffer);
                        break;
                    }

                    status = sendMessage(events[c].data.fd, messageContent, messageLength);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 4;
                    memcpy(responseBuffer + 2, &status, 1);

                    send(events[c].data.fd, responseBuffer, 3, MSG_DONTWAIT);

                    free(messageContent);

                    break;
                }

                // Leave room message parsing
                case 5: {
                    int roomId = leaveRoom(events[c].data.fd);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 5;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    send(events[c].data.fd, responseBuffer, 6, MSG_DONTWAIT);

                    break;
                }

                // Update info message parsing
                case 6: {
                    char pictureId = 0;

                    bytesRead = recv(events[c].data.fd, &pictureId, 1, MSG_DONTWAIT);
                    if(bytesRead != 1) {
                        clearDescriptor(events[c].data.fd);
                        sendErrorResponse(events[c].data.fd, 6, responseBuffer);
                        break;
                    }

                    pictureId = changeInfo(events[c].data.fd, pictureId);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 6;
                    responseBuffer[2] = pictureId;

                    send(events[c].data.fd, responseBuffer, 3, MSG_DONTWAIT);

                    break;
                }

                // Get rooms message parsing
                case 7: {
                    
                    int bytes = getRooms();

                    roomInfoBuffer[0] = 0;
                    roomInfoBuffer[1] = 7;

                    send(events[c].data.fd, roomInfoBuffer, bytes, MSG_DONTWAIT);

                    break;
                }

                // Get users message parsing
                case 8: {
                    int bytes = getUsers(events[c].data.fd);

                    userInfoBuffer[0] = 0;
                    userInfoBuffer[1] = 8;

                    send(events[c].data.fd, userInfoBuffer, bytes, MSG_DONTWAIT);

                    break;
                }

                default:
                    break;
            }
        }
    }  
}

void terminateServer(int sigNum) {
    printf("Terminating server...\n");

    User *currentUser;
    Room *currentRoom;

    map<int, User*>::iterator userIt = users.begin();
    map<int, User*>::iterator userEnd = users.end();

    map<int, Room*>::iterator roomIt = rooms.begin();
    map<int, Room*>::iterator roomEnd = rooms.end();

    while(userIt != userEnd) {
        currentUser = userIt->second;

        close(currentUser->descriptor);

        free(currentUser->name);
        delete currentUser;

        userIt++;
    }

    while(roomIt != roomEnd) {
        currentRoom = roomIt->second;
        
        free(currentRoom->name);
        delete currentRoom;

        roomIt++;
    }

    close(epollDescriptor);
    close(serverDescriptor);

    exit(0);
}

int main(int argc, char *argv[]) {
    struct sigaction terminationAction;
    memset(&terminationAction, 0, sizeof(terminationAction));
    terminationAction.sa_handler = terminateServer;

    sigaction(SIGINT, &terminationAction, NULL);

    sockaddr_in serverAddress;
    sockaddr_in clientAddress;

    int clientDescriptor;
    int addressSize;
    int connectionsCounter;

    struct epoll_event event;

    addressSize = sizeof(clientAddress);
    userCounter = 0;
    connectionsCounter = 0;

    memset((char*) &serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;

    // Use default IP and PORT values
    if(argc < 3) {
        serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
        serverAddress.sin_port = htons(SERVER_PORT);
    }

    // Use parameters value
    else {
        serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
        serverAddress.sin_port = htons(atoi(argv[2]));
    }

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
        if((clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddress, (unsigned int*) &addressSize)) < 0) {
            printf("Error accepting client connection.\n");
            printf("Client info:\n\tClient IP: %s\n\tClient Port: %d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            continue;
        }

        if(connectionsCounter < MAX_USERS) {
            event.events = EPOLLIN;
            event.data.fd = clientDescriptor;

            sem_wait(&epollSemaphor);

            epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, clientDescriptor, &event);

            sem_post(&epollSemaphor);

            connectionsCounter++;
        }

        else {
            printf("Maximum users reached. Connection refused.\n");
            close(clientDescriptor);
        }
    }

    return 0;
}