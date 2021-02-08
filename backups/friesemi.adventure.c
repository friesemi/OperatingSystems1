#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct Room {
	char* nameOfRoom;
	char* roomType;
	int numConnections;
	int index;
	char* connections[9];
}Room;

//global thread variables
pthread_t threadTime;
pthread_mutex_t timer;

//function to display end of game text and path steps/names
void endGame(int pathSteps, char pathNames[20][9]) {
	int i = 0;
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", pathSteps);
	for (i; i < pathSteps; i++) {
		printf("%s\n", pathNames[i]);
	}
	exit(0);
}


//writes the time to a file when the time thread is woken up
void writeTime() {
	time_t t = time(NULL);
	struct tm *tmp;
	char *timeFile = "currentTime.txt", timeOutput[64];
	FILE *fp = fopen(timeFile, "w");

	tmp = localtime(&t);
	strftime(timeOutput, sizeof(timeOutput), "%I:%M%P, %A, %B %d, %Y\n", tmp);
	fprintf(fp, "%s", timeOutput);
	fclose(fp);
}


//prints the time
void printTime() {
	FILE *fp = fopen("currentTime.txt", "r");
	char timeOutput[64];
	printf("%s\n", fgets(timeOutput, 64, fp));
}


//validates that user input is valid
int validateUserInput(Room room, Room *rooms, char *input) {
	int j, i;
	strtok(input, "\n");
	for (j = 0; j < room.numConnections; j++) {
		if (strcmp(room.connections[j], input) == 0) {
			for (i = 0; i < 7; i++) {
				if (strcmp(room.connections[j], rooms[i].nameOfRoom) == 0) {
					printf("\n");
					return rooms[i].index;
				}
			}
		}
	}
	//unlock the mutex to allow the time to be written to the file and relock it after the time thread finishes
	if (strcmp(input, "time") == 0) {
		pthread_mutex_unlock(&timer);
		writeTime();

		pthread_join(threadTime, NULL);
		pthread_mutex_unlock(&timer);
		//print the time
		printTime();
		pthread_mutex_lock(&timer);
	}
	else
		printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
	return -1;
}


//plays through the game with text displays
void playGame(Room room, Room *rooms, int pathSteps, char pathNames[20][9]) {
	int j = 0, nextMove = -1;
	size_t enterSize = 9;
	char *userEnterPath = malloc(9 * sizeof(char));

	do {
		//checks initially if the room the user is in is the end
		if (strcmp(room.roomType, "END_ROOM") == 0) {
			strcpy(pathNames[pathSteps], room.nameOfRoom);
			pathSteps++;
			endGame(pathSteps, pathNames);
		}
		j = 0;

		printf("CURRENT LOCATION: %s\n",room.nameOfRoom);

		//print out possible connections the user can choose
		printf("POSSIBLE CONNECTIONS: ");
		for (j; j < room.numConnections; j++) {
			printf("%s", room.connections[j]);
			if (j == (room.numConnections - 1))
				printf(".\n");
			else
				printf(", ");
		}

		//get user input and need to validate it 
		printf("WHERE TO? > ");
		if (getline(&userEnterPath, &enterSize, stdin) == -1) {
			printf("error\n");
		}
		else {
			//validates users input, should return index of moved to room
			printf("\n");
			//move rooms and increment path/i
			if ((nextMove = validateUserInput(room, rooms, userEnterPath)) >= 0) {
				if (strcmp(room.roomType, "START_ROOM") != 0 || pathSteps > 0) {
					strcpy(pathNames[pathSteps], room.nameOfRoom);
					pathSteps++;
				}
				playGame(rooms[nextMove], rooms, pathSteps, pathNames);
			}
		}
	} while (1);
}


char *getNewestDir() {
	int newestDirTime = -1; // Modified timestamp of newest subdir examined
	char targetDirPrefix[32] = "friesemi.rooms."; // Prefix we're looking for
	static char newestDirName[256]; // Holds the name of the newest dir that contains prefix
	memset(newestDirName, '\0', sizeof(newestDirName));

	DIR* dirToCheck;
	struct dirent *fileInDir; // Holds the current subdir of the starting dir
	struct stat dirAttributes; // Holds information we've gained about subdir

	dirToCheck = opendir("."); // Open up the directory this program was run in

	if (dirToCheck > 0) // Make sure the current directory could be opened
	{
		while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
		{
			if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
			{
				stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

				if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
				{
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name);

				}

			}

		}
	}
	closedir(dirToCheck);

	return newestDirName;
}


void readFiles(Room *rooms, char *dirName) {
	int i = 0, j = 0;
	char fileContents[32], fileToRead[32], *tempContents, parse[32];
	FILE *fp;
	DIR *currentDir;
	struct dirent *fileInDir;

	currentDir = opendir(dirName);
	while ((fileInDir = readdir(currentDir)) != NULL) {				//parse over all the files in the directory
		j = 0;
		if ((int)fileInDir->d_type == 4)							//check for .. and . directories and ignore them
			continue;
		else {														//else read from files
			sprintf(fileToRead, "%s/%s", dirName, fileInDir->d_name);
			fp = fopen(fileToRead, "r");

			sscanf(fgets(fileContents, 32, fp), "%*s %*s %s", rooms[i].nameOfRoom);

			while (tempContents = fgets(fileContents, 32, fp)) {	//get each connection and save them into the rooms struct array
				sscanf(tempContents, "%*s %*s %s", parse);

				if ((strcmp(parse, "END_ROOM") == 0) || (strcmp(parse, "MID_ROOM") == 0) || (strcmp(parse, "START_ROOM") == 0)) {
					strcpy(rooms[i].roomType, parse);
				}
				else {
					strcpy(rooms[i].connections[j], parse);			//save the connection to the connections array
					rooms[i].numConnections++;
					j++;
				}
			}
			fclose(fp);
			i++;
		}
	}
	closedir(currentDir);
}


//function called when creating the second thread
void *lockThread(void *arg) {
	pthread_mutex_lock(&timer);
	return NULL;
}


//main function that starts program
int main() {
	//threading calls
	pthread_mutex_init(&timer, NULL);
	pthread_mutex_lock(&timer);
	pthread_create(&threadTime, NULL, &lockThread, NULL);

	//read files into struct
	Room *rooms = (Room*)calloc(7, sizeof(Room));					//allocate space for each room struct and loop through to allocate memory for each pointer
	int i, j, pathSteps = 0;
	char *dirName;
	char pathNames[20][9];
	for (i = 0; i < 7; i++) {
		rooms[i].index = i;
		rooms[i].nameOfRoom = malloc(9 * sizeof(char));
		rooms[i].roomType = malloc(11 * sizeof(char));
		for (j = 0; j < 6; j++) {
			rooms[i].connections[j] = malloc(9 * sizeof(char));
		}
	}

	//get the name of the newest directory and populate the structs from files
	dirName = getNewestDir();
	readFiles(rooms, dirName);
	//play the game, pass in the rooms that are populated from reading files
	for (i = 0; i < 7; i++) {
		if (strcmp(rooms[i].roomType, "START_ROOM") == 0) {
			playGame(rooms[i], rooms, pathSteps, pathNames);
		}
	}

	return 0;
}