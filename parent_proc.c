#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODOs
// need to write a function to handle the cf file
// need to write a function get random line from the text file
// need to write a function to spawn children process
// need to write a functuion to create shared memory segment

#define MAX_COMMAND_SIZE 15

//parse config file as command for the parent
typedef struct Command {
    int timestamp;      // Timestamp (21, 22, 23, ...)
    char cid[3];        // Child Process ID (C1, C2, C3, ...)
    char status;        // S (Spawn), T (Termination), E (Exit)
} Command;


// read config file and
void getCommands(char *filename) {
    //open file for reading
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error opening file\n");
        exit(EXIT_FAILURE);
    }

    char line[MAX_COMMAND_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s\n", line);
    }


    fclose(file);
    return;
}



char* getRandomLine(char *filename) {
    //open file for reading
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        exit(EXIT_FAILURE);
    }



}


// todo's
int spawn_child(){ return 0;}
int terminate_child() { return 0;}


int main() {

    // Command **commands = malloc(10 * sizeof(Command));

    // getCommands("config_3_100.txt");

    // printf("%s\n", getRandomLine("mobydick.txt"));

    return 0;
}
