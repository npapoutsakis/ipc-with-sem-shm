#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>        //shared memory
#include <sys/sem.h>        //semaphores
#include <sys/wait.h>

// shared memory Size
#define SHM_SIZE 1024
#define TRUE 1
#define FALSE 0

// global time variable
int global_time = 0;

// create a buffer for shared memory
char buf[SHM_SIZE];

//parse config file as command for the parent
typedef struct Command {
    int timestamp;        // Timestamp (21, 22, 23, ...)
    char cid[5];          // Child Process ID (C1, C2, C3, ...)
    char status[5];       // S (Spawn), T (Termination), E (Exit)
} Command;

// Simple structure to keep useful info about a child process upon creation
typedef struct ChildProcess {
    pid_t pid;                      // Child Process ID
    int semaphore_index;            // semaphore assigned to the specific child
    Command *creation_command;      // creation commands executed from parent
} ChildProcess;

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
        if(i == line_num && strlen(buffer) > 1)
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
        free(cemp[i]);
    }
    free(cemp); // Free the array of pointers
}

// Sleep in ms
void sleep_in_ms(int ms) {
    usleep(ms * 1000);
}

// Initialize a semaphore set of size num_semaphores or M
int create_semaphores(int num_semaphores) {
    // create the semaphore set
    int semaphore_id = semget(IPC_PRIVATE, num_semaphores, IPC_CREAT | 0666);
    if (semaphore_id == -1) {
        printf("semget failed");
        exit(EXIT_FAILURE);
    }

    // init the semaphore set with 0's - blocked state
    for (int i = 0; i < num_semaphores; i++) {
        if (semctl(semaphore_id, i, SETVAL, 0) == -1) {
            printf("semctl failed");
            exit(EXIT_FAILURE);
        }
    }
    return semaphore_id;
}

// Wait operation
void wait_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, -1, 0};
    if (semop(semaphore_id, &sb, 1) == -1) {
        printf("semop wait failed");
        exit(EXIT_FAILURE);
    }
}

// Signal operation
void signal_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, 1, 0};
    if (semop(semaphore_id, &sb, 1) == -1) {
        printf("semop signal failed");
        exit(EXIT_FAILURE);
    }
}

// clear used semaphores
void remove_semaphores(int semaphore_id) {
    if(semctl(semaphore_id, 0, IPC_RMID) == -1) {
        printf("Semaphores removal failed\n");
        exit(EXIT_FAILURE);
    }
}

// assign defaults
void initializeChildren(ChildProcess* children, int M) {
    for (int i = 0; i < M; i++) {
        children[i].pid = -1;
        children[i].semaphore_index = -1;
        children[i].creation_command = NULL;
    }
}

// Return the first available semaphore index
int getAvailableSemaphore(int *free_array, int M) {    
    for(int index = 0; index < M; index++) {
        // if free, return index
        if(free_array[index] == 0) {
            return index;
        }
    }
}

