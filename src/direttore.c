#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "sportello.h"
#include "memory_handler.h"
#include "direttore.h"
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {


    // clean shared memory
    int shmid1 = shmget(SPORTELLO_SHM_KEY, 0, 0666);
    if (shmid1 != -1) shmctl(shmid1, IPC_RMID, NULL);

    int shmid2 = shmget(QUEUE_SHM_KEY, 0, 0666);
    if (shmid2 != -1) shmctl(shmid2, IPC_RMID, NULL);

    // Remove message queue
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid != -1) msgctl(msgid, IPC_RMID, NULL);

    load_config("config/config.json");//load configuration values



    srand(time(NULL) ^ getpid());



    // Create and attach Sportello shared memory
    int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
    SportelloStatus *sportello = (SportelloStatus *)attach_shared_memory(shmid_sportello, "Sportello");



  	/*start all the different processes*/

    start_process("erogatore_ticket", "./bin/erogatore_ticket", 0);

    /******** initialize counters(sportello) *****************+*/
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {
        sportello->service_type[i] = rand() % NUM_SERVICES;  // assign random service
        sportello->available[i] = 1;  // mark as available
        sportello->assigned_operator[i] = -1;  // no operator assigned yet
    }
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {

        //assigning the operator to the counter
        start_process("sportello", "./bin/sportello", i);

    }

    while (sportello->sportelli_ready != 1) {
    	printf("Waiting for sportelli to initialize...\n");
    	sleep(1);
	}
	printf("All sportelli initialized. Starting operatore processes...\n");


    /********** initializing all the operators ******************************+*/
    for (int i = 0; i < NOF_WORKERS; i++) {
        start_process("operatore", "./bin/operatore",  i);
    }

    //waiting for all the operators to be at the counter
    while (sportello->operatori_ready != 1) {
    	printf("Waiting for operatori to be assigned...\n");
    	sleep(1);
	}
    printf("All operators assigned. Starting user processes...\n");

	/************ intialize all customers *****************+*//****************************************+*/
    for (int i = 0; i < NOF_USERS; i++) {
        start_process("utente", "./bin/utente", i);
    }


    printf("Simulation started. Running for %d minutes...\n", SIM_DURATION);
    sleep(SIM_DURATION * 60);

    printf("Simulation duration over. Terminating all child processes...\n");
    printf("Sending SIGTERM to all child processes...\n");
    for (int i = 0; i < child_count; i++) {
        kill(child_pids[i], SIGTERM);
    }

    sleep(3);  // Give them time to clean up

    // Force kill any that didn’t exit
    for (int i = 0; i < child_count; i++) {
        kill(child_pids[i], SIGKILL);
    }

    while (wait(NULL) > 0);
    printf("All processes terminated. Simulation finished.\n");

    shmctl(shmid_sportello, IPC_RMID, NULL);
    msgctl(msgid, IPC_RMID, NULL);
    return 0;
}


/*pid_t start_process(const char *name, const char *path) {
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
*/

pid_t start_process(const char *name, const char *path, int arg) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char arg_str[10];
        snprintf(arg_str, sizeof(arg_str), "%d", arg);
        execl(path, name, arg_str, NULL);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    }

    // Save PID
    if (child_count < MAX_CHILDREN) {
        child_pids[child_count++] = pid;
    }

    return pid;
}
