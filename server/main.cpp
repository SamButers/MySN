#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>

#include <map>

#define MAX_USERS   256
#define MAX_ROOMS   256
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080

typedef struct sockaddr_in sockaddr_in;

typedef struct User {
    int id, pictureId, userDescriptor;
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

User users[MAX_USERS];
Room rooms[MAX_ROOMS];

fd_set userSet;

int userCounter;

void *userOperationsHandler(void *params) {
    
}

void *chatroomHandler(void *params) {

}

int main() {
    sockaddr_in serverAddress;
    sockaddr_in clientAddress;

    int serverDescriptor, clientDescriptor;
    int addressSize;

    pthread_t userOperationsThread, chatroomThreads[MAX_ROOMS];

    addressSize = sizeof(clientAddress);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet(SERVER_IP);
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

    FD_ZERO(userSet);

    pthread_create(&userOperationsThread, NULL, &userOperationsHandler, NULL);
    
    while(1) {
        if((clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddress, &addressSize)) < 0) {
            printf("Error accepting client connection.\n");
            printf("Client info:\n\tClient IP: %s\n\tClient Port: %d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            continue;
        }

        if(userCounter < MAX_USERS) {
            FD_SET(clientDescriptor, &userSet);
            // pingNewUserDescriptor();
            userCounter++;
        }

        else {
            printf("Maximum users reached. Connection refused.\n");
            close(userDescriptor);
        }
    }

    return 0;
}