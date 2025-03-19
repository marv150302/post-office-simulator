#include "config.h"
#include "sportello.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

int main() {

    srand(time(NULL) ^ getpid());

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

    // attach to user waiting queue
    int shmid_queue = shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), 0666);
    if (shmid_queue == -1) {
        perror("Shared memory access failed (Queue)");
        exit(EXIT_FAILURE);
    }
    WaitingQueue *queue = (WaitingQueue *)shmat(shmid_queue, NULL, 0);
    if (queue == (void *)-1) {
        perror("Shared memory attach failed (Queue)");
        exit(EXIT_FAILURE);
    }

    return 0;
}
