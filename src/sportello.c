//
// Created by Marvel  Asuenimhen  on 17/03/25.
//

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

    int sportello_index = -1;
    // find which counter(sportello) this process is responsible for
    for (int i = 0; i < MAX_SPORTELLI; i++) {
        if (sportello->assigned_operator[i] == getpid()) {
            sportello_index = i;
            break;
        }
    }

    if (sportello_index == -1) {
        printf("[Sportello %d] ERROR: Could not find assigned sportello.\n", getpid());
        exit(EXIT_FAILURE);
    }

    printf("[Sportello %d] Handling service %d.\n", getpid(), sportello_index);

    while (1) {
        // check if users are waiting for this service
        if (queue->queue_size[sportello_index] > 0 && sportello->available[getpid()] == 1) {
            sportello->available[getpid()] = 0; // Mark as busy

            int ticket = queue->ticket_queue[sportello_index][0];

            printf("[Sportello %d] Calling ticket %d for service %d.\n", getpid(), ticket, sportello_index);

            //make the counter wait until it becomes available again
            while (sportello->available[getpid()] == 0) {
                sleep(1);
            }

            printf("[Sportello %d] Now free for the next user.\n", getpid());
        }

        sleep(1);
    }

    shmdt(sportello);
    shmdt(queue);
    return 0;
}
