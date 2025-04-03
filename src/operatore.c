#include "config.h"
#include "sportello.h"
#include "semaphore_utils.h"
#include "memory_handler.h"
#include <stdio.h>
#include "direttore.h"
#include <string.h>
#include <stdlib.h>
#include "statistiche.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "operatore.h"
#include <time.h>

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
	LOG_WARN("[OPERATORE] Received signal %d, shutting down\n", sig);
	running = 0;
}

int main(int argc, char *argv[]) {
	attach_sim_time();

	signal(SIGTERM, handle_sigterm);
	srand(time(NULL) ^ getpid());


	int shmid_direttore = get_shared_memory(DIRETTORE_SHM_KEY, "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");

	// Create and attach Sportello shared memory
	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");


	if (argc < 1) {
		printf("arg; %s", argv[1]);
		LOG_ERR("Usage: %s <Operatore service>\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	if (argc > 2 && strcmp(argv[2], "--from-direttore") == 0) {
		LOG_WARN("Operator Launched by direttore");
	} else {
		LOG_WARN("Operator Launched manually from terminal");
		// Save PID
		lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		if (direttore->child_proc_count < MAX_CHILDREN) {
			direttore->child_pids[direttore->child_proc_count++] = getpid();
		}
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	}


	load_config("config/config.json");



	// Create and attach waiting queue shared memory
	int shmid_queue = get_shared_memory(QUEUE_SHM_KEY, "WaitingQueue");
	WaitingQueue *queue = (WaitingQueue *) attach_shared_memory(shmid_queue, "WaitingQueue");

	// Create and attach operator shared memory
	int shmid_operator = get_shared_memory(OPERATORS_SHM_KEY, "Operatore");
	Operatore *operator = (Operatore *) attach_shared_memory(shmid_operator, "Operatore");

	// Create and attach operator shared memory
	int shmid_stats = get_shared_memory(STATISTIC_SHM_KEY, "Statistics");
	Stats *stats = (Stats *) attach_shared_memory(shmid_stats, "Statistics");

	//int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
	//TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");


	//allocate memory for all the workers and counters
	lock_semaphore(OPERATORE_SEMAPHORE_KEY);
	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);

	operator->breaks_taken = malloc(sizeof(int));
	operator->assigned_sportello = malloc(sizeof(int));
	operator->service_type = malloc(sizeof(int));


	//int assigned_service = -1;
	int operatore_index = direttore->operator_count++;
	int sportelllo_index = 0;

	operator->assigned_sportello[operatore_index] = -1;
	int service_type = rand() % NUM_SERVICES;
	operator->service_type[operatore_index] = service_type;


	//LOG_ERR("Servince type at creation: %d", service_type);
	unlock_semaphore(OPERATORE_SEMAPHORE_KEY);
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);

	// Find a free counter that handles your service
	while (operator->assigned_sportello[operatore_index] == -1) {

		lock_semaphore(OPERATORE_SEMAPHORE_KEY);
		for (int i = 0; i < NOF_WORKER_SEATS; i++) {
			lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
			//if the counter is available and the sportello is ofeering the same service type
			//as the operator
			if (sportello->available[i] == 1 &&
				(sportello->service_type[i] == operator->service_type[operatore_index])) {



				sportello->available[i] = 0;
				sportello->assigned_operator[i] = getpid();
				operator->breaks_taken[operatore_index] = 0;
				operator->assigned_sportello[operatore_index] = i;
				sportello->assigned_operator_count++;
				unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);

				//used to track the index of the counter and the operator
				//this will be used to stop the operator and assign the counter to another possible operator waiting in line
				sportelllo_index = i;
				LOG_INFO("[Operatore %d] Assigned to sportello %d.\n", getpid(),
				         operator->assigned_sportello[operatore_index]);
				unlock_semaphore(OPERATORE_SEMAPHORE_KEY);


				lock_semaphore(STATISTIC_SEMAPHORE_KEY);
				stats->active_operators_today++;
				stats->active_operators_total++;

				printf("Active operators taken %d.\n", stats->active_operators_today);
				unlock_semaphore(STATISTIC_SEMAPHORE_KEY);
				break;
			}
			unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
		}
		unlock_semaphore(OPERATORE_SEMAPHORE_KEY);

		//LOG_INFO("[Operatore %d] Waiting for an available sportello\n", getpid());
		sleep(1); // wait for a counter that offers the same service to be available again
	}

	/**
	lock_semaphore(OPERATORE_SEMAPHORE_KEY);
	if (operator->assigned_sportello[operatore_index] == -1) {
		LOG_ERR("[Operatore %d] No available sportello for service %d. Exiting...\n", getpid(), assigned_service);
		exit(1);
	}
	unlock_semaphore(OPERATORE_SEMAPHORE_KEY);*/

	LOG_INFO("[Operatore %d] Ready to serve customers\n", getpid());


	/*lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	sportello->assigned_operator_count++;
	if (sportello->assigned_operator_count == NOF_WORKERS) {
		sportello->operatori_ready = 1;
	}
	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);*/
	//
	//
	///lock_semaphore(OPERATORE_SEMAPHORE_KEY);

	//LOG_ERR("Service type for UTENTE:%d   is %d.\n",getpid(), service_type);
	//unlock_semaphore(OPERATORE_SEMAPHORE_KEY);
	while (running) {

		//int found_ticket = 0;

		/*for (int service_type = 0; service_type < NUM_SERVICES; ++service_type) {
			if (queue->queue_size[service_type] <= 0) continue; //skip if there's no queu


			//unlock_semaphore(service_type);
		}*/
		//LOG_ERR("[Operatore %d] Waiting for services\n", getpid());
		///
		///
		if (operatore_index==1) {

			//LOG_ERR("Queue Size: %d\n", queue->queue_size[operator->service_type[operatore_index]]);
			//OG_ERR("Service type for UTENTE:%d   is %d.\n",getpid(), service_type);
		}


		//LOG_ERR("Queue Size: %d\n", queue->queue_size[service_type]);
		//LOG_ERR("[UTENTE %d] Size: %d, Service: %d",getpid(),  queue->queue_size[service_type], service_type);


		//lock_semaphore(service_type);


		for (int j = 0; j < queue->queue_size[service_type]; j++) {


			int ticket = queue->ticket_queue[service_type][j];

			if (ticket == -1) continue; // skip already served


			lock_semaphore(QUEUE_SEMAPHORE_KEY);
			queue->ticket_queue[service_type][j] = -1; //signing the ticket as served
			unlock_semaphore(QUEUE_SEMAPHORE_KEY);

			//unlock_semaphore(service_type);

			int service_time = SERVICE_TIME[service_type] + (rand() % 5 - 2);

			LOG_INFO("[Operatore %d] Serving ticket %d for Service: [%s] (Expected time: %d min)\n",
					 getpid(), ticket, SERVICE_NAMES[service_type], service_time);


			//printf("Service time %d", service_time);
			sleep_sim_minutes(service_time);

			lock_semaphore(QUEUE_SEMAPHORE_KEY);
			queue->served[service_type][j] = 1;
			unlock_semaphore(QUEUE_SEMAPHORE_KEY);

			//update stats
			lock_semaphore(STATISTIC_SEMAPHORE_KEY);
			stats->served_clients_total++;
			stats->total_serving_time+=service_time;;
			stats->per_service[service_type].served_clients_total++;
			stats->per_service[service_type].total_serving_time+=service_time;

			unlock_semaphore(STATISTIC_SEMAPHORE_KEY);


			LOG_INFO("[Operatore %d] Finished serving ticket %d for Service: [%s] at sportello %d.\n",
					 getpid(), ticket, SERVICE_NAMES[service_type], operator->assigned_sportello[operatore_index]);

			//unlock_semaphore(service_type);
			//LOG_ERR("Number of operators on break: %d", operator->n_op_on_break);
			if (operator->breaks_taken[operatore_index] < NOF_PAUSE && (rand() % 100) < BREAK_PROBABILITY &&
				operator->n_op_on_break < NOF_WORKERS - 3) {
				//to prevent all workers to go on break at the same period

				lock_semaphore(STATISTIC_SEMAPHORE_KEY);
				stats->breaks_today++;
				stats->breaks_total++;
				unlock_semaphore(STATISTIC_SEMAPHORE_KEY);

				operator->n_op_on_break++;
				free_counter(sportello, operator, sportelllo_index, operatore_index);
				take_break(operator, operatore_index);
				}
			break; // Handle one ticket per loop
		}

		//unlock_semaphore(service_type);
		sleep_sim_minutes(1);
	}
}

