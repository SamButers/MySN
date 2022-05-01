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
#include <iterator>

#define MAX_USERS   256
#define MAX_ROOMS   256
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define FLUSH_BUFFER_SIZE 64000

using namespace std;

typedef struct sockaddr_in sockaddr_in;

typedef struct User {
    int id, pictureId, descriptor, roomId;
    char* name;
} User;

typedef struct Room {
    int id, userLimit;
    char nameLength;
    char* name;
    std::map<int, User*> users;
} Room;

typedef struct Message {
    int userId;
    char *content;
} Message;

map<int, User*> users;
map<int, Room*> rooms;
int userCounter;

int epollDescriptor;

sem_t epollSemaphor, usersSemaphor, roomsSemaphor;

// Buffers
char flushBuffer[FLUSH_BUFFER_SIZE];
char roomInfoBuffer[68102];
char userInfoBuffer[16897];
char roomUpdateBuffer[136];

//Utility functions
void clearDescriptor(int descriptor) {
    printf("Reading...\n");
    int readBytes = recv(descriptor, &flushBuffer, FLUSH_BUFFER_SIZE, MSG_DONTWAIT);
    printf("READ IT\n");

    if(readBytes < 0)
        printf("Error on flushing.\n");
    else
        printf("Flushed bytes: %d\n", readBytes);
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

        case 3:
        case 2:
        case 0:
            integerResponse = -1;
            memcpy(buffer + 2, &integerResponse, 4);

            return write(descriptor, buffer, 6) != 6;

        case 4:
            buffer[2] = -1;

            return write(descriptor, buffer, 3) != 3;

        default:
            return 1;
    }
}

void deleteRoom(Room *room) {
    rooms.erase(room->id);

    free(room->name);
    delete room;
}

int getRandomRoomId() {
    int id;

    do {
        id = abs(rand());
    } while(rooms.find(id) != rooms.end());

    return id;
}

void sendBufferToUsers(char *buffer, int bytes, map<int, User*> &users, int updaterDescriptor) {
    map<int, User*>::iterator it = users.begin();
    map<int, User*>::iterator end = users.end();

    int userDescriptor;

    while(it != end) {
        userDescriptor = it->first;

        if(userDescriptor != updaterDescriptor)
            write(userDescriptor, buffer, bytes);

        it++;
    }
}
// End of utility functions

// Update functions

void sendUserJoinUpdate(Room *room, User *user) {
    map<int, User*>::iterator it = room->users.begin();
    map<int, User*>::iterator end = room->users.end();

    int userDescriptor, targetDescriptor;
    int userNameLength = strlen(user->name);
    char *buffer = (char*) malloc(8 + userNameLength);

    buffer[0] = 1;
    buffer[1] = 2;

    userDescriptor = user->descriptor;

    memcpy(buffer + 2, &(user->id), 4);
    buffer[6] = (char) user->pictureId;
    buffer[7] = (char) userNameLength;
    memcpy(buffer + 8, user->name, userNameLength);

    sendBufferToUsers(buffer, 8 + userNameLength, room->users, userDescriptor);

    free(buffer);
}

void sendUserLeaveUpdate(Room *room, User *user) {
    map<int, User*>::iterator it = room->users.begin();
    map<int, User*>::iterator end = room->users.end();

    int targetDescriptor;
    char *buffer = (char*) malloc(6);

    buffer[0] = 1;
    buffer[1] = 3;
    memcpy(buffer + 2, &(user->id), 4);

    sendBufferToUsers(buffer, 6, room->users, -1);

    free(buffer);
}

void sendUserUpdate(Room *room, User *user, char type) {
    switch(type) {
        case 0:
            printf("LEAVE UPDATE\n");
            sendUserLeaveUpdate(room, user);
            break;
        case 1:
            printf("JOIN UPDATE\n");
            sendUserJoinUpdate(room, user);
            break;
        /*case 2:
            userInfoUpdate(room, updaterDescriptor);
            break;*/
        default:
            return;
    }
}

