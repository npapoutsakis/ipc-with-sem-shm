#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


// TODOs
// need to write a function to handle the cf file
// need to write a function to spawn child process
// need to write a function to create shared memory segment
// need to write a function to create semaphores
// need to be able to handle active child processes & semaphores

#define MAX_COMMAND_SIZE 15


// global time variable
int global_time = 0;

//parse config file as command for the parent
typedef struct Command {
    int timestamp;      // Timestamp (21, 22, 23, ...)
    char cid[5];          // Child Process ID (C1, C2, C3, ...)
    char status[5];       // S (Spawn), T (Termination), E (Exit)
} Command;


// Get a non empty random line from text file
char *getRandomLine(char *filename){
    // Open file for reading
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Randomness
    srand(time(NULL));
    int line_num = rand() % 1000;

    // line buffer
    char *buffer = (char *)malloc(1000 * sizeof(char));
    if(buffer == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while(fgets(buffer, 1000, file) != NULL) {
        if (i == line_num) {
            break;
        }
        i++;
    }

    fclose(file);
    return buffer;
}


// Parse commands from the config file
Command *parseCommand(char *filename) {

    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }




    return NULL;
}














int main(int argc, char *argv[]) {
    Command **commands = (Command *)malloc(2 * sizeof(Command));
    return 0;
}
