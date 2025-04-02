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
	operator->current_day = 1;


	initialize_all_semaphores();
	start_all_processes(direttore);

	LOG_WARN(
		"\n\n ===================== SIMULATION STARTED ===================== \n [Direttore] Starting all services...\n\n");


	int total_minutes = SIM_DURATION * 24 * 60;

	for (int i = 0; i < total_minutes; ++i) {
		// nanosleep for N_NANO_SECS
		struct timespec ts = {
			.tv_sec = N_NANO_SECS / 1000000000,
			.tv_nsec = N_NANO_SECS % 1000000000
		};
		nanosleep(&ts, NULL);

		//sim_time->current_hour = (i % (24 * 60)) / 60;
		//sim_time->current_minute = (i % (24 * 60)) % 60;
		int minutes_since_start = i + (8 * 60); // 8 AM
		sim_time->current_day = (minutes_since_start / (24 * 60)) + 1;
		sim_time->current_hour = (minutes_since_start % (24 * 60)) / 60;
		sim_time->current_minute = (minutes_since_start % (24 * 60)) % 60;
		if ((i + 1) % (24 * 60) == 0) {
			int day = (i + 1) / (24 * 60);
			sim_time->current_day = day;
			sim_time->current_hour = (total_minutes % (24 * 60)) / 60;
			sim_time->current_minute = (total_minutes % (24 * 60)) % 60;


			//clear_screen();
			LOG_WARN("\033[1;33m\n\n ========================================== SIMULATION DAY %d ENDED ====================================================================  \n\n\033[0m", day);

			kill_all_processes(direttore);
			cleanup_all_semaphores();
			// clean shared memory
			clean_shared_memory(SPORTELLO_SHM_KEY);
			clean_shared_memory(QUEUE_SHM_KEY);
			// Remove message queue
			clean_message_queue(MSG_KEY);

			if (day < SIM_DURATION) {
				//restart
				initialize_all_semaphores();
				start_all_processes(direttore);

				LOG_WARN("\033[1;33m\n\n ========================================== SIMULATION DAY %d STARTED ====================================================================  \n\n\033[0m", day + 1);
				//update the operator current day
				operator->current_day = day+1;
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

	/*start all the different processes*/
	start_process("erogatore_ticket", "./bin/erogatore_ticket", 0, direttore);

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
}

void initialize_all_semaphores(void) {

	//initialize service semaphores
	for (int i = 0; i < NUM_SERVICES; i++) {
		initialize_semaphores(i);
	}

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
