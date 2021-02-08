//this is likened to a client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


int main(int argc, char *argv[]) {
	//argc will need to be 4 when having files
	if (argc < 3) {
		fprintf(stderr, "Incorrect amount of parameters\n");
		exit(0);
	}

	//socket variables
	int socketFD, portNumber, plainRead, keyRead, plainLen, keyLen, i, badChars = 0;
	FILE *keyFileFD, *plainFD;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char *bufferKey = calloc(70001, sizeof(char)), *bufferPlain = calloc(70001, sizeof(char)), *bufferEnc;
	char charRead, charKRead;

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Connect via localhost
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	//open files and read them to send to daemon
	//get plaintext into bufferPlain
	plainFD = fopen(argv[1], "r");
	while ((charRead = fgetc(plainFD)) != EOF) {
		strncat(bufferPlain, &charRead, 1);
	}

	//check if any illegal characters are in the plaintext
	//needs to be strlen - 1 to cut off newline
	for (i = 0; i < strlen(bufferPlain) - 1; i++) {
		if (bufferPlain[i] < 65 || bufferPlain[i] > 90) {
			if (bufferPlain[i] != 32) {
				badChars = 1;
			}
		}
	}

	//get key into bufferKey
	keyFileFD = fopen(argv[2], "r");
	while ((charKRead = fgetc(keyFileFD)) != EOF) {
		strncat(bufferKey, &charKRead, 1);
	}

	//check if key is shorter than the plaintext
	if (strlen(bufferPlain) > strlen(bufferKey)) {
		fprintf(stderr, "ERROR: key is too short\n");
		return 1;
	}
	//throw error if any illegal characters are present
	else if (badChars) {
		fprintf(stderr, "ERROR: plaintext contains illegal characters\n");
		return 1;
	}
	else {
		//send the length of plaintext for reference
		plainLen = htonl(strlen(bufferPlain) - 1);
		plainRead = send(socketFD, &plainLen, sizeof(plainLen), 0);

		//send the plaintext to the server
		bufferPlain = strtok(bufferPlain, "\n");
		plainRead = send(socketFD, bufferPlain, strlen(bufferPlain), 0); // Write to the server
		if (plainRead < 0) error("CLIENT: ERROR writing to socket");

		//send the length of the key for reference
		keyLen = htonl(strlen(bufferKey) - 1);
		send(socketFD, &keyLen, sizeof(keyLen), 0);

		//send the key to the server
		bufferKey = strtok(bufferKey, "\n");
		keyRead = send(socketFD, bufferKey, strlen(bufferKey), 0); // Write to the server
		if (keyRead < 0) error("CLIENT: ERROR writing to socket");

		//allocate space for the encrypted text and receive from the server into bufferEnc
		bufferEnc = calloc(strlen(bufferPlain) + 1, sizeof(char));
		plainRead = 0;
		plainRead = recv(socketFD, bufferEnc, strlen(bufferPlain), 0);
		if (plainRead < 0) error("CLIENT: ERROR reading from socket");
		//add newline to the end and print to stdout
		strncat(bufferEnc, "\n", 1);
		printf("%s", bufferEnc);
	}

	//free loose memory
	free(bufferPlain);
	free(bufferKey);
	free(bufferEnc);
	close(socketFD); // Close the socket
	return 0;
}