#include "config.h"
#include "semaphore_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "sportello.h"
#include "memory_handler.h"
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

	sim_time->current_day = 1;
	sim_time->current_hour = 8;
	sim_time->current_minute = 0;
	srand(time(NULL) ^ getpid());



	clean_shared_memory(SPORTELLO_SHM_KEY);// clean shared sportello memory
	clean_shared_memory(QUEUE_SHM_KEY);// clean shared queue memory
	clean_message_queue(MSG_KEY); // Remove message queue

	load_config("config/config.json"); //load configuration values

	int shmid_direttore = create_shared_memory(DIRETTORE_KEY, sizeof(Direttore), "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");
	/*
	 * initialize
	 */
	direttore->child_proc_count = 0;
	direttore->client_count = 0;
	direttore->operator_count = 0;


	int shmid_operator = create_shared_memory(OPERATORS_SHM_KEY, sizeof(Operatore), "Operatore");
	Operatore *operator = (Operatore *) attach_shared_memory(shmid_operator, "Operatore");
	operator->n_op_on_break = 0;


	initialize_all_semaphores();
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

		int minute_of_day = i % (12 * 60);  // Wrap every 12 hours
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		sim_time->current_day = current_day;
		sim_time->current_hour = (minute_of_day / 60) + 8; // Start at 08:00
		sim_time->current_minute = minute_of_day % 60;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);


		if ((i + 1) % (12 * 60) == 0) {
			LOG_WARN("\033[1;33m\n\n ========== SIMULATION DAY %d ENDED ========== \n\n\033[0m", current_day);

			kill_all_processes(direttore);
			cleanup_all_semaphores();
			clean_shared_memory(SPORTELLO_SHM_KEY);
			clean_shared_memory(QUEUE_SHM_KEY);
			clean_message_queue(MSG_KEY);

			current_day++;

			//lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
			sim_time->current_day = current_day;
			sim_time->current_hour = 8;
			sim_time->current_minute = 0;
			//lock_semaphore(SIM_TIME_SEMAPHORE_KEY);

			if (current_day <= SIM_DURATION) {
				initialize_all_semaphores();
				start_all_processes(direttore);
				LOG_WARN("\033[1;33m\n\n ========== SIMULATION DAY %d STARTED ========== \n\n\033[0m", current_day);

			}
		}
	}


	sleep(3); // give the processes time to clean up

	// force kill any that didn’t exit
	for (int i = 0; i < direttore->child_proc_count; i++) {
		kill(direttore->child_pids[i], SIGKILL);
	}

	while (wait(NULL) > 0);
	LOG_WARN("All processes terminated. Simulation finished.\n");

	cleanup_all_semaphores();


	return 0;
}

void start_all_processes(Direttore* direttore) {
	// Create and attach Sportello shared memory
	int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
	TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	/*start all the different processes*/
	start_process("erogatore_ticket", "./bin/erogatore_ticket", 0, direttore);

	tickets->in_use = 0;
	/******** initialize counters(sportello) *****************+*/

	for (int i = 0; i < NOF_WORKER_SEATS; i++) {
		//assigning the operator to the counter
		start_process("sportello", "./bin/sportello", i, direttore);
	}
	sportello->assigned_operator_count = 0;

	int ready_sportello = 0;
	while (!ready_sportello) {

		lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
		ready_sportello = sportello->sportelli_ready;
		unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
		if (!ready_sportello) {
			LOG_INFO("Waiting for sportelli to initialize...\n");
			sleep(1);
		}
	}
	LOG_INFO("All sportelli initialized. Starting operatore processes...\n");

	/********** initializing all the operators ******************************+*/
	for (int i = 0; i < NOF_WORKERS; i++) {
		start_process("operatore", "./bin/operatore", i, direttore);
	}

	//waiting for all the operators to be at the counter
	//lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	int ready_operatore = 0;
	while (!ready_operatore) {
		lock_semaphore(OPERATORE_SEMAPHORE_KEY);
		ready_operatore = sportello->operatori_ready;
		unlock_semaphore(OPERATORE_SEMAPHORE_KEY);
		if (!ready_operatore) {
			LOG_INFO("Waiting for Operatori to initialize...\n");
			sleep(1);
		}
	}
	//unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	LOG_INFO("All operators assigned. Starting user processes...\n");

	/************ intialize all customers *****************+*/ /****************************************+*/
	for (int i = 0; i < NOF_USERS; i++) {
		start_process("utente", "./bin/utente", i, direttore);
	}
}

void kill_all_processes(Direttore* direttore) {
	for (int child_index = 0; child_index < direttore->child_proc_count; child_index++) {
		kill(direttore->child_pids[child_index], SIGTERM);
	}
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
