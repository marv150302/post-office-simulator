#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "sportello.h"
#include "memory_handler.h"
#include "direttore.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {


    load_config("config/config.txt");//load configuration values
    printf("Starting simulation...\n");
    printf("Size of SportelloStatus: %lu bytes\n", sizeof(SportelloStatus));

	// Remove old shared memory before creating a new one
shmctl(shmget(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), 0666), IPC_RMID, NULL);
shmctl(shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), 0666), IPC_RMID, NULL);

    // Create and attach Sportello shared memory
    int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
    SportelloStatus *sportello = (SportelloStatus *)attach_shared_memory(shmid_sportello, "Sportello");



  	/*start all the different processes*/

    start_process("erogatore_ticket", "./bin/erogatore_ticket");

    /******** initialize counters(sportello) *****************+*/
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {
        sportello->service_type[i] = rand() % NUM_SERVICES;  // assign random service
        sportello->available[i] = 1;  // mark as available
        sportello->assigned_operator[i] = -1;  // no operator assigned yet
    }
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {

        //assigning the operator to the counter
        sportello->assigned_operator[i] = start_process("sportello", "./bin/sportello");
    }

    while (sportello->sportelli_ready != 1) {
    	printf("Waiting for sportelli to initialize...\n");
    	sleep(1);
	}
	printf("All sportelli initialized. Starting operatore processes...\n");

    /********** initializing all the operators ******************************+*/
    for (int i = 0; i < NOF_WORKERS; i++) {
        start_process("operatore", "./bin/operatore");
    }

    //waiting for all the operators to be at the counter
    while (sportello->operatori_ready != 1) {
    	printf("Waiting for operatori to be assigned...\n");
    	sleep(1);
	}
    printf("All operators assigned. Starting user processes...\n");

	/****************************************+*//****************************************+*/
    for (int i = 0; i < NOF_USERS; i++) {
        start_process("utente", "./bin/utente");
    }


    // wait for all child processes to finish
    while (wait(NULL) > 0);

    printf("Simulation finished.\n");
    return 0;
}


pid_t start_process(const char *name, const char *path) {
    pid_t pid = fork();  // Create a new process

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: Execute the new program
        execl(path, name, NULL);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    }

    // Parent process: Return the child process ID
    return pid;
}
