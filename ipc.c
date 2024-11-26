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
    exit(EXIT_SUCCESS);
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

    //create shared memory segment
    int shm_id = shmget(IPC_PRIVATE, sizeof(buf), IPC_CREAT | 0666);

    //attach shared memory to parent process
    char *shared_mem = shmat(shm_id, NULL, 0);

    // keep track of active children
    ChildProcess children[max_children];

    // this will be the index of children[] && semaphores
    int active_children = 0;

    // index to hold the current command
    int command_index = 0;
    while (TRUE) {
        printf("Tick %d:\n", global_time);

        // check for active children
        // create a random index for that child
        // select the child ChildProcess[random_child]
        // get a random line from file
        // attach data to shared memory
        // upon completion, signal the [random's_child] semaphore
        // free(line)

        if(active_children > 0) {
            // generate a random index for the child
            int random_child = rand() % active_children;
            char *message = getRandomLine(argv[2]);

            // wait_semaphore(sems_id, random_child);

            strcpy(shared_mem, message);
            printf("Parent sent to: %s the msg: %s" , (children[random_child].creation_command)->cid, message);
            //we have to signal the semaphore of that specific child
            signal_semaphore(sems_id, random_child);
            free(message);
        }

        //hold the command until we get to the timestamp
        Command *current = commands[command_index];

        // EXIT STATUS
        if (strcmp(current->status, "EXIT") == 0 && current->timestamp == global_time) {

            //should we wait for the child to finish?
            //if exit comes and some children are running, just terminate them wait() and then exit
            for(int i = 0; i < active_children; i++) {
                // wait for the children to finish
                pid_t pid = wait(NULL); 
                if(pid == -1)
                    perror("wait failed!");
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
            // printf("%d %s %s\n", current->timestamp, current->cid, current->status);

            // only when a 'compatible' command is found, move to the next one
            command_index++;
            int child_index = active_children;

            pid_t pid = fork();
            if (pid == 0) {
                // child process

                // the child here will lock & wait for the semaphore value to be incremented by the parent
                while(TRUE) {
                    // printf("Hello from child %s! I'm waiting the semaphore!\n", current->cid);
                    wait_semaphore(sems_id, child_index);
                    // attach to shared memory segment
                    char *data = shmat(shm_id, NULL, 0);
                    printf("Child %s received msg: %s\n", current->cid, data);

                    shmdt(data);
                }
            }
            else if(pid > 0) {
                // the parent has to update the stats. Increase the array of active children
                active_children++;

                //init the structure before the fork is called or make the parent
                children[child_index].semaphore_index = child_index;
                children[child_index].creation_command = current;
                children[child_index].pid = pid;
            }
            else {
                // fork failed
                printf("Fork failed\n");
                exit(EXIT_FAILURE);
            }

        }
        // TERMINATION STATUS
        else if (strcmp(current->status, "T") == 0 && current->timestamp == global_time) {
            // printf("%d %s %s\n", current->timestamp, current->cid, current->status);

            // upon termination the child process should not continue with the rest for the code
            // it will terminate/exit only when the parent process calls kill on the child
            for (int i = 0; i < active_children; i++){
                if(!strcmp(current->cid, (children[i].creation_command)->cid)){
                    if (kill(children[i].pid, SIGKILL) == 0)
                        printf("Termination of %s with process id %d\n", current->cid, children[i].pid);
                }
            }
            // Decrement active children number
            active_children--;

            // only a 'compatible' command is found, move to the next one
            command_index++;
        }

        // Each iteration is a simulation 'tick'
        global_time++;
        sleep_ms(250);
    }

    return 0;
}
