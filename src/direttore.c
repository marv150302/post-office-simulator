#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "sportello.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

int main() {
    load_config("config/config.txt");

    printf("Starting simulation...\n");

    // attach to sportello shared memory
    int shmid_sportello = shmget(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), 0666);
    if (shmid_sportello == -1) {
        perror("Shared memory access failed (Sportello)");
        exit(EXIT_FAILURE);
    }
    SportelloStatus *sportello = (SportelloStatus *)shmat(shmid_sportello, NULL, 0);
    if (sportello == (void *)-1) {
        perror("Shared memory attach failed (Sportello)");
        exit(EXIT_FAILURE);
    }

    // Create shared memory for the queue
    int shmid_queue = shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), IPC_CREAT | 0666);
    if (shmid_queue == -1) {
        perror("Shared memory creation failed (Queue)");
        exit(EXIT_FAILURE);
    }
    WaitingQueue *queue = (WaitingQueue *)shmat(shmid_queue, NULL, 0);
    if (queue == (void *)-1) {
        perror("Shared memory attach failed (Queue)");
        exit(EXIT_FAILURE);
    }

    printf("Shared memory successfully created.\n");


    start_process("erogatore_ticket", "./bin/erogatore_ticket");

    for (int i = 0; i < NOF_WORKERS; i++) {
        start_process("operatore", "./bin/operatore");
    }

    for (int i = 0; i < NOF_USERS; i++) {
        start_process("utente", "./bin/utente");
    }

    // initialize counters
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {
        sportello->service_type[i] = rand() % NUM_SERVICES;  // assign random service
        sportello->available[i] = 1;  // mark as available
        sportello->assigned_operator[i] = -1;  // no operator assigned yet
    }
    for (int i = 0; i < NOF_WORKER_SEATS; i++) {

        sportello->assigned_operator[i] = start_process("sportello", "./bin/sportello"); //assigning the operator
    }


    // wait for all child processes to finish
    while (wait(NULL) > 0);

    printf("Simulation finished.\n");
    return 0;
}
