#include "config.h"
#include "sportello.h"
#include "memory_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include "erogatore_ticket.h"
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include "semaphore_utils.h"

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
	LOG_WARN("Received signal %d, shutting down [SPORTELLO]\n", sig);
	running = 0;
}

int main(int argc, char **argv) {

	attach_sim_time();
	signal(SIGTERM, handle_sigterm);
	load_config("config/config.json");


	if (argc < 2) {
		LOG_ERR("Usage: %s <sportello service>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	srand(time(NULL) ^ getpid());

	// Create and attach Sportello shared memory
	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	// Create and attach waiting queue shared memory
	//int shmid_queu = create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "WaitingQueue");
	//WaitingQueue *queue = (WaitingQueue *) attach_shared_memory(shmid_queu, "WaitingQueue");

	//int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
	//TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	// get the counter(sportello) number from argument passed on command
	int sportello_index = atoi(argv[1]);



	lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	int service = rand() % NUM_SERVICES;
	sportello->service_type[sportello_index] = service; // assign random service
	sportello->available[sportello_index] = 1; // mark as available
	sportello->assigned_operator[sportello_index] = -1; // no operator assigned yet
	sportello->ready[sportello_index] = 1;
	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);


	if (sportello_index == -1) {
		LOG_ERR("[Sportello %d] ERROR: No available sportello slot.\n", getpid());
		exit(EXIT_FAILURE);
	}

	//printf("sportllo index; %d \n" ,sportello_index);
	LOG_INFO("[Sportello %d] Handling service %d.\n", getpid(), service);


	lock_semaphore(SPORTELLO_SEMAPHORE_KEY);

	sportello->sportelli_ready+=1;
	int ready = sportello->sportelli_ready;  // Copy value for logging

	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);

	LOG_WARN("[Sportello %d] Sportello ready. Total ready: %d", getpid(), ready);

	while (running) {
		// check if users are waiting for this service
		//lock_semaphore(QUEUE_SEMAPHORE_KEY);
		/*if (queue->queue_size[service] > 0) {
			//lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
			if (sportello->available[sportello_index] == 1) {
				sportello->available[sportello_index] = 0; // Mark as busy
				//unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);

				int ticket = tickets->ticket_number[service];

				LOG_INFO("[Sportello %d] Calling ticket %d for service %d.\n",
				         getpid(), ticket, sportello_index);

				// make the counter wait until it becomes available again
				while (1) {
					//lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
					if (sportello->available[sportello_index]) {
						//unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
						break;
					}
					//unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
					sleep(1);
				}

				LOG_INFO("[Sportello %d] Now free for the next user.\n", getpid());
			}
		}*/
		//unlock_semaphore(QUEUE_SEMAPHORE_KEY);
		sleep(1);
	}

	shmdt(sportello);
	//shmdt(queue);
	return 0;
}
