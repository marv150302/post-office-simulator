#include "config.h"
#include "semaphore_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "sportello.h"
#include "memory_handler.h"
#include "statistiche.h"
#include <errno.h>
#include "direttore.h"
#include <time.h>
#include <unistd.h>
#include "process_utils.h"
#include <sys/types.h>
#include "operatore.h"
#include <sys/wait.h>
#include <signal.h>

#include "erogatore_ticket.h"


int main() {
	attach_sim_time(); // for handling simulation time

	//initializing day and time
	sim_time->current_day = 1;
	sim_time->current_hour = OPENING_HOUR;
	sim_time->current_minute = 0;
	srand(time(NULL) ^ getpid());


	load_config("config/config.json"); //load configuration values
	load_termination_config("config/config_timeout.conf");
	load_termination_config("config/config_explode.conf");

	int shmid_direttore = create_shared_memory(DIRETTORE_SHM_KEY, sizeof(Direttore), "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");
	/*
	 * initialize direttore
	 */
	direttore->child_proc_count = 0;
	direttore->client_count = 0;
	direttore->operator_count = 0;
	direttore->killed_added_users = 0;


	int shmid_operator = create_shared_memory(OPERATORS_SHM_KEY, sizeof(Operatore), "Operatore");
	Operatore *operator = (Operatore *) attach_shared_memory(shmid_operator, "Operatore");
	operator->n_op_on_break = 0;

	int shmid_ticket = create_shared_memory(TICKET_EROGATOR_SHM_KEY, sizeof(TicketSystem), "Erogatore");
	TicketSystem* ticket_machine = (TicketSystem *) attach_shared_memory(shmid_ticket, "Erogatore");

	int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");
	//create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");

	create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");


	create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");

	int shmid_stats = create_shared_memory(STATISTIC_SHM_KEY, sizeof(Stats), "Statistics");
	Stats *stats = (Stats *) attach_shared_memory(shmid_stats, "Statistics");

	int cause_of_termination = 0; // cause of simulation termination

	initialize_all_semaphores();
	initialize_stats(stats);
	start_all_processes(direttore);

	LOG_WARN(
		"\n\n ===================== SIMULATION STARTED ===================== \n [Direttore] Starting all services...\n\n");


	int total_minutes = SIM_DURATION * 12 * 60;

	// keep track of the current simulation day
	int current_day = 1;

	for (int i = 0; i < total_minutes; ++i) {
		struct timespec ts = {
			.tv_sec = N_NANO_SECS / 1000000000,
			.tv_nsec = N_NANO_SECS % 1000000000
		};
		nanosleep(&ts, NULL);

		int minute_of_day = i % (12 * 60); // wrrap every 12 hours
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		sim_time->current_day = current_day;
		sim_time->current_hour = (minute_of_day / 60) + OPENING_HOUR;
		sim_time->current_minute = minute_of_day % 60;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);


		//printf("Current Day: %d", current_day);
		if ((i + 1) % (12 * 60) == 0) {
			lock_semaphore(STATISTIC_SEMAPHORE_KEY);
			print_daily_stats(stats, sim_time->current_day, stdout);
			char filename[64];
			snprintf(filename, sizeof(filename), "./csv_logs/statistiche_day_%d.csv", current_day);
			FILE *csv_output = fopen(filename, "w");
			if (csv_output) {
				print_daily_stats(stats, current_day, csv_output);
				fclose(csv_output);
			} else {
				perror("Failed to write stats to file");
			}
			unlock_semaphore(STATISTIC_SEMAPHORE_KEY);

			LOG_WARN("========== SIMULATION DAY %d ENDED ==========\n", current_day);




			clean_shared_memory(QUEUE_SHM_KEY, "Queue");
			clean_shared_memory(TICKET_EROGATOR_SHM_KEY, "Erogatore");
			current_day++;
			sim_time->current_day = current_day;
			sim_time->current_hour = OPENING_HOUR;
			sim_time->current_minute = 0;
			stats->active_operators_today = 0;
			stats->breaks_today = 0;

			lock_semaphore(OPERATORE_SEMAPHORE_KEY);
			operator->n_op_on_break = 0;
			unlock_semaphore(OPERATORE_SEMAPHORE_KEY);


			lock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
			if (ticket_machine->nof_clients_waiting > EXPLODE_THRESHOLD) {

				LOG_ERR("Simulation terminated: explode threshold exceeded (waiting users: %d)", ticket_machine->nof_clients_waiting);
				cause_of_termination = TERMINATION_EXPLODE;
				kill_all_processes(direttore);
				break;
			}
			unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);

			if (current_day <= SIM_DURATION) {
				create_shared_memory(TICKET_EROGATOR_SHM_KEY, sizeof(TicketSystem), "Erogatore");
				create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");
				LOG_WARN("========== SIMULATION DAY %d STARTED ==========\n", current_day);

				assign_service_to_sportello(sportello); //reassign new services to sportelllo

				for (int k = 0; k < NOF_USERS; k++) {
					start_process("utente", "./bin/utente", i, direttore);
				}
			}else {

				LOG_WARN("Simulation terminated: reached maximum simulation duration of %d days", SIM_DURATION);
				cause_of_termination = TERMINATION_TIMEOUT;
				//kill_all_processes(direttore);
				break;
			}
		}
	}



	switch (cause_of_termination) {
		case TERMINATION_TIMEOUT:
			printf("Simulation ended due to SIM_DURATION timeout.\n");
		break;
		case TERMINATION_EXPLODE:
			printf("Simulation ended due to EXPLODE_THRESHOLD being exceeded.\n");
		break;
		default:
			printf("Simulation ended for an unknown reason.\n");
	}

	for (int i = 0; i < direttore->child_proc_count; i++) {
		kill(direttore->child_pids[i], SIGKILL);

	}


	while (wait(NULL) > 0 && direttore->killed_added_users);
	LOG_WARN("All processes terminated. Simulation finished\n");

	clean_shared_memory(STATISTIC_SHM_KEY, "STATISTIC");
	clean_shared_memory(SPORTELLO_SHM_KEY, "Sportello"); // clean shared sportello memory
	clean_shared_memory(DIRETTORE_SHM_KEY, "Direttore"); // clean shared sportello memory
	clean_message_queue(MSG_KEY); // Remove message queue
	cleanup_all_semaphores();


	return 0;
}

