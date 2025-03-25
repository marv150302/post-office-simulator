#include "config.h"
#include "sportello.h"
#include "semaphore_utils.h"
#include "memory_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
    LOG_WARN("[OPERATORE] Received signal %d, shutting down\n", sig);
    running = 0;
}

int main() {

    signal(SIGTERM, handle_sigterm);
    srand(time(NULL) ^ getpid());

    load_config("config/config.json");

    // Create and attach Sportello shared memory
    int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
    SportelloStatus *sportello = (SportelloStatus *)attach_shared_memory(shmid_sportello, "Sportello");

    // Create and attach waiting queue shared memory
    int shmid_queue = create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "WaitingQueue");
    WaitingQueue *queue = (WaitingQueue *)attach_shared_memory(shmid_queue, "WaitingQueue");

    int assigned_service = -1;
    int assigned_sportello = -1;

    // Find a free counter
    while (assigned_sportello == -1) {
        for (int i = 0; i < NOF_WORKER_SEATS; i++) {
            if (sportello->available[i] == 1) {
                sportello->available[i] = 0;
                sportello->assigned_operator[i] = getpid();
                assigned_sportello = i;
                LOG_INFO("[Operatore %d] Assigned to sportello %d.\n", getpid(), assigned_sportello);
                break;
            }
        }

        LOG_INFO("[Operatore %d] Waiting for an available sportello\n", getpid());
        sleep(1);  // Wait and retry
    }

    if (assigned_sportello == -1) {
        LOG_ERR("[Operatore %d] No available sportello for service %d. Exiting...\n", getpid(), assigned_service);
        exit(1);
    }

    LOG_INFO("[Operatore %d] Ready to serve customers\n", getpid());

    int assigned_count = 0;
    for (int i = 0; i < MAX_SPORTELLI; i++) {
        if (sportello->assigned_operator[i] != -1) {
            assigned_count++;
        }
    }

    if (assigned_count == NOF_WORKERS) {
        sportello->operatori_ready = 1;
    }

    while (running) {
        int found_ticket = 0;

        for (int service_type = 0; service_type < NUM_SERVICES; ++service_type) {
            if (queue->queue_size[service_type] > 0) {
                int ticket = queue->ticket_queue[service_type][0];

                lock_semaphore(service_type);
                // Shift the queue (FIFO)
                for (int i = 0; i < queue->queue_size[service_type] - 1; i++) {
                    queue->ticket_queue[service_type][i] = queue->ticket_queue[service_type][i + 1];
                }
                queue->queue_size[service_type]--;
                unlock_semaphore(service_type);

                int service_time = SERVICE_TIME[service_type] + (rand() % 5 - 2);
                LOG_INFO("[Operatore %d] Serving ticket %d for Service: [%s] (Expected time: %d min)\n",
                         getpid(), ticket, SERVICE_NAMES[service_type], service_time);

                sleep(service_time);

                LOG_INFO("[Operatore %d] Finished ticket %d for Service: [%s] at sportello %d.\n",
                            getpid(), ticket, SERVICE_NAMES[service_type], assigned_sportello);

                found_ticket = 1;
                break;  // Handle one ticket per loop
            }
        }

        if (!found_ticket) {
            sleep(1);  // Only sleep if the cycle is idle
        }
    }
}