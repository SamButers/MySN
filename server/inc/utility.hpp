#ifndef UTILITY_HPP_
#define UTILITY_HPP_

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <iterator>

#include "definitions.hpp"

void clearDescriptor(int descriptor);
int sendErrorResponse(int descriptor, char functionId, char *buffer);
void deleteRoom(Room *room);
int getRandomRoomId();
void sendBufferToUsers(char *buffer, int bytes, map<int, User*> &users, int updaterDescriptor);

#endif