void assign_service_to_sportello(void *arg) {
	SportelloStatus *sportello = (SportelloStatus *) arg;
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {
		sportello->service_type[i] = rand() % NUM_SERVICES;
	}
}


void start_all_processes(Direttore *direttore) {
	// Create and attach Sportello shared memory
	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	int shmid_erogatore = get_shared_memory(TICKET_EROGATOR_SHM_KEY, "Erogatore");
	TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");


	/*start all the different processes*/
	start_process("erogatore_ticket", "./bin/erogatore_ticket", 0, direttore);

	tickets->in_use = 0;
	tickets->nof_clients_waiting = 0;
	/******** initialize counters(sportello) *****************+*/

	//sportello->sportelli_ready = 0;
	sportello->assigned_operator_count = 0;

	lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	//int service_already_running = 0;
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {
		sportello->ready[i] = 0;
		//assigning the operator to the counter
		start_process("sportello", "./bin/sportello", i, direttore);
		//if (sportello->service_type[i]==service) service_already_running = 1;
	}
	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);


	while (1) {
		int ready = 0;

		lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
		for (int i = 0; i < NOF_WORKER_SEATS; i++) {
			if (sportello->ready[i] == 1) {
				ready++;
			}
		}
		unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);

		LOG_INFO("Sportelli ready: %d/%d\n", ready, NOF_WORKER_SEATS);

		if (ready >= NOF_WORKER_SEATS) break;

		sleep(1);
	}
	LOG_INFO("All sportelli initialized. Starting operatore processes...\n");

	/********** initializing all the operators ******************************+*/
	for (int i = 0; i < NOF_WORKERS; i++) {
		start_process("operatore", "./bin/operatore", i, direttore);
	}


	LOG_INFO("Operators assigned. Starting user processes...\n");

	/************ intialize all customers *****************+*/ /****************************************+*/

	for (int i = 0; i < NOF_USERS; i++) {
		start_process("utente", "./bin/utente", i, direttore);
	}
}

void kill_all_processes(Direttore *direttore) {
	for (int child_index = 0; child_index < direttore->child_proc_count; child_index++) {
		kill(direttore->child_pids[child_index], SIGTERM);
	}
	//direttore->child_proc_count = 0;
	//while (wait(NULL) > 0);

	while (!direttore->killed_added_users);

	//while (wait(NULL) > 0);
}

void cleanup_all_semaphores(void) {
	//cleanup the service semaphores
	for (int i = 0; i < NUM_SERVICES; i++) {
		cleanup_semaphores(i);
	}
	cleanup_semaphores(OPERATORE_SEMAPHORE_KEY);
	cleanup_semaphores(QUEUE_SEMAPHORE_KEY);
	cleanup_semaphores(SPORTELLO_SEMAPHORE_KEY);
	cleanup_semaphores(DIRETTORE_SEMAPHORE_KEY);
	cleanup_semaphores(SIM_TIME_SEMAPHORE_KEY);
	cleanup_semaphores(STATISTIC_SEMAPHORE_KEY);
	cleanup_semaphores(TICKET_EROGATOR_SEMAPHORE_KEY);
}

void initialize_all_semaphores(void) {
	//initialize service semaphores
	for (int i = 0; i < NUM_SERVICES; i++) {
		initialize_semaphores(i);
	}

	initialize_semaphores(SIM_TIME_SEMAPHORE_KEY);
	initialize_semaphores(OPERATORE_SEMAPHORE_KEY);
	initialize_semaphores(QUEUE_SEMAPHORE_KEY);
	initialize_semaphores(SPORTELLO_SEMAPHORE_KEY);
	initialize_semaphores(DIRETTORE_SEMAPHORE_KEY);
	initialize_semaphores(STATISTIC_SEMAPHORE_KEY);
	initialize_semaphores(TICKET_EROGATOR_SEMAPHORE_KEY);
}
