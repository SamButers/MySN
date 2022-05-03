#include "userOperations.hpp"

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
int createRoom(char *roomName, int roomNameLength, char userLimit) {
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

char changeInfo(int descriptor, char pictureId) {
    User *targetUser = users.find(descriptor) != users.end() ? users[descriptor] : NULL;
    Room *targetRoom;

    if(targetUser == NULL)
        return -1;

    targetRoom = rooms.find(targetUser->roomId) != rooms.end() ? rooms[targetUser->roomId] : NULL;

    targetUser->pictureId = pictureId;

    if(targetRoom != NULL)
        sendUserUpdate(targetRoom, targetUser, 2);

    return pictureId;
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
        leaveRoom(user->descriptor);

        users.erase(descriptor);
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
       roomInfoBuffer[4 + bytes] = currentRoom->userLimit;
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
       userInfoBuffer[4 + bytes] = currentUser->pictureId;
       userInfoBuffer[5 + bytes] = (char) nameLength;
       memcpy(userInfoBuffer + 6 + bytes, currentUser->name, nameLength);

       bytes += 6 + nameLength;
       it++;
    }

    userInfoBuffer[2] = userCount;

    return bytes;
}