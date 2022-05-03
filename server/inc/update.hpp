#ifndef UPDATE_FUNC_HPP_
#define UPDATE_FUNC_HPP_

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <iterator>

#include "definitions.hpp"
#include "utility.hpp"

void sendUserJoinUpdate(Room *room, User *user);
void sendUserLeaveUpdate(Room *room, User *user);
void userInfoUpdate(Room *room, User *user);
void sendUserUpdate(Room *room, User *user, char type);
void sendRoomUpdate(Room *room, int updaterDescriptor);
void sendRoomDeletionUpdate(Room *room);

#endif