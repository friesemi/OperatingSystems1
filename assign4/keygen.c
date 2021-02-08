#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>



//make function to generate number of random numbers
char *generateKey(int keyLength, char *key) {
	int i = 0, randInt;
	char randChar[32];
	for (i = 0; i < keyLength; i++) {
		randInt = rand() % 27;
		if (randInt == 26) {
			strcpy(randChar, " ");
		}
		else {
			sprintf(randChar, "%c", (randInt + 65));
		}
		strcat(key, randChar);
	}
	strcat(key, "\n");
	return key;
}



int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Incorrect number of arguments\n");
		exit(0);
	}

	int i = 0, keyLen = atoi(argv[1]);
	char *key = malloc(keyLen * sizeof(char));
	srand(time(NULL));

	key = generateKey(keyLen, key);
	printf("%s", key);
	free(key);
	return 0;
}