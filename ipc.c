#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>        //shared memory
#include <sys/sem.h>        //semaphores

// Shared Memory Size
#define SHM_SIZE 1024
#define TRUE 1
#define FALSE 0

// TODOs
// need to write a function to spawn child process
// need to write a function to create shared memory segment
// need to write a function to create semaphores
// need to be able to handle active child processes & semaphores

// global time variable
int global_time = 0;

//parse config file as command for the parent
typedef struct Command {
    int timestamp;        // Timestamp (21, 22, 23, ...)
    char cid[5];          // Child Process ID (C1, C2, C3, ...)
    char status[5];       // S (Spawn), T (Termination), E (Exit)
} Command;

typedef struct SharedMemory {
    char text[SHM_SIZE];  // For storing message
    int child_id;         // Child ID to whom message is sent
} SharedMemory;


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
    char *buffer = (char *)malloc(1024 * sizeof(char));
    if(buffer == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while(fgets(buffer, 1024, file) != NULL) {
        if(i == line_num)
            break;
        i++;
    }

    fclose(file);
    return buffer;
}


// Parse commands from the config file
void parseCommands(char *filename, Command ***commands, int *num_commands) {
    //open file for reading
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // count total command lines
    char c;
    int total_commands = 0;
    while((c = fgetc(file)) != EOF) {
        if(c == '\n') {
            total_commands++;
        }
    }

    // reset file pointer
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the array of Command pointers
    *commands = (Command **)malloc(total_commands * sizeof(Command *));
    if (*commands == NULL) {
        printf("Error allocating memory for command array\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < total_commands; i++) {
        (*commands)[i] = (Command *)malloc(sizeof(Command));

        if((*commands)[i] == NULL) {
            printf("Error allocating memory for Command object\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        fscanf(file, "%d %s %s", &((*commands)[i]->timestamp), (*commands)[i]->cid, (*commands)[i]->status);

        if (strcmp((*commands)[i]->cid, "EXIT") == 0) {
            strcpy((*commands)[i]->status, (*commands)[i]->cid);
            strcpy((*commands)[i]->cid, "");
        }
    }

    fclose(file);
    *num_commands = total_commands;
    return;
}



// Spawns a new child process
void spawn_child(){

}




void terminate_child(pid_t cid, int timestamp) {

}




void send_random_data(){



    return;
}



void init_semaphores(int M) {
    // create semaphore set
    int sem_id = semget(IPC_PRIVATE, M, IPC_CREAT | 0666);
    if(sem_id == -1) {
        printf("Error creating semaphore\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < M; i++) {
        semctl(sem_id, i, SETVAL, 1);  // Initialize semaphores to 1
    }
    return;
}


// Free Command Array
void freeSpace(Command ***commands, int total_commands) {
    // Free the allocated memory
    Command **cemp = *commands;
    for (int i = 0; i < total_commands; i++) {
        free(cemp[i]); // Free each Command object
    }
    free(cemp); // Free the array of pointers
}

// Sleep in ms
void sleep_ms(int ms) {
    usleep(ms * 1000);
}



// ./ipc. config_3_100.txt mobydick.txt 3
int main(int argc, char *argv[]) {

    // Get commands from config file
    Command **commands = NULL;
    int num_commands = 0;
    int running = TRUE;
    int active_children = 0;

    // Parsing commands in an array of (struct) Command
    parseCommands(argv[1], &commands, &num_commands);

    while(running) {
        // Printing the 'tick' of simulation
        printf("Tick: %d\n", global_time);

        for(int i = 0; i < num_commands; i++) {
            // Exit status found, termination program
            if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "EXIT") == 0)
            {
                printf("Global time is %d and command is %s -> Program Terminated!\n", global_time, commands[i]->status);
                running = FALSE;
            }

            else if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "S") == 0)
            {
                // spawn_child(global_time);





                break;
            }

            else if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "T") == 0)
            {
                // terminate_child(global_time);





                break;
            }

        }




        // send_random_data();






        // Each iteration of the loop is a "tick" of the simulation
        global_time++;
        sleep_ms(10);
    }

    freeSpace(&commands, num_commands);
    return 0;
}
