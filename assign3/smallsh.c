//here's the beginning of my new shell!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

//constants for max command size/length
#define MAX_COMMANDS 512
#define MAX_CHARS 2048

//array of process ids to check lingering processes
pid_t *processIds;
int numProcs = 0;
sig_atomic_t foregroundProc;
//ints to see if in foreground only mode
sig_atomic_t ctrlZcheck = 0;
//int designating exit status
int exitStatus = 0;
pid_t mainProc = 0;


//command to exit the shell and terminate any lingering processes
int exitCommand() {
	//check if there any processes running and kill them
	int i = 0;
	for (i; i < numProcs; i++) {
		if (processIds[i] != 0) {
			kill(processIds[i], SIGTERM);
		}
	}
	free(processIds);
	exit(0);
}


//command to display exit status/terminating signal of last foreground process
void statusCommand() {
	if (exitStatus == 0) {
		printf("exit status: %d\n", exitStatus);
		fflush(stdout);
	}
	else {
		printf("terminated by signal: %d\n", exitStatus);
		fflush(stdout);
	}
}


//command to change working directory
void cdCommand(char **commandsArray) {
	char *pathName;
	//change to desired directory
	if (commandsArray[1] != NULL) {
		pathName = commandsArray[1];
		chdir(pathName);
	}
	//no args given means use the HOME variable
	else {
		pathName = getenv("HOME");
		chdir(pathName);
	}
}


//function to split commands into array of chars
int splitString(char *input, char **commands) {
	int i = 0;
	char *temp = malloc(8 * sizeof(char));
	//use strtok to delimit the commands by space
	temp = strtok(input, " ");
	commands[i] = temp;
	i++;
	//loop through input to delimit each word and save it to the array
	while (temp != NULL) {
		temp = strtok(NULL, " ");
		commands[i] = temp;
		i++;
	}
	free(temp);
	//return the amount of commands in the command line
	return i - 1;
}


//function that parses commands to redirect stdin/stdout before forking
void parseCommands(char **commandArray, int numCommands) {
	int i = 0, j = 0, saver = 1, out, in, devNull, outFlag = 0;
	char **argvArray = malloc(MAX_COMMANDS * sizeof(char *));
	//check for < or > and redirect output/input accordingly
	for (i; i < numCommands; i++) {
		//redirect stdout to specified file
		if (strcmp(commandArray[i], ">") == 0) {
			saver = 0;
			outFlag = 1;
			out = open(commandArray[i + 1], O_WRONLY | O_CREAT, 0666);
			dup2(out, 1);
		}
		//redirect stdin from specified file
		else if (strcmp(commandArray[i], "<") == 0) {
			saver = 0;
			in = open(commandArray[i + 1], O_RDONLY);
			if (in == -1) {
				//print error if file is invalid and kill child
				printf("Error opening file\n");
				exitStatus = 1;
				raise(SIGTERM);
				return;
			}
			else {
				dup2(in, 0);
			}
		}
		else if ((strcmp(commandArray[i], "&") == 0) && !outFlag) {
			//direct output of background process to dev null if no file specified otherwise
			devNull = open("/dev/null", O_WRONLY);
			dup2(devNull, 1);
			saver = 0;
		}
		else if (saver) {
			argvArray[j] = commandArray[i];
			j++;
		}
	}
	//exec command or print error if failed
	if (execvp(argvArray[0], argvArray) == -1) {
		printf("%s is not a command\n", argvArray[0]);
		fflush(stdout);
		exitStatus = 1;
	}
}


