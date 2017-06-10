#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	size_t max_length = 80000;
	char* message;
	char* key;
	char* newline;
	FILE* file;

	if (argc != 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); fflush(stderr); exit(1); } // Check usage & args

	// Get message from file
	message = (char*)malloc(sizeof(char) * max_length);
	memset(message, '\0', max_length);
	file = fopen(argv[1], "r");
	if (!file) { fprintf(stderr,"CLIENT: ERROR opening file %s\n", argv[1]); exit(1); }
	if (getline(&message, &max_length, file) == -1) {
		fprintf(stderr,"CLIENT: ERROR reading file %s\n", argv[1]); exit(1);
	}
	if (( newline = strchr(message, '\n' )) != NULL ) {
		*newline = '\0';
	}
	fclose(file);

	// Get key from file
	key = (char*)malloc(sizeof(char) * max_length);
	memset(key, '\0', max_length);
	file = fopen(argv[2], "r");
	if (!file) { fprintf(stderr,"CLIENT: ERROR opening file %s\n", argv[2]); exit(1); }
	if (getline(&key, &max_length, file) == -1) {
		fprintf(stderr,"CLIENT: ERROR reading file %s\n", argv[2]); exit(1);
	}
	if (( newline = strchr(key, '\n' )) != NULL ) {
		*newline = '\0';
	}
	fclose(file);

	int msgSize = strlen(message);
	int keySize = strlen(key);
	if (msgSize > keySize) { fprintf(stderr,"CLIENT: ERROR, key is smaller than plaintext\n"); exit(1); } // Check key size

	int i;
	for (i = 0; i < msgSize; i++) {
		if (message[i] != 32 && (message[i] < 65 || message[i] > 90)) {
			fprintf(stderr,"CLIENT: ERROR bad characters present\n"); exit(1);
		}
	}

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(2); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
		fprintf(stderr,"Error: could not contact otp_enc_d on port %d\n", portNumber);
		exit(2);
	}

	char ok_buf[3];
	memset(ok_buf,'\0', sizeof(ok_buf));

	// Send client id to server
	charsWritten = send(socketFD, "ENC", 3, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < 3) printf("CLIENT: WARNING: Not all data written to socket!\n");
	charsRead = recv(socketFD, ok_buf, 2, 0); // wait until server sends success message to indicate it is done reading
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	if (strcmp(ok_buf, "ok") != 0) exit(1);

	char msgSizeCh[6];
	memset(msgSizeCh,'\0',sizeof(msgSizeCh));
	sprintf(msgSizeCh, "%d", msgSize);

	// Send message size to server
	charsWritten = send(socketFD, msgSizeCh, strlen(msgSizeCh), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(msgSizeCh)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	charsRead = recv(socketFD, ok_buf, 2, 0); // wait until server sends success message to indicate it is done reading
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// Send plaintext message to server
	charsWritten = send(socketFD, message, strlen(message), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(message)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	charsRead = recv(socketFD, ok_buf, 2, 0); // wait until server sends success message to indicate it is done reading
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// Send key to server
	charsWritten = send(socketFD, key, strlen(key), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(key)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	charsRead = recv(socketFD, ok_buf, 2, 0); // wait until server sends success message to indicate it is done reading
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// Get return message from server
	memset(message, '\0', sizeof(message)); // Clear out the message again for reuse
	int totalRead = 0;
	char buffer[1024];
	while (totalRead < msgSize) {
		memset(buffer, '\0', 1024);
		charsRead = recv(socketFD, buffer, 1023, 0); // Read the message from the socket
		if (charsRead < 1) error("ERROR reading from socket");
		totalRead += charsRead;
		strcat(message,buffer);
	}

	printf("%s\n", message);
	fflush(stdout);

	close(socketFD); // Close the socket
	free(message);
	free(key);

	return 0;
}
