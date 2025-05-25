//
// Created by Marvel  Asuenimhen  on 04/04/25.
//

#include "add_users.h"
#include "process_utils.h"
#include "memory_handler.h"


volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
	LOG_WARN("Received signal %d, shutting down [User Adder]\n", sig);
	running = 0;
}

int main(int argc, char *argv[]) {


	signal(SIGTERM, handle_sigterm);


	attach_sim_time();
	if (argc != 2) {
		LOG_ERR("Usage: ./bin/add_users <number of clients to add>");
		return EXIT_FAILURE;
	}

	int num_clients = atoi(argv[1]);

	LOG_INFO("Number of clients Added: %d ", num_clients);

	if (num_clients <= 0) {

		LOG_ERR("Please provide a number of clients > 0");
		return EXIT_FAILURE;
	}
	int shmid_direttore = get_shared_memory(DIRETTORE_SHM_KEY, "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");


	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	direttore->child_pids[direttore->child_proc_count++] = getpid();
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	for (int i = 0; i < num_clients && running; i++) {

		start_process("utente", "./bin/utente", i, direttore);
	}

	while (wait(NULL) > 0);

	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	direttore->killed_added_users = 1;
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	return 0;
}