//check background processes for completion
void checkBackground() {
	pid_t finishedChild = -5;
	int i;

	//check if any background children have finished
	finishedChild = waitpid(-1, &exitStatus, WNOHANG);
	if (finishedChild > 0) {
		printf("Background pid %d is done: ", finishedChild);
		fflush(stdout);
		if (exitStatus == 0) {
			printf("exit value %d\n", exitStatus);
			fflush(stdout);
		}
		else {
			printf("terminated with signal %d\n", exitStatus);
			fflush(stdout);
		}
	}

	//need to remove id from process id list
	for (i = 0; i < numProcs; i++) {
	for (i = 0; i < numProcs; i++) {
		if (finishedChild == processIds[i]) {
			processIds[i] = processIds[i + 1];
			processIds[i + 1] = 0;
			numProcs--;
		}
	}
}


//checks user input for desired command and runs said command
int runCommands(char *input) {
	char *tempInput = malloc(2 * sizeof(char)), **commandsArray = malloc(MAX_COMMANDS * sizeof(char*));
	int i = 0, numCommands = 0;
	pid_t tempChild = -5, finishedChild = -5;
	//take newline off end of input
	strtok(input, "\n");
	//parse commands and put them in an array
	numCommands = splitString(input, commandsArray);

	//check for built in commands
	if (strcmp(commandsArray[0], "exit") == 0) {
		free(tempInput);
		free(commandsArray);
		return exitCommand();
	}

	//status
	else if (strcmp(commandsArray[0], "status") == 0) {
		statusCommand();

		free(tempInput);
		free(commandsArray);
		return 1;
	}

	//cd
	else if (strcmp(commandsArray[0], "cd") == 0) {
		cdCommand(commandsArray);

		free(tempInput);
		free(commandsArray);
		return 1;
	}

	//fork, parse string and run commands
	else {
		tempChild = fork();
		//0 equates to being in the child process
		if (tempChild == 0) {
			parseCommands(commandsArray, numCommands);
		}
		//still within the parent process
		else if (tempChild > 0) {
			//main process doesn't need to wait if the ampersand is present at the end
			if (strcmp(commandsArray[numCommands - 1], "&") == 0 && ctrlZcheck == 0) {
				printf("background pid is: %d\n", tempChild);
				fflush(stdout);
				//save background proc to array
				processIds[numProcs] = tempChild;
				numProcs++;

				free(tempInput);
				free(commandsArray);
				return 1;
			}
			else {
				//foreground process is ran and proc id is recorded
				foregroundProc = tempChild;
				waitpid(tempChild, &exitStatus, 0);

				free(tempInput);
				free(commandsArray);
				return 1;
			}
		}
	}
}


//function to replace $$ with process id
char *replaceProc(char *userInput) {
	char *ptr = userInput, idNum[5], temp[64];
	//replace instances of $$ with mainproc
	strtok(ptr, "\n");
	sprintf(idNum, "%d", mainProc);

	//loop through getting each occurence of $$
	while (ptr = strstr(userInput, "$$")) {
		//save up until first occurence into temp string
		strncpy(temp, userInput, ptr - userInput);
		//add null terminator for strcat
		temp[ptr - userInput] = '\0';
		//replace the $$ with the id and append rest of string after to temp
		strcat(temp, idNum);
		strcat(temp, (ptr + strlen("$$")));
		//copy temp into original string and find next occurence of $$
		strcpy(userInput, temp);
		ptr++;
	}
	//return result of changed string
	return userInput;
}


//gets the user's input with correct : format
int getUserInput() {
	char *userInput = malloc(MAX_CHARS * sizeof(char));
	size_t inputBuffer = MAX_CHARS;
	int exiter, ignoreLine = 0;


	//starting prompt
	printf(": ");
	fflush(stdout);
	if (getline(&userInput, &inputBuffer, stdin) == -1) {
		printf("error: getline broke\n");
		fflush(stdout);
	}
	else {
		userInput = replaceProc(userInput);
		//ignores comments and blank lines using flag variable
		if ((strcmp(userInput, "\n") == 0) || (strstr(userInput, "#") != NULL)) {
			ignoreLine = 1;
		}
		//else run the desired command
		else if(!ignoreLine) {
			exiter = runCommands(userInput);
		}
		//check for finished background processes
		checkBackground();
	}
	free(userInput);
	return exiter;
}


//functions for signal handling
//sigtstp handler
void sigtstpHandle(int signum) {
	char *message, *message1;
	if (ctrlZcheck == 0) {
		message = "\nEntering foreground - only mode\n";
		write(STDOUT_FILENO, message, strlen(message));
		fflush(stdout);
		ctrlZcheck = 1;
	}
	else if (ctrlZcheck == 1) {
		message1 = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message1, strlen(message1));
		fflush(stdout);
		ctrlZcheck = 0;
	}
}


//sigint handler
void sigintHandle(int signum) {
	char *message = " Terminated by signal 2\n";
	kill(foregroundProc, SIGTERM);
	write(STDOUT_FILENO, message, strlen(message));
	fflush(stdout);
}


//main function
int main() {
	//sigaction struct declarations
	struct sigaction SIGTSTP_action = { 0 }, SIGINT_action = { 0 };

	//ctrlz signal catch
	SIGTSTP_action.sa_handler = sigtstpHandle;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;

	//ctrlc signal catch
	SIGINT_action.sa_handler = sigintHandle;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;

	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGINT, &SIGINT_action, NULL);

	//carry out shell execution 
	processIds = (pid_t *)calloc(10, sizeof(pid_t));
	mainProc = getpid();
	//exiter is only set to 0 if returned from the exit command
	int exiter = 1;

	//loop to keep executing commands until otherwise stated
	while (exiter) {
		exiter = getUserInput();
	}
	return 0;
}