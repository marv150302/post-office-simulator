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

    // Attach to the shared queue memory
    int shmid = shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), 0666);
    if (shmid == -1) {
        perror("Shared memory access failed");
        exit(EXIT_FAILURE);
    }
    WaitingQueue *queue = (WaitingQueue *)shmat(shmid, NULL, 0);
    if (queue == (void *)-1) {
        perror("Shared memory attach failed");
        exit(EXIT_FAILURE);
    }

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


    int assigned_service = -1;
    int assigned_sportello = -1;

    // find a free counter with a matching service
    for (int i = 0; i < MAX_SPORTELLI; i++) {
        if (sportello->available[i] == 1 && sportello->service_type[i] == assigned_service) {
            assigned_sportello = i;
            sportello->available[i] = 0;  // mark the counter as occupied
            sportello->assigned_operator[i] = getpid();  // assign an operator to the counter
            break;
        }
    }

    if (assigned_sportello == -1) {
        printf("[Operatore %d] No available sportello for service %d. Exiting...\n", getpid(), assigned_service);
        exit(1);
    }


    printf("[Operatore %d] Ready to serve customers...\n", getpid());

    while (1) {
        int service_type = rand() % NUM_SERVICES;  // randomly choose a service to check

        // check if users are waiting for this service
        if (queue->queue_size[service_type] > 0) {
            int ticket = queue->ticket_queue[service_type][0];

            // shift queue forward the first user that arrives is the first user to be served(fifo)
            for (int i = 0; i < queue->queue_size[service_type] - 1; i++) {
                queue->ticket_queue[service_type][i] = queue->ticket_queue[service_type][i + 1];
            }
            queue->queue_size[service_type]--;  // Reduce queue size

            // calculate operator service time randomly
            // rand() % 5 → Generates a random number between 0 and 4.
            // Subtracting 2 shifts the range of the service time to -2 to +2.
            int service_time = SERVICE_TIME[service_type] + (rand() % 5 - 2);
            printf("[Operatore %d] Serving ticket %d for service %d (Expected time: %d min)\n",
                   getpid(), ticket, service_type, service_time);
            //simulate the operator working
            sleep(service_time);

            printf("[Operatore %d] Finished serving ticket %d\n", getpid(), ticket);
            printf("[Operatore %d] Finished serving ticket %d at sportello %d.\n", getpid(), ticket, assigned_sportello);
            sportello->available[assigned_sportello] = 1;  // Free the sportello
        }

        sleep(1);  // wait before checking in, so we prevent the cpu from working too much
    }
}