void take_break(Operatore *operatore, int index) {
	lock_semaphore(OPERATORE_SEMAPHORE_KEY);
	operatore->breaks_taken[index]++;
	unlock_semaphore(OPERATORE_SEMAPHORE_KEY);

	LOG_WARN("[Operatore %d] IS TAKING A BREAK\n", getpid());
	//end working day

	int timeout = 300; // max wait 5 minutes in case direttore crashes or the current day isn't updated
	int current_day = sim_time->current_day;
	int next_day = current_day + 1;
	while (current_day != next_day + 1 && timeout-- > 0) {
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		current_day = sim_time->current_day;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		next_day = current_day + 1;
		sleep_sim_minutes(1);
	}

	operatore->n_op_on_break--;
	//LOG_WARN("[Operatore %d] IS BACK AT WORK\n", getpid());
}

void free_counter(SportelloStatus *sportello, Operatore *operatore, int sportello_index, int operatore_index) {
	lock_semaphore(OPERATORE_SEMAPHORE_KEY);
	lock_semaphore(SPORTELLO_SEMAPHORE_KEY);

	sportello->available[sportello_index] = 1;
	sportello->assigned_operator[sportello_index] = -1;
	operatore->assigned_sportello[operatore_index] = -1;

	unlock_semaphore(OPERATORE_SEMAPHORE_KEY);
	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
}
