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
#include <sys/wait.h>
#include <signal.h>


void clear_screen() {
	printf("\033[2J\033[H");
	fflush(stdout);
}

int main() {
	// clean shared memory
	clean_shared_memory(SPORTELLO_SHM_KEY);
	clean_shared_memory(QUEUE_SHM_KEY);


	// Remove message queue
	clean_message_queue(MSG_KEY);

	load_config("config/config.json"); //load configuration values

	srand(time(NULL) ^ getpid());

	initialize_all_semaphores();
	start_all_processes();

	LOG_INFO(
		"\n\n ===================== SIMULATION STARTED ===================== \n [Direttore] Starting all services...\n\n");


	int total_minutes = SIM_DURATION * 24 * 60;

	for (int i = 0; i < total_minutes; ++i) {
		// nanosleep for N_NANO_SECS
		struct timespec ts = {
			.tv_sec = N_NANO_SECS / 1000000000,
			.tv_nsec = N_NANO_SECS % 1000000000
		};
		nanosleep(&ts, NULL);


		if ((i + 1) % (24 * 60) == 0) {
			int day = (i + 1) / (24 * 60);
			//clear_screen();
			LOG_WARN("\033[1;33m\n\n ========================================== SIMULATION DAY %d STARTED ====================================================================  \n\n\033[0m", day);

			kill_all_processes();
			cleanup_all_semaphores();
			// clean shared memory
			clean_shared_memory(SPORTELLO_SHM_KEY);
			clean_shared_memory(QUEUE_SHM_KEY);
			// Remove message queue
			clean_message_queue(MSG_KEY);

			if (day < SIM_DURATION) {
				//restart
				initialize_all_semaphores();
				start_all_processes();

				LOG_WARN("\033[1;33m\n\n ========================================== SIMULATION DAY %d STARTED ====================================================================  \n\n\033[0m", day + 1);
			}
		}
	}

	//LOG_WARN("Simulation duration over. Terminating all child processes...\n");
	//LOG_WARN("Sending SIGTERM to all child processes...\n");


	sleep(3); // give the processes time to clean up

	// force kill any that didn’t exit
	for (int i = 0; i < child_count; i++) {
		kill(child_pids[i], SIGKILL);
	}

	while (wait(NULL) > 0);
	LOG_WARN("All processes terminated. Simulation finished.\n");

	cleanup_all_semaphores();


	return 0;
}

void start_all_processes() {
	// Create and attach Sportello shared memory
	int shmid_sportello = create_shared_memory(SPORTELLO_SHM_KEY, sizeof(SportelloStatus), "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	/*start all the different processes*/
	start_process("erogatore_ticket", "./bin/erogatore_ticket", 0);

	/******** initialize counters(sportello) *****************+*/
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {
		sportello->service_type[i] = rand() % NUM_SERVICES; // assign random service
		sportello->available[i] = 1; // mark as available
		sportello->assigned_operator[i] = -1; // no operator assigned yet
	}
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {
		//assigning the operator to the counter
		start_process("sportello", "./bin/sportello", i);
	}

	while (sportello->sportelli_ready != 1) {
		LOG_INFO("Waiting for sportelli to initialize...\n");
		sleep(1);
	}
	LOG_INFO("All sportelli initialized. Starting operatore processes...\n");

	/********** initializing all the operators ******************************+*/
	for (int i = 0; i < NOF_WORKERS; i++) {
		start_process("operatore", "./bin/operatore", i);
	}

	//waiting for all the operators to be at the counter
	while (sportello->operatori_ready != 1) {
		LOG_INFO("Waiting for operatori to be assigned...\n");
		sleep(1);
	}
	LOG_INFO("All operators assigned. Starting user processes...\n");

	/************ intialize all customers *****************+*/ /****************************************+*/
	for (int i = 0; i < NOF_USERS; i++) {
		start_process("utente", "./bin/utente", i);
	}
}

void kill_all_processes() {
	for (int child_index = 0; child_index < child_count; child_index++) {
		kill(child_pids[child_index], SIGTERM);
	}
}

void cleanup_all_semaphores(void) {

	for (int i = 0; i < NUM_SERVICES; i++) {
		cleanup_semaphores(i);
	}
}

void initialize_all_semaphores(void) {

	//initialize semaphores
	for (int i = 0; i < NUM_SERVICES; i++) {
		initialize_semaphores(i);
	}
}


pid_t start_process(const char *name, const char *path, int arg) {
	pid_t pid = fork();
	if (pid < 0) {
		LOG_ERR("Fork failed\n");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		char arg_str[10];
		snprintf(arg_str, sizeof(arg_str), "%d", arg);
		execl(path, name, arg_str, NULL);
		perror("Exec failed");
		exit(EXIT_FAILURE);
	}

	// Save PID
	if (child_count < MAX_CHILDREN) {
		child_pids[child_count++] = pid;
	}

	return pid;
}