void sendRoomUpdate(Room *room, int updaterDescriptor) {
    int reservedBytes;
    char userAmount;

    reservedBytes = 2;
    userAmount = (char) room->users.size();

    roomUpdateBuffer[0] = 1;
    roomUpdateBuffer[1] = 0;

    memcpy(roomUpdateBuffer + reservedBytes, &(room->id), 4);
    memcpy(roomUpdateBuffer + 4 + reservedBytes, &(room->userLimit), 1);
    roomUpdateBuffer[5 + reservedBytes] = userAmount;
    roomUpdateBuffer[6 + reservedBytes] = room->nameLength;
    memcpy(roomUpdateBuffer + 7 + reservedBytes, room->name, (size_t) room->nameLength);

    sendBufferToUsers(roomUpdateBuffer, reservedBytes + 7 + (int) room->nameLength, users, updaterDescriptor);
}

void sendRoomDeletionUpdate(Room *room) {
    int reservedBytes;

    reservedBytes = 2;

    roomUpdateBuffer[0] = 1;
    roomUpdateBuffer[1] = 0;

    memcpy(roomUpdateBuffer + reservedBytes, &(room->id), 4);
    roomUpdateBuffer[4 + reservedBytes] = -1;

    sendBufferToUsers(roomUpdateBuffer, reservedBytes + 5, users, -1);

    deleteRoom(room);
}

// End of update functions

