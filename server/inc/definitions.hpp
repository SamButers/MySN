#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <map>
#include <iterator>

#include <semaphore.h>

#define MAX_USERS   256
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

extern map<int, User*> users;
extern map<int, Room*> rooms;
extern int userCounter;

extern int epollDescriptor;

extern sem_t epollSemaphor;

extern char flushBuffer[FLUSH_BUFFER_SIZE];
extern char roomInfoBuffer[68102];
extern char userInfoBuffer[16897];
extern char roomUpdateBuffer[136];

#endif