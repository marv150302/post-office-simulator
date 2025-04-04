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
#include <sys/types.h>
#include "operatore.h"
#include <sys/wait.h>
#include <signal.h>

#include "erogatore_ticket.h"


void clear_screen() {
	printf("\033[2J\033[H");
	fflush(stdout);
}

int main() {

	attach_sim_time(); // for handling simulation time

	//initializing day and time
	sim_time->current_day = 1;
	sim_time->current_hour = OPENING_HOUR;
	sim_time->current_minute = 0;
	srand(time(NULL) ^ getpid());



	//clean_shared_memory(SPORTELLO_SHM_KEY, "Sportello");// clean shared sportello memory
	//clean_shared_memory(SHM_KEY, "Erogatore");
	//clean_shared_memory(DIRETTORE_SHM_KEY, "Direttore");// clean shared sportello memory
	//clean_shared_memory(QUEUE_SHM_KEY, "Queue");// clean shared queue memory
	//clean_message_queue(MSG_KEY); // Remove message queue

	load_config("config/config.json"); //load configuration values

	int shmid_direttore = create_shared_memory(DIRETTORE_SHM_KEY, sizeof(Direttore), "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");
	/*
	 * initialize direttore
	 */
	direttore->child_proc_count = 0;
	direttore->client_count = 0;
	direttore->operator_count = 0;



	int shmid_operator = create_shared_memory(OPERATORS_SHM_KEY, sizeof(Operatore), "Operatore");
	Operatore *operator = (Operatore *) attach_shared_memory(shmid_operator, "Operatore");
	operator->n_op_on_break = 0;

	create_shared_memory(SHM_KEY,sizeof(TicketSystem), "Erogatore");

	int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");
	//create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");

	create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");
	int shmid_stats = create_shared_memory(STATISTIC_SHM_KEY,sizeof(Stats), "Statistics");
	Stats *stats = (Stats *) attach_shared_memory(shmid_stats, "Statistics");




	initialize_all_semaphores();
	initialize_stats(stats);
	start_all_processes(direttore);

	LOG_WARN(
		"\n\n ===================== SIMULATION STARTED ===================== \n [Direttore] Starting all services...\n\n");


	int total_minutes = SIM_DURATION * 12 * 60;

	// Keep track of the current simulation day
	int current_day = 1;

	for (int i = 0; i < total_minutes; ++i) {
		struct timespec ts = {
			.tv_sec = N_NANO_SECS / 1000000000,
			.tv_nsec = N_NANO_SECS % 1000000000
		};
		nanosleep(&ts, NULL);

		int minute_of_day = i % (12 * 60);  // wrrap every 12 hours
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		sim_time->current_day = current_day;
		sim_time->current_hour = (minute_of_day / 60) + OPENING_HOUR;
		sim_time->current_minute = minute_of_day % 60;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);


		//printf("Current Day: %d", current_day);
		if ((i + 1) % (12 * 60) == 0) {

			/*FILE *logfile = fopen("daily_stats.txt", "a"); // or stdout if printing
			if (logfile != NULL) {
				print_daily_stats(stats, sim_time->current_day, logfile);
				fclose(logfile);
			} else {
				perror("Failed to open stats log file");
			}*/





			/*kill_all_processes(direttore);
			cleanup_all_semaphores();

			clean_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
			clean_shared_memory(SHM_KEY, "Erogatore");
			clean_shared_memory(DIRETTORE_SHM_KEY, "Direttore");
			clean_shared_memory(QUEUE_SHM_KEY, "Queue");*/

			//clean_message_queue(MSG_KEY);

			lock_semaphore(STATISTIC_SEMAPHORE_KEY);
			/*for (int j = 0; j < NOF_WORKER_SEATS; j++) {

				stats->operator_to_sportello_ratio_today[j] = (float)stats->active_operators_today / (float)NOF_WORKER_SEATS;
			}*/
			//stats->services_not_offered_total = NUM_SERVICES - stats->services_offered_total;
			print_daily_stats(stats, sim_time->current_day, stdout);
			/*stats->active_operators_today = 0;
			stats->breaks_today = 0;*/
			unlock_semaphore(STATISTIC_SEMAPHORE_KEY);

			LOG_WARN("========== SIMULATION DAY %d ENDED ==========\n", current_day);

			clean_shared_memory(QUEUE_SHM_KEY, "Queue");
			clean_shared_memory(SHM_KEY, "Erogatore");
			current_day++;
			sim_time->current_day = current_day;
			sim_time->current_hour = OPENING_HOUR;
			sim_time->current_minute = 0;

			if (current_day <= SIM_DURATION) {

				//LOG_WARN("Preparing to start processes for Day %d", current_day);
				/*
				create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
				create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
				create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");
				create_shared_memory(DIRETTORE_SHM_KEY, sizeof(Direttore), "Direttore");

				initialize_all_semaphores();
				start_all_processes(direttore);*/

				create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
				create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "Queue");
				LOG_WARN("========== SIMULATION DAY %d STARTED ==========\n", current_day);

				assign_service_to_sportello(sportello); //reassign new service to sportelllo
				//i have to make NOF_USERS + the users that weren't served the previous day
				for (int k = 0; k < NOF_USERS; k++) {
					start_process("utente", "./bin/utente", i, direttore);
				}
			}
		}
	}

	//update_statistics(stats);

	sleep(3); // give the processes time to clean up

	// force kill any that didn’t exit
	for (int i = 0; i < direttore->child_proc_count; i++) {
		kill(direttore->child_pids[i], SIGKILL);
	}

	while (wait(NULL) > 0);
	LOG_WARN("All processes terminated. Simulation finished.\n");

	clean_shared_memory(STATISTIC_SHM_KEY, "STATISTIC");
	cleanup_all_semaphores();


	return 0;
}

