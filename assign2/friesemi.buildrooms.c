#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


char filenames[10][9] = { "Boomer", "Banger", "Skipper", "Dungeon", "Sleeper", "Armory", "Spooky", "Medical", "Treasure", "Swinging" };


//populate room struct and pull from variables to put in file
typedef struct Room{
	char* nameOfRoom;
	char* roomType;
	int numConnections;
	struct Room* connections;
}Room;


//check to see if all rooms have 3-6 connections
int IsMapFull(Room *rooms) {
	int i;
	for (i = 0; i < 7; i++) {
		if (rooms[i].numConnections < 3)						//only checks lowest amount of connections
			return 1;											//graph is not full yet
	}
	return 0;													//each room has at least 3 connections, aka graph is full
}


//check if a connection can be made from room passed in
int canAddConnection(Room *rooms, int xRan) {
	if (rooms[xRan].numConnections < 6)
		return 1;												//return 1 if connection can be added to make if statement true
	return 0;													//return 0 if connection can't be made to stop addRandConnection
}


//check if a connection already exists between rooms
int isAlreadyConnection(Room *rooms, int xRan, int yRan) {
	int i = 0;
	do {
		if (rooms[xRan].connections[i].nameOfRoom == rooms[yRan].nameOfRoom) {
			return 0;											//return 0 if connection already exists
		}
		i++;
	} while (i < rooms[xRan].numConnections);
	return 1;													//return 1 if the connection doesn't exist already
}


//connect rooms
void connectRooms(Room *rooms, int xRan, int yRan) {
	rooms[xRan].connections[rooms[xRan].numConnections] = rooms[yRan];
	rooms[xRan].numConnections += 1;							//set connection struct equal to connected room and increment numConnections
}																//process is then repeated to have backwards connection made


//checks if the rooms are the same room
int sameRooms(Room *rooms, int xRan, int yRan) {
	if (rooms[xRan].nameOfRoom == rooms[yRan].nameOfRoom)
		return 0;												//return 0 if rooms are the same
	return 1;													//return 1 if the rooms aren't the same
}


//add random outbound connection from a room to another
int addRandConnection(Room *rooms) {
	int xRan = rand() % 7, yRan = rand() % 7;
	if (sameRooms(rooms, xRan, yRan) && canAddConnection(rooms, xRan) && canAddConnection(rooms, yRan) && isAlreadyConnection(rooms, xRan, yRan)) {
		connectRooms(rooms, xRan, yRan);						//connect the rooms and then call function again to make reversed connection
		connectRooms(rooms, yRan, xRan);
	}
}



//assign the types of the rooms
void assignRoomType(Room *rooms) {
	int i = 0, j = 0, ran1, ran2;
	ran1 = rand() % 7;
	do {
		ran2 = rand() % 7;
		if (ran1 = ran2)
			ran2 = rand() % 7;
	} while (ran1 == ran2);
	rooms[ran1].roomType = "START_ROOM";						//add start and end room to two seperate rooms
	rooms[ran2].roomType = "END_ROOM";

	do {
		if (rooms[i].roomType != "START_ROOM" && rooms[i].roomType != "END_ROOM") {
			rooms[i].roomType = "MID_ROOM";						//add mid room to all remaining rooms
			j++;
		}
		i++;
	} while (j < 5);
}


//writes all the information to the files
void writeToFile(Room *rooms, char *dirName) {
	FILE *fp;
	char pathName[32], fileName[32];
	int i, j;

	for (i = 0; i < 7; i++) {
		sprintf(fileName, "%s.txt", rooms[i].nameOfRoom);									//make the pathname with the room name and create a file path
		sprintf(pathName, "%s/%s", dirName, fileName);
		fp = fopen(pathName, "a");

		for (j = 0; j < rooms[i].numConnections; j++) {										//for each connection I need to make a connection header and append new connection
			fprintf(fp, "CONNECTION %d: %s\n", j+1, rooms[i].connections[j].nameOfRoom);
		}
		fprintf(fp, "ROOM TYPE: %s\n", rooms[i].roomType);									//append room type and then close file stream
		fclose(fp);
	}
}


//make 7 room files within new directory
void popArray(Room *rooms, char* dirName) {
	char pathName[32], *roomName, roomTextName[32];
	int randName = 0, i = 0, j = 0, randRoom = 0;
	FILE *fp;

	do {
		//generate a file and check if file already exists
		//open file and add .txt extension
		randName = rand() % 10;

		roomName = filenames[randName];
		sprintf(roomTextName, "%s.txt", roomName);
		sprintf(pathName, "%s/%s", dirName, roomTextName);

		if (access(pathName, F_OK) != -1) {													//check if the file exists to ensure no names are repeated
			continue;
		}
		else {
			rooms[i].nameOfRoom = roomName;
			rooms[i].numConnections = 0;													//populate structs with names and number of connections

			fp = fopen(pathName, "w");
			fprintf(fp, "ROOM NAME: %s\n", rooms[i].nameOfRoom);
			fclose(fp);

			i++;
		}
	} while (i < 7); //needs to run 7 times
	

	assignRoomType(rooms);
	while (IsMapFull(rooms)) {
		addRandConnection(rooms);															//continue adding connections until the graph is full
	}

	writeToFile(rooms, dirName);
}


//makes the directory to store rooms in
void makingDir(Room *rooms) {
	int procID = getpid();
	char  idNum[64] = { }, dirName[64] = { };

	sprintf(idNum, "%d", procID);
	strcpy(dirName, "friesemi.rooms.");
	strcat(dirName, idNum);

	mkdir(dirName, 0777);

	popArray(rooms, dirName);
}


//main function for the basis of the program
int main() {
	int i, j;
	char *dirName;
	Room *rooms = (Room*)calloc(7, sizeof(Room));					//allocate space for each room struct and loop through to allocate memory for each pointer
	for (i = 0; i < 7; i++) {
		rooms[i].nameOfRoom = malloc(9 * sizeof(char));
		rooms[i].roomType = malloc(11 * sizeof(char));
		rooms[i].connections = malloc(6 * sizeof(Room));
	}

	srand(time(NULL));

	makingDir(rooms);												//function to make the directory to store the files in
	return 0;
}