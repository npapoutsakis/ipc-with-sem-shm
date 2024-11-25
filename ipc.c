#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>        //shared memory
#include <sys/sem.h>        //semaphores

// shared memory Size
#define SHM_SIZE 1024
#define TRUE 1
#define FALSE 0

// global time variable
int global_time = 0;

//parse config file as command for the parent
typedef struct Command {
    int timestamp;        // Timestamp (21, 22, 23, ...)
    char cid[5];          // Child Process ID (C1, C2, C3, ...)
    char status[5];       // S (Spawn), T (Termination), E (Exit)
} Command;


// create a structure to hold the shared memory segment
typedef struct SharedMemory {
    char text[SHM_SIZE];      // For storing message
    int child_id;             // Child ID to whom message is sent
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

int create_semaphores(int num_semaphores) {
    // create the semaphore set
    int semaphore_id = semget(IPC_PRIVATE, num_semaphores, IPC_CREAT | 0666);

    // init the semaphore set with 1's
    for (int i = 0; i < num_semaphores; i++) {
        semctl(semaphore_id, i, SETVAL, 1);
    }
    return semaphore_id;
}

// clear used semaphores
void remove_semaphores(int semaphore_id) {
    if (semctl(semaphore_id, 0, IPC_RMID) == -1) {
        printf("Semaphores removal failed\n");
        exit(EXIT_FAILURE);
    }
    return;
}

void wait_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, -1, 0}; // Wait (P operation)
    semop(semaphore_id, &sb, 1);
    return;
}

void signal_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, 1, 0}; // Signal (V operation)
    semop(semaphore_id, &sb, 1);
    return;
}

// ./ipc config_3_100.txt mobydick.txt 3
int main(int argc, char *argv[]) {

    // usage error
    if(argc != 4){
        printf("Usage: %s <config_file> <text_file> <max_processes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    // Get commands from config file
    Command **commands = NULL;
    int num_commands = 0;
    int running = TRUE;
    int max_children = atoi(argv[3]);

    // Parsing commands in an array of (struct) Command
    parseCommands(argv[1], &commands, &num_commands);

    // create semaphores for synchronization
    int sems_id = create_semaphores(max_children);

    //create shared memory segment
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);

    //attach shared memory to parent process
    SharedMemory *shared_mem = shmat(shm_id, NULL, 0);

    // keep track of active children
    pid_t child_ids[max_children];
    int active_children = 0;

    while(running) {
        // Printing the 'tick' of simulation
        printf("Tick: %d\n", global_time);

        for(int i = 0; i < num_commands; i++) {
            // Exit status found, termination program
            if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "EXIT") == 0) {
                printf("Global time is %d and command is %s -> Program Terminated!\n", global_time, commands[i]->status);
                running = FALSE;
            }
            else if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "S") == 0)
            {
                // Spawn Process Found
                // assign a semaphore to the process
                // assign a shared memory segment to the process

                // child_index = 0; at first
                int child_index = active_children;

                pid_t pid = fork();

                if(pid == 0) {
                    // child process
                    // assign a semaphore to that process

                    SharedMemory *shared_mem = shmat(shm_id, NULL, 0);
                    int messages_received = 0;

                    while(TRUE) {
                        wait_semaphore(sems_id, child_index);
                        printf("Child %d received: %s\n", child_index, shared_mem->text);
                        messages_received++;
                    }

                    shmdt(shared_mem);
                    exit(EXIT_SUCCESS);
                }
                else if (pid > 0) {
                    child_ids[active_children] = pid;
                    active_children++;
                    printf("Child %d spawned at time %d\n", child_index, global_time);
                }
                else {
                    printf("Failed to spawn child process\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            else if(commands[i]->timestamp == global_time && strcmp(commands[i]->status, "T") == 0)
            {
                int child_index = active_children - 1;
                if(child_ids[child_index] > 0) {
                        kill(child_ids[child_index], SIGTERM);
                        printf("Terminated child %d\n", child_index + 1);
                        child_ids[child_index] = 0;
                        active_children--;
                }
                break;
            }

        }

        // random data will be sent on each iteration to the active children
        // 1. attach to shared memory
        // 2. write data to shared memory
        // 3. signal to children that data is available
        // send_random_data();

        if(active_children > 0) {
            int random_child = rand() % active_children;
            char *random_line = getRandomLine("mobydick.txt");

            strcpy(shared_mem->text, random_line);
            shared_mem->child_id = random_child;

            // signal i-th semaphore
            signal_semaphore(sems_id, random_child);


            printf("Sent data to child %d: %s", random_child, random_line);
            free(random_line);
        }


        // Each iteration of the loop is a "tick" of the simulation
        global_time++;
        // sleep_ms(1000);
        // sleep(1);
    }



    // Clean Up
    shmctl(shm_id, IPC_RMID, NULL);
    remove_semaphores(sems_id);
    freeSpace(&commands, num_commands);
    return 0;
}
