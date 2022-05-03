#ifndef USER_OPERATIONS_HPP_
#define USER_OPERATIONS_HPP_

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <iterator>

#include <sys/epoll.h>

#include "definitions.hpp"
#include "utility.hpp"
#include "update.hpp"

int loginUser(int descriptor, char *displayName, int displayNameSize);
int createRoom(char *roomName, int roomNameLength, int userLimit);
int joinRoom(int descriptor, int roomId);
char sendMessage(int descriptor, char *content, int length);
int changeInfo(int descriptor, int pictureId);
int leaveRoom(int descriptor);
void logoutUser(int descriptor);
int getRooms();
int getUsers(int descriptor);

#endif