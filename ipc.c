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
char shared_memory[SHM_SIZE];


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
    if (semaphore_id == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // init the semaphore set with 0's - blocked state
    for (int i = 0; i < num_semaphores; i++) {
        if (semctl(semaphore_id, i, SETVAL, 0) == -1) {
            perror("semctl failed");
            exit(EXIT_FAILURE);
        }
    }
    return semaphore_id;
}

void wait_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, -1, 0}; // Wait (P operation)
    if (semop(semaphore_id, &sb, 1) == -1) {
        perror("semop wait failed");
        exit(EXIT_FAILURE);
    }
}


void signal_semaphore(int semaphore_id, int index) {
    struct sembuf sb = {index, 1, 0}; // Signal (V operation)
    if (semop(semaphore_id, &sb, 1) == -1) {
        perror("semop signal failed");
        exit(EXIT_FAILURE);
    }
}


// clear used semaphores
void remove_semaphores(int semaphore_id) {
    if(semctl(semaphore_id, 0, IPC_RMID) == -1) {
        perror("Semaphores removal failed\n");
        exit(EXIT_FAILURE);
    }
}
void handle_termination(int sig) {
    printf("Child %d received termination signal. Exiting...\n", getpid());
    shmdt(shared_memory); // Detach shared memory
    exit(EXIT_SUCCESS);
}


void child_process(int sems_id, int semaphore_index, char *shared_mem) {
    signal(SIGTERM, handle_termination); // Set up termination handler

    while (TRUE) {
        wait_semaphore(sems_id, semaphore_index); // Wait for a message

        printf("Child %d received message: %s\n", getpid(), shared_mem);

    }
}


// ./ipc config_3_100.txt mobydick.txt 3
int main(int argc, char *argv[]) {

    // usage error
    if(argc != 4) {
        printf("Usage: %s <config_file> <text_file> <max_processes>\n", argv[0]);
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
    // for(int i = 0; i < max_children; i++) {

    // }

    //create shared memory segment
    int shm_id = shmget(IPC_PRIVATE, sizeof(shared_memory), IPC_CREAT | 0666);

    //attach shared memory to parent process
    char *shared_mem = shmat(shm_id, NULL, 0);

    // keep track of active children
    ChildProcess children[max_children];

    // this will be the index of children[] && semaphores
    int active_children = 0;

    //global to -1 - random thought
    while (TRUE) {
        printf("Tick %d:\n", global_time);

        for (int i = 0; i < num_commands; i++) {
            if (strcmp(commands[i]->status, "EXIT") == 0 && commands[i]->timestamp == global_time) {
                for (int j = 0; j < active_children; j++) {
                    kill(children[j].pid, SIGTERM); // Send termination signal
                    waitpid(children[j].pid, NULL, 0); // Wait for child to exit
                }

                // Cleanup
                shmdt(shared_mem);
                shmctl(shm_id, IPC_RMID, NULL);
                remove_semaphores(sems_id);
                freeSpace(&commands, num_commands);
                printf("Exiting parent process.\n");
                exit(EXIT_SUCCESS);
            }

            if (strcmp(commands[i]->status, "S") == 0 && commands[i]->timestamp == global_time) {
                if (active_children < max_children) {
                    int child_index = active_children;
                    pid_t pid = fork();

                    if (pid == 0) {
                        // In child process
                        child_process(sems_id, child_index, shared_mem);
                    } else if (pid > 0) {
                        // In parent process
                        children[child_index].pid = pid;
                        children[child_index].semaphore_index = child_index;
                        children[child_index].creation_command = commands[i];
                        active_children++;
                    }
                }
            }

            if (strcmp(commands[i]->status, "T") == 0 && commands[i]->timestamp == global_time) {
                for (int j = 0; j < active_children; j++) {
                    if (children[j].creation_command == commands[i]) {
                        kill(children[j].pid, SIGTERM);
                        waitpid(children[j].pid, NULL, 0);
                        printf("Terminated child process with PID: %d\n", children[j].pid);
                        active_children--;

                        // Shift remaining children down
                        for (int k = j; k < active_children; k++) {
                            children[k] = children[k + 1];
                        }
                        break;
                    }
                }
            }
        }

        if (active_children > 0) {
            int random_child = rand() % active_children;
            char *msg = getRandomLine(argv[2]);
            strcpy(shared_mem, msg);
            signal_semaphore(sems_id, children[random_child].semaphore_index);
        }

        global_time++;
    }


    return 0;
}