void assign_service_to_sportello(void* arg) {

    SportelloStatus* sportello = (SportelloStatus*) arg;
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {

		sportello->service_type[i] = rand() % NUM_SERVICES;
	}
}

/*void update_statistics(Stats *stats) {



}

void reset_statistics(Stats *stats) {

	//stats->services_offered_total=0;
}*/


void start_all_processes(Direttore* direttore) {

	// Create and attach Sportello shared memory
	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	int shmid_erogatore = get_shared_memory(SHM_KEY, "Erogatore");
	TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");


	/*start all the different processes*/
	start_process("erogatore_ticket", "./bin/erogatore_ticket", 0, direttore);

	tickets->in_use = 0;
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

	//lock_semaphore(STATISTIC_SEMAPHORE_KEY);
	//if (!service_already_running) stats->services_offered_total++;
	//unlock_semaphore(STATISTIC_SEMAPHORE_KEY);


	/*while (1) {
		lock_semaphore(SPORTELLO_SHM_KEY);
		int sportelli_ready = sportello->sportelli_ready;
		unlock_semaphore(SPORTELLO_SHM_KEY);

		LOG_INFO("Waiting for sportelli to initialize... (%d/%d)\n", sportelli_ready, NOF_WORKER_SEATS);

		if (sportelli_ready >= NOF_WORKER_SEATS)
			break;

		sleep(1);
	}*/
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

	//waiting for all the operators to be at the counter
	//lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	/*int ready_operatore = 0;
	while (!ready_operatore) {
		lock_semaphore(OPERATORE_SEMAPHORE_KEY);
		ready_operatore = sportello->operatori_ready;
		unlock_semaphore(OPERATORE_SEMAPHORE_KEY);
		if (!ready_operatore) {
			LOG_INFO("Waiting for Operatori to initialize...\n");
			sleep(1);
		}
	}*/
	//unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	LOG_INFO("Operators assigned. Starting user processes...\n");

	/************ intialize all customers *****************+*/ /****************************************+*/

	for (int i = 0; i < NOF_USERS; i++) {
		start_process("utente", "./bin/utente", i, direttore);
	}
}

void kill_all_processes(Direttore* direttore) {
	for (int child_index = 0; child_index < direttore->child_proc_count; child_index++) {
		kill(direttore->child_pids[child_index], SIGTERM);
	}
	direttore->child_proc_count = 0;
    while (wait(NULL) > 0);
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


pid_t start_process(const char *name, const char *path, int arg, Direttore* direttore) {

	pid_t pid = fork();
	if (pid < 0) {
		LOG_ERR("Fork failed\n");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
		char arg_str[10];
		snprintf(arg_str, sizeof(arg_str), "%d", arg);
		execl(path, name, arg_str,"--from-direttore", NULL);
		perror("Exec failed");
		exit(EXIT_FAILURE);
	}

	// Save PID
	if (direttore->child_proc_count < MAX_CHILDREN) {

		lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		direttore->child_pids[direttore->child_proc_count++] = pid;
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	}

	return pid;
}