// User operation functions
int loginUser(int descriptor, char *displayName, int displayNameSize) {
    if(users.find(descriptor) == users.end()) {
        User *newUser;

        users[descriptor] = new User();
        newUser = users[descriptor];

        newUser->id = userCounter;
        newUser->pictureId = 0;
        newUser->descriptor = descriptor;
        newUser->roomId = -1;
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

// Pending:
//  Send update
int createRoom(char *roomName, int roomNameLength, int userLimit) {
    Room *newRoom = new Room();

    newRoom->id = getRandomRoomId();
    newRoom->userLimit = userLimit;
    newRoom->nameLength = (char) roomNameLength - 1;
    newRoom->name = (char*) malloc(roomNameLength);
    memcpy(newRoom->name, roomName, roomNameLength);

    rooms[newRoom->id] = newRoom;

    return newRoom->id;
}

// Pending:
//  Send update
int joinRoom(int descriptor, int roomId) {
    Room *targetRoom = rooms.find(roomId) != rooms.end() ? rooms[roomId] : NULL; //Storing the iterator may be more efficient, but perhaps less readable
    User *targetUser = users.find(descriptor) != users.end() ? users[descriptor] : NULL;

    if(targetRoom == NULL || targetUser == NULL)
        return -1;

    if(targetUser->roomId != -1)
        return -3;

    else if(targetRoom->users.size() >= targetRoom->userLimit)
        return -2;

    targetUser->roomId = roomId;
    targetRoom->users[descriptor] = targetUser;

    sendUserUpdate(targetRoom, targetUser, 1);
    sendRoomUpdate(targetRoom, descriptor);

    return roomId;
}

char sendMessage(int descriptor, char *content, int length) {
    User *targetUser = users.find(descriptor) != users.end() ? users[descriptor] : NULL;
    Room *targetRoom;

    if(targetUser == NULL)
        return -1;

    if(targetUser->roomId == -1)
        return -2;

    targetRoom = rooms.find(targetUser->roomId) != rooms.end() ? rooms[targetUser->roomId] : NULL;

    if(targetRoom == NULL)
        return -1;

    map<int, User*>::iterator it = targetRoom->users.begin();
    map<int, User*>::iterator end = targetRoom->users.end();

    int userDescriptor;
    char *buffer = (char*) malloc(8 + length);

    buffer[0] = 1;
    buffer[1] = 1;
    memcpy(buffer + 2, &(targetUser->id), 4);
    memcpy(buffer + 6, &length, 2);
    memcpy(buffer + 8, content, length);

    while(it != end) {
        userDescriptor = it->first;

        if(userDescriptor != descriptor)
            write(userDescriptor, buffer, length + 8);

        it++;
    }

    free(buffer);
    return 0;
}

int leaveRoom(int descriptor) {
    User *targetUser = users.find(descriptor) != users.end() ? users[descriptor] : NULL;
    Room *targetRoom;

    if(targetUser == NULL)
        return -1;

    targetRoom = rooms.find(targetUser->roomId) != rooms.end() ? rooms[targetUser->roomId] : NULL;

    if(targetRoom == NULL)
        return -1;

    targetRoom->users.erase(descriptor);
    targetUser->roomId = -1;

    if(targetRoom->users.size() <= 0)
        sendRoomDeletionUpdate(targetRoom);
    else {
        sendUserUpdate(targetRoom, targetUser, 0);
        sendRoomUpdate(targetRoom, -1);
    }

    return 0;
}

void logoutUser(int descriptor) {
    sem_wait(&epollSemaphor);

    epoll_ctl(epollDescriptor, EPOLL_CTL_DEL, descriptor, NULL);

    sem_post(&epollSemaphor);

    if(users.find(descriptor) != users.end()) {
        User *user = users[descriptor];
        users.erase(descriptor);

        leaveRoom(user->descriptor);
        free(user->name);
        delete user;
    }
}

// Can be optimized by buffering
int getRooms() {
    int bytes, roomCount;
    char users;
    map<int, Room*>::iterator it = rooms.begin();
    Room *currentRoom;

    bytes = 6; // Reserved Bytes
    roomCount = 0;

    while(it != rooms.end()) {
        // id, userLimit, users, roomnamelength => 7 bytes
        // roomname => ? bytes
       currentRoom = it->second;

       users = (char) currentRoom->users.size();

       memcpy(roomInfoBuffer + bytes, &(currentRoom->id), 4);
       memcpy(roomInfoBuffer + 4 + bytes, &(currentRoom->userLimit), 1);
       roomInfoBuffer[5 + bytes] = users;
       roomInfoBuffer[6 + bytes] = currentRoom->nameLength;
       memcpy(roomInfoBuffer + 7 + bytes, currentRoom->name, (size_t) currentRoom->nameLength);

       bytes += 7 + (int) currentRoom->nameLength;
       roomCount++;

       it++;
    }

    memcpy(roomInfoBuffer + 2, &roomCount, 4);

    return bytes;
}

int getUsers(int descriptor) {
    User *targetUser = users.find(descriptor) != users.end() ? users[descriptor] : NULL;
    Room *targetRoom;

    if(targetUser == NULL)
        return -1;

    targetRoom = rooms.find(targetUser->roomId) != rooms.end() ? rooms[targetUser->roomId] : NULL;

    if(targetRoom == NULL)
        return -1;

    int bytes, nameLength;
    char userCount;
    User *currentUser;

    map<int, User*>::iterator it = targetRoom->users.begin();
    map<int, User*>::iterator end = targetRoom->users.end();

    bytes = 3; // Reserved Bytes
    userCount = (char) targetRoom->users.size();

    while(it != end) {
       currentUser = it->second;
       nameLength = strlen(currentUser->name);

       memcpy(userInfoBuffer + bytes, &(currentUser->id), 4);
       userInfoBuffer[4 + bytes] = (char) currentUser->pictureId;
       userInfoBuffer[5 + bytes] = (char) nameLength;
       memcpy(userInfoBuffer + 6 + bytes, currentUser->name, nameLength);

       bytes += 6 + nameLength;
       it++;
    }

    userInfoBuffer[2] = userCount;

    return bytes;
}

// End of user operations functions

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
                    // Logout
                    printf("Logout\n");

                    //responseBuffer[0] = 0;
                    //responseBuffer[1] = 0;
                    //responseBuffer[2] = 0;

                    //write(events[c].data.fd, responseBuffer, 3);

                    logoutUser(events[c].data.fd);

                    break;
                }

                case 2: {
                    // Create room
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
                    // Join room
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
                    // Send message
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

                    //messageContent[messageLength] = '\0';

                    status = sendMessage(events[c].data.fd, messageContent, messageLength);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 4;
                    memcpy(responseBuffer + 2, &status, 1);

                    write(events[c].data.fd, responseBuffer, 3);

                    free(messageContent);

                    break;
                }

                case 5: {
                    // Leave room
                    printf("Leave room\n");
                    int roomId = leaveRoom(events[c].data.fd);

                    responseBuffer[0] = 0;
                    responseBuffer[1] = 5;
                    memcpy(responseBuffer + 2, &roomId, 4);

                    write(events[c].data.fd, responseBuffer, 6);

                    break;
                }

                case 6:
                    // Change info
                    break;

                case 7: {
                    // Get rooms
                    printf("Get rooms\n");
                    int bytes = getRooms();

                    roomInfoBuffer[0] = 0;
                    roomInfoBuffer[1] = 7;

                    write(events[c].data.fd, roomInfoBuffer, bytes);

                    break;
                }

                case 8: {
                    // Get users
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