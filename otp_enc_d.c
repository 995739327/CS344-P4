#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char completeMessage[80000];
	char key[80000];
	char buffer[1024];
	char id_buffer[4];
	char num_buffer[6];
	struct sockaddr_in serverAddress, clientAddress;
	char* client_id = "ENC";

	if (argc != 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while (1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		// Fork the process to encrypt the message
		pid_t pid;
		pid = fork();
		if (pid == 0) { // this is the child process

			int msgSize, i;

			// Get the client id from the client
			memset(id_buffer, '\0', sizeof(id_buffer));
			charsRead = recv(establishedConnectionFD, id_buffer, 3, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			if (strcmp(id_buffer,client_id) != 0) {
				fprintf(stderr,"Error, connection refused, unauthorized client.\n");
				send(establishedConnectionFD, "no", 2, 0);
				exit(1);
			} // refuse connection id wrong client id
			charsRead = send(establishedConnectionFD, "ok", 2, 0);
			if (charsRead < 0) error("ERROR writing to socket");

			// Get message size from client
			memset(num_buffer, '\0', sizeof(num_buffer));
			charsRead = recv(establishedConnectionFD, num_buffer, 5, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			msgSize = atoi(num_buffer);

			// send message to indicate message received
			charsRead = send(establishedConnectionFD, "ok", 2, 0);
			if (charsRead < 0) error("ERROR writing to socket");

			// Get the complete plaintext message
			int totalRead;
			while (totalRead < msgSize) {
				memset(buffer, '\0', 1024);
				charsRead = recv(establishedConnectionFD, buffer, 1023, 0); // Read the client's message from the socket
				if (charsRead < 1) error("ERROR reading from socket");
				totalRead += charsRead;
				strcat(completeMessage,buffer);
			}
			// send message to indicate message received
			charsRead = send(establishedConnectionFD, "ok", 2, 0);
			if (charsRead < 0) error("ERROR writing to socket");

			// get the key
			totalRead = 0;
			while (totalRead < msgSize) {
				memset(buffer, '\0', 1024);
				charsRead = recv(establishedConnectionFD, buffer, 1023, 0); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket");
				if (charsRead == 0) { fprintf(stderr,"Error: key not large enough\n"); exit(1); }
				totalRead += charsRead;
				strcat(key,buffer);
			}
			// send message to indicate message received
			charsRead = send(establishedConnectionFD, "ok", 2, 0);
			if (charsRead < 0) error("ERROR writing to socket");

			// Encode the message with the key
			for (i = 0; i < strlen(completeMessage); i++) {
				if (completeMessage[i] == 32) completeMessage[i] = 91;
				if (key[i] == 32) key[i] = 91;
				completeMessage[i] = ((completeMessage[i] - 65 + key[i] - 65) % 27) + 65;
				if (completeMessage[i] == 91) completeMessage[i] = 32;
			}

			// Send the encoded message back to the client
			charsRead = send(establishedConnectionFD, completeMessage, strlen(completeMessage), 0);
			if (charsRead < 0) error("ERROR writing to socket");
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			close(listenSocketFD); // Close the listening socket
			exit(0);

		} else if (pid < 0) {
			perror("fork error");
			fflush(stderr);
		} else { // this is the parent process
			waitpid(pid, NULL, WNOHANG);
		}

	}

	return 0;

}
