#include "updateFunctions.hpp"

void sendUserJoinUpdate(Room *room, User *user) {
    int userDescriptor;
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
    char *buffer = (char*) malloc(6);

    buffer[0] = 1;
    buffer[1] = 3;
    memcpy(buffer + 2, &(user->id), 4);

    sendBufferToUsers(buffer, 6, room->users, -1);

    free(buffer);
}

void userInfoUpdate(Room *room, User *user) {
    int userDescriptor;
    char *buffer = (char*) malloc(7);

    buffer[0] = 1;
    buffer[1] = 4;
    memcpy(buffer + 2, &(user->id), 4);
    buffer[6] = (char) user->pictureId;

    userDescriptor = user->descriptor;

    sendBufferToUsers(buffer, 7, room->users, userDescriptor);

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
        case 2:
            printf("INFO UPDATE");
            userInfoUpdate(room, user);
            break;
        default:
            return;
    }
}

void sendRoomUpdate(Room *room, int updaterDescriptor) {
    printf("ROOMS UUPDATE\n");
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