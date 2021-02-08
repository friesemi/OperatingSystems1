//this is likened to a server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Incorrect number of parameters\n");
		exit(0);
	}

	//variables to hold socket information
	int listenSocketFD, establishedConnectionFD, portNumber, encRead, keyRead, encryptLen, keyLen, i;
	socklen_t sizeOfClientInfo;
	char bufferEncrypt[70001], bufferKey[70001], *bufferDec;
	struct sockaddr_in serverAddress, clientAddress;
	pid_t forkedChild;

	memset((char *)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[1]);
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

		//this section is where the file will be received/encrypted
		//need to receive length of plaintext first
		recv(establishedConnectionFD, &encryptLen, sizeof(encryptLen), 0);
		memset(bufferEncrypt, '\0', 70001);
		encRead = recv(establishedConnectionFD, bufferEncrypt, ntohl(encryptLen), 0); // Read the client's message from the socket
		if (encRead < 0) {
			error("ERROR reading from socket");
		}

		//receive the length of the key first
		recv(establishedConnectionFD, &keyLen, sizeof(keyLen), 0);
		//receive the key according to the size
		memset(bufferKey, '\0', 70001);
		keyRead = recv(establishedConnectionFD, bufferKey, ntohl(keyLen), 0);
		if (keyRead < 0) {
			error("ERROR reading from socket");
		}

		//fork here and encrypt the file
		forkedChild = fork();
		if (forkedChild == 0) {
			bufferDec = calloc(ntohl(encryptLen), sizeof(char));
			for (i = 0; i < ntohl(encryptLen); i++) {
				//convert the ascii characters to 0-26 range
				if (bufferEncrypt[i] == 32) { bufferEncrypt[i] = 26; }
				else { bufferEncrypt[i] = bufferEncrypt[i] - 65; }

				if (bufferKey[i] == 32) { bufferKey[i] = 26; }
				else { bufferKey[i] = bufferKey[i] - 65; }

				//subtract the encrypted text by the key to decode the text
				bufferDec[i] = bufferEncrypt[i] - bufferKey[i];
				if (bufferDec[i] < 0) { bufferDec[i] = bufferDec[i] + 27; }
				if (bufferDec[i] == 26) { bufferDec[i] = 32; }
				else { bufferDec[i] = bufferDec[i] + 65; }
			}

			//this is where the encrypted file will be returned
			encRead = send(establishedConnectionFD, bufferDec, strlen(bufferDec), 0); // Send success back
			if (encRead < 0) error("ERROR writing to socket");
			free(bufferDec);
		}
		//parent process that waits for the child to send decoded text back
		else if (forkedChild > 0) {
			wait(NULL);
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
		}
	}
	close(listenSocketFD); // Close the listening socket
}