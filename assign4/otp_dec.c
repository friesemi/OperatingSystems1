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
	int socketFD, portNumber, plainRead, keyRead, encryptRead, encryptLen, keyLen;
	FILE *keyFileFD, *plainFD;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char *bufferKey = calloc(70001, sizeof(char)), *bufferEncrypt = calloc(70001, sizeof(char)), *bufferPlain;
	char charRead, charKRead;

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Connect using localhost
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	//open files and read them to send to daemon
	plainFD = fopen(argv[1], "r");
	while ((charRead = fgetc(plainFD)) != EOF) {
		strncat(bufferEncrypt, &charRead, 1);
	}

	keyFileFD = fopen(argv[2], "r");
	while ((charKRead = fgetc(keyFileFD)) != EOF) {
		strncat(bufferKey, &charKRead, 1);
	}

	//error if the key is shorter than the plaintext
	if (strlen(bufferEncrypt) > strlen(bufferKey)) {
		fprintf(stderr, "ERROR: key it too short\n");
	}
	else {
		//send the length of enctext for reference
		encryptLen = htonl(strlen(bufferEncrypt) - 1);
		send(socketFD, &encryptLen, sizeof(encryptLen), 0);

		//send the encrypted text to the server
		bufferEncrypt = strtok(bufferEncrypt, "\n");
		encryptRead = send(socketFD, bufferEncrypt, strlen(bufferEncrypt), 0); // Write to the server
		if (encryptRead < 0) error("CLIENT: ERROR writing to socket");
		if (encryptRead < strlen(bufferEncrypt)) printf("CLIENT: WARNING: Not all data written to socket!\n");

		//send the length of the key for reference
		keyLen = htonl(strlen(bufferKey) - 1);
		send(socketFD, &keyLen, sizeof(keyLen), 0);

		//send the key to the server
		bufferKey = strtok(bufferKey, "\n");
		keyRead = send(socketFD, bufferKey, strlen(bufferKey), 0); // Write to the server
		if (keyRead < 0) error("CLIENT: ERROR writing to socket");
		if (keyRead < strlen(bufferKey)) printf("CLIENT: WARNING: Not all data written to socket!\n");

		//receive the decrypted text back from the server and add the newline
		bufferPlain = calloc(strlen(bufferEncrypt) + 1, sizeof(char));
		plainRead = recv(socketFD, bufferPlain, strlen(bufferEncrypt), 0);
		if (plainRead < 0) error("CLIENT: ERROR reading from socket");
		strncat(bufferPlain, "\n", 1);
		printf("%s", bufferPlain);
	}

	//free all loose memory
	free(bufferEncrypt);
	free(bufferKey);
	free(bufferPlain);
	close(socketFD); // Close the socket
	return 0;
}