#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080

using namespace std;

void loginUser(int socketDescriptor) {
	char buffer[6];
	char username[] = "Test";
	buffer[0] = 0;
	buffer[1] = 4;
	memcpy(buffer + 2, username, (size_t) buffer[1]);

	send(socketDescriptor, buffer, 6, 0);
}

void logoutUser(int socketDescriptor) {
	char buffer[1];
	buffer[0] = 1;

	send(socketDescriptor, buffer, 1, 0);
}

int main() {
	struct sockaddr_in serverAddress;
	int socketDescriptor;

	memset((char *) &serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddress.sin_port = htons(SERVER_PORT);

	socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if(socketDescriptor < 0) {
		cout << "Error during socket creation." << endl;
		return 1;
	}

	if(connect(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		cout << "Error during connection." << endl;
		return 1;
	}

	// Send valid login
	loginUser(socketDescriptor);

	// Send valid logout
	logoutUser(socketDescriptor);

	close(socketDescriptor);

	return 0;
}

