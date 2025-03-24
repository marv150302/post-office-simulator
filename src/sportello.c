#include "config.h"
#include "sportello.h"
#include "memory_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {

    printf("Received signal %d, shutting down [TICKET EROGATOR]\n", sig);
    running = 0;
}

int main(int argc, char **argv) {

    signal(SIGTERM, handle_sigterm);
    load_config("config/config.json");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sportello_index>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) ^ getpid());


    // Create and attach Sportello shared memory
    int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
    SportelloStatus *sportello = (SportelloStatus *)attach_shared_memory(shmid_sportello, "Sportello");

    //create and attach waiting queue shared memory
    int shmid_queu = create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "WaitingQueue");
    WaitingQueue *queue = (WaitingQueue *)attach_shared_memory(shmid_queu, "WaitingQueue");

    int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
    TicketSystem *tickets = (TicketSystem *)attach_shared_memory(shmid_erogatore, "Erogatore");

    // Read sportello index from argument
    int sportello_index = atoi(argv[1]);

    if (sportello_index == -1) {
        printf("[Sportello %d] ERROR: No available sportello slot.\n", getpid());
        exit(EXIT_FAILURE);
    }

    printf("[Sportello %d] Handling service %d.\n", getpid(), sportello_index);

    if (sportello_index == NOF_WORKER_SEATS - 1) {

        sportello->sportelli_ready = 1;
        printf("[Sportello %d] All sportelli are now ready!\n", sportello_index);
    }

    while (running) {
        // check if users are waiting for this service
        // printf("queu size: %d\n", queue->queue_size[sportello_index]);
        if (queue->queue_size[sportello_index] > 0 && sportello->available[sportello_index] == 1) {

          //printf("sportello %d is available.\n", sportello_index);
            sportello->available[sportello_index] = 0; // Mark as busy
            int ticket = tickets->ticket_number[sportello_index];

            printf("[Sportello %d] Calling ticket %d for service %d.\n", getpid(), ticket, sportello_index);

            //make the counter wait until it becomes available again
            while (sportello->available[sportello_index] == 0) {
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
