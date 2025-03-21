//
// Created by Marvel  Asuenimhen  on 17/03/25.
//

#include "config.h"
#include "sportello.h"
#include "memory_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv) {

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


    // Read sportello index from argument
    int sportello_index = atoi(argv[1]);

    // Set PID (for confirmation / debugging, )
    //sportello->assigned_operator[sportello_index] = getpid();


    if (sportello_index == -1) {
        printf("[Sportello %d] ERROR: No available sportello slot.\n", getpid());
        exit(EXIT_FAILURE);
    }


    printf("[Sportello %d] Handling service %d.\n", getpid(), sportello_index);

    if (sportello_index == NOF_WORKER_SEATS - 1) {


        sportello->sportelli_ready = 1;
        printf("[Sportello %d] All sportelli are now ready!\n", sportello_index);
    }
    while (1) {
        // check if users are waiting for this service
        if (queue->queue_size[sportello_index] > 0 && sportello->available[sportello_index] == 1) {
            sportello->available[sportello_index] = 0; // Mark as busy

            int ticket = queue->ticket_queue[sportello_index][0];

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
