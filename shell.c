#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <wait.h>
#include <errno.h>

#define BUFFER_SIZE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMANDS 10

static char inputBuffer[BUFFER_SIZE];
static char commands[MAX_COMMANDS][BUFFER_SIZE];
int countCommands = 0; /* increments for each command that is added */
int mostRecentCommandIndex = 0; /* index of the most recent command to keep track of where you are in the array */

/* Returns the index of the command that should be executed */
int promptUntilCommandReceived() {
    int cmd,  /* the number of the command that the user want  */
    length,   /* # of characters in the command line */
    i, j;        /* iteration through the command list commands */
    int validCommandFound = 0;
    
    while (validCommandFound == 0) {
        printf("Your command?\n");
        fflush(0);
        length = read(STDIN_FILENO, inputBuffer, BUFFER_SIZE);
        if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
        if (length < 0) {
            perror("error reading the command");
	        exit(-1);           /* terminate with error code of -1 */
        }
        inputBuffer[length-1] = '\0';
        length = strlen(inputBuffer);
        if (length < 1) {
            printf("Sorry, that's not a valid command. Please try again.\n");
            fflush(0);
        } else {            /* length >= 1 */
            if (inputBuffer[0] == 'r') {
                if (length <= 1) {
                    return mostRecentCommandIndex;
                } else if (length == 3) {
                    char cmdLetter = inputBuffer[2];
                    j = 0;
                    i = mostRecentCommandIndex;

                    /* loop through the commands and check if a command matches the letter inputted */
                    while (j < MAX_COMMANDS && validCommandFound == 0) {
                        if (i < 0) {
                            i += MAX_COMMANDS;
                        }
                        if (cmdLetter == commands[i][0]) {
                            return i;
                            validCommandFound = 1;
                        }
                        i--;
                        j++;
                    }
                }
            }
            if (validCommandFound == 0) {
                printf("Sorry, that's not a valid command. Please try again.\n");
                fflush(0);
            }
        }
    }
}

/* Prints the possible commands (the most recent MAX_COMMANDS commands) */
void printCommands() {
    int i, j;
    printf("Most recent %d commands:\n", MAX_COMMANDS);
    fflush(0);
    i = mostRecentCommandIndex;
    while (j < MAX_COMMANDS && j < countCommands) {
        if (i < 0) {
            i += MAX_COMMANDS;
        }
        printf("Command %d: ", countCommands - j);
        fflush(0);
        write(STDOUT_FILENO, commands[i], strlen(commands[i]));
        i--;
        j++;
    }
}

void sigSetup(char inputBuffer[], char *args[], int *background);

/* processes the command given within the signal handler */
void processCommand(int cmdInd) {
    int background = 0;              /* equals 1 if a command is followed by '&' */
    char *args[BUFFER_SIZE/2+1]; /* command line (of 80) has max of 40 arguments */
    strcpy(inputBuffer, commands[cmdInd]);
    sigSetup(inputBuffer, args, &background);
    int pid = fork();
    int *returnCode;
    /* maybe need to add more from setup? */
    if (pid == 0) {
        printf("Running: %s...\n", inputBuffer);
        fflush(0);
        execvp(inputBuffer, args);
        printf("Command not recognized.\n");
        fflush(0);
        exit(-1);
    } else if (pid < 0) {
        printf("Fork! It Failed.\n");
        fflush(0);
    } else {
        if (background == 0) {
            waitpid(pid, returnCode, 0);
        }
    }
    printf("COMMAND->");
    fflush(0);
}

/* the signal handler function */
void handle_SIGINT() {
    int i = 0;
    printf("\nCaught <ctrl><c>.\n");
    fflush(0);
    printCommands();
    int cmd = promptUntilCommandReceived();
    processCommand(cmd);
}

/* adds the command in inputBuffer to the next spot in the commands array */
void addCommandToArray() {
    countCommands++;
    mostRecentCommandIndex++;
    if (mostRecentCommandIndex >= MAX_COMMANDS) {
        mostRecentCommandIndex -= MAX_COMMANDS;
    }
    strcpy(commands[mostRecentCommandIndex], inputBuffer);
}

/**
 * sigSetup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a 
 * null-terminated string.
 */
void sigSetup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    ct = 0;

    length = strlen(inputBuffer);

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0) {
        perror("error reading the command");
	    exit(-1);           /* terminate with error code of -1 */
    }
    inputBuffer[length] = '\0';

    /* examine every character in the inputBuffer */
    for (i = 0; i < length; i++) { 
        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;
                
            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];     
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            case '&':
                *background = 1;
                inputBuffer[i] = '\0';
                break;
                
            default :             /* some other character */
                if (start == -1)
                    start = i;
	    }
    }
    args[ct] = NULL; /* just in case the input line was > 80 */
} 

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a 
 * null-terminated string.
 */
void setup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    ct = 0;
    
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, BUFFER_SIZE);
    printf("\n%s\n", inputBuffer);
    fflush(0);

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0) {
        perror("error reading the command");
	    exit(-1);           /* terminate with error code of -1 */
    }
    inputBuffer[length] = '\0';
    addCommandToArray();

    /* examine every character in the inputBuffer */
    for (i = 0; i < length; i++) { 
        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;
                
            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];     
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            case '&':
                *background = 1;
                inputBuffer[i] = '\0';
                break;
                
            default :             /* some other character */
                if (start == -1)
                    start = i;
	    } 
    }    
    args[ct] = NULL; /* just in case the input line was > 80 */
} 

int main(void)
{
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[BUFFER_SIZE/2+1]; /* command line (of 80) has max of 40 arguments */
    /* set up the signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    handler.sa_flags = SA_RESTART;
    sigaction(SIGINT, &handler, NULL);
    
    while (1) {            /* Program terminates normally inside setup */
        background = 0;
        printf("COMMAND->");
        fflush(0);
        setup(inputBuffer, args, &background);       /* get next command */

        /* the steps are:
        (1) fork a child process using fork() 
        (2) the child process will invoke execvp()
        (3) if background == 0, the parent will wait,
            otherwise returns to the setup() function. */
        int pid = fork();
        int *returnCode;
        if (pid == 0) {
            execvp(inputBuffer, args);
            printf("Command not recognized.\n");
            exit(-1);
        } else if (pid < 0) {
            printf("Fork! It Failed.\n");
        } else {
            if (background == 0) {
                waitpid(pid, returnCode, 0);
            }
        }
    }
}
