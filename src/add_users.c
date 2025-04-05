//
// Created by Marvel  Asuenimhen  on 04/04/25.
//

#include "add_users.h"
#include "process_utils.h"
#include "memory_handler.h"



int main(int argc, char *argv[]) {

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
	for (int i = 0; i < num_clients; i++) {
		start_process("utente", "./bin/utente", i, direttore);
	}

	LOG_INFO("Successfully added users");
	return EXIT_SUCCESS;
}

