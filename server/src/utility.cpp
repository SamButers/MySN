#include "utility.hpp"

// Reads and clear descriptor with invalid data
void clearDescriptor(int descriptor) {
    int readBytes = recv(descriptor, &flushBuffer, FLUSH_BUFFER_SIZE, MSG_DONTWAIT);

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
            return send(descriptor, buffer, 2, MSG_DONTWAIT) != 2;

        case 6:
        case 3:
        case 2:
        case 0:
            integerResponse = -1;
            memcpy(buffer + 2, &integerResponse, 4);

            return send(descriptor, buffer, 6, MSG_DONTWAIT) != 6;

        case 4:
            buffer[2] = -1;

            return send(descriptor, buffer, 3, MSG_DONTWAIT) != 3;

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
            send(userDescriptor, buffer, bytes, MSG_DONTWAIT);

        it++;
    }
}