// Main
int main(int argc, char *argv[]) {

    // usage error
    if(argc != 4) {
        printf("Usage: %s <config_file> <text_file> <M>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Randomness
    srand(time(NULL));

    // Get commands from config file
    Command **commands = NULL;
    int num_commands = 0;
    int max_children = atoi(argv[3]);

    // Parsing commands in an array of (struct) Command
    parseCommands(argv[1], &commands, &num_commands);

    // create semaphores
    int sems_id = create_semaphores(max_children);

    // array to show which semaphore is available
    int availableSemaphore[max_children];
    for (int i = 0; i < max_children; i++) {
        availableSemaphore[i] = 0;              // 0 (Free) or 1 (Occupied)  
    }
    
    //create shared memory segment
    int shm_id = shmget(IPC_PRIVATE, sizeof(buf), IPC_CREAT | 0666);

    //attach shared memory to parent process
    char *shared_mem = shmat(shm_id, NULL, 0);

    // this will be the index of children[] && semaphores
    int active_children = 0;

    // index to hold the current command
    int command_index = 0;

    // keep track of active children
    ChildProcess children[max_children];
    initializeChildren(children, max_children);

    while (TRUE) {

        // Get current command
        Command *current = commands[command_index];

        // EXIT STATUS
        if (strcmp(current->status, "EXIT") == 0 && current->timestamp == global_time) {
            
            printf("EXIT STATUS\n");
            //if exit comes and some children are running, just terminate them wait() and then exit
            for (int i = 0; i < max_children; i++) {
                
                if (children[i].pid != -1) {
                    // signal the child to exit
                    strcpy(shared_mem, "SIGTERM");
                    signal_semaphore(sems_id, children[i].semaphore_index);

                    int status;
                    waitpid(children[i].pid, &status, 0);
                        
                    int active_time = global_time - children[i].creation_command->timestamp;
                    printf("\nChild %s was alive -> Terminate. \n\tMessages received: %d\n\tActive time(tick): %d\n", children[i].creation_command->cid, WEXITSTATUS(status) - 1, active_time);
                }

            }

            // Cleanup
            shmdt(shared_mem);
            shmctl(shm_id, IPC_RMID, NULL);
            remove_semaphores(sems_id);
            freeSpace(&commands, num_commands);
            exit(EXIT_SUCCESS);
        }

        // SPAWN STATUS
        else if (strcmp(current->status, "S") == 0 && current->timestamp == global_time) {

            // only when a 'compatible' command is found, move to the next one
            command_index++;

            // semaphore available index & only that
            int available_semaphore_index = getAvailableSemaphore(availableSemaphore, max_children);


            // get available children[] index
            int child_index = -1;
            for (int i = 0; i < max_children; i++) {
                if (children[i].pid == -1) {
                    child_index = i;
                    break;
                }
            }
        
            // create the new child
            pid_t pid = fork();
            if (pid == 0) {
                // child process
                int creation_time = global_time;
                int message_counter = 0;

                while(TRUE) {
                    // the child here will lock & wait for the semaphore value to be incremented by the parent
                    wait_semaphore(sems_id, available_semaphore_index);

                    // attach to shared memory segment
                    char *data = shmat(shm_id, NULL, 0);

                    // If 'SIGTERM' is received, exit the child process
                    if (strcmp(data, "SIGTERM") == 0) {
                        // Detach from shared memory
                        shmdt(data);
                        exit(message_counter);
                    }

                    printf("Child %s received msg: %s\n", current->cid, data);
                    message_counter++;

                    
                    if (shmdt(data) == -1) {
                        printf("Error detaching from shared memory");
                        exit(EXIT_FAILURE);
                    }
                }

            }
            else if (pid > 0) {
                // the parent has to update the stats. Increase the array of active children

                // store active child on array
                children[child_index].pid = pid;
                children[child_index].semaphore_index = available_semaphore_index;
                children[child_index].creation_command = current;

                // mark semaphore as occupied
                availableSemaphore[available_semaphore_index] = 1;                    //occupied
                active_children++;
            }
            else {
                printf("Fork failed\n");
                exit(EXIT_FAILURE);
            }
        }

        // TERMINATION STATUS
        else if (strcmp(current->status, "T") == 0 && current->timestamp == global_time) {

            // upon termination the child process should not continue with the rest for the code
            // it will terminate/exit only when the parent process sends sigterm on the child

            for (int i = 0; i < max_children; i++) {

                if(children[i].pid != -1 && strcmp(current->cid, (children[i].creation_command)->cid) == 0) {

                    // kill(children[i].pid, SIGTERM); ? in both cases the child will exit

                    // Method 1: Send SIGTERM to the shared memory segment and signal the child to see it. Then termination is occuried
                    // Method 2: Send SIGTERM via the kill() function and terminate the child process.

                    // signal the child to exit - IPC move
                    strcpy(shared_mem, "SIGTERM");
                    signal_semaphore(sems_id, children[i].semaphore_index);

                    int status;
                    waitpid(children[i].pid, &status, 0);
                        
                    int active_time = global_time - children[i].creation_command->timestamp;
                    printf("Child %s terminated. \n\tMessages received: %d\n\tActive time(tick): %d\n", current->cid, WEXITSTATUS(status) - 1, active_time);

                    // Make the semaphore available again
                    availableSemaphore[children[i].semaphore_index] = 0;

                    children[i].pid = -1;
                    children[i].semaphore_index = -1;
                    children[i].creation_command = NULL;

                    // Decrement active children number
                    active_children--;
                    break;
                }

            }

            // only a 'compatible' command is found, move to the next one
            command_index++;
        }


        // Sending randon line to a random child
        if(active_children > 0) {
            // Generate a random index from the list of active children indices
            int valid_child_found = FALSE;
            int random_index = -1;

            // get a valid index for choosing a child
            while (!valid_child_found) {
                random_index = rand() % max_children;
                if (children[random_index].pid != -1) {
                    valid_child_found = TRUE;
                }
            }

            int valid_random_child = random_index;

            // may contain lines with 0 characters
            char *message = getRandomLine(argv[2]);

            // Copy the random line to the shared memory segment
            strcpy(shared_mem, message);
            // printf("Parent signals %s msg: %s", (children[valid_random_child].creation_command)->cid, message);

            // we have to signal the semaphore of that specific child
            signal_semaphore(sems_id, children[valid_random_child].semaphore_index);
            free(message);
        }

        // Each iteration is a simulation 'tick'
        global_time++;
        sleep_in_ms(50);
    }

}
