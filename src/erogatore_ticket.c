#include "config.h"
#include "erogatore_ticket.h"
#include "memory_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>


volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
	LOG_WARN("Received signal %d, shutting down [TICKET EROGATOR]\n", sig);
	running = 0;
}

int main() {

	attach_sim_time();
	load_config("config/config.json");
	signal(SIGTERM, handle_sigterm);

	// Create and attach shared memory for ticket system
	int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
	TicketSystem *tickets = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	srand(time(NULL) ^ getpid());

	for (int i = 0; i < NUM_SERVICES; i++) {
		tickets->ticket_number[i] = 1;
	}

	// Create message queue for ticket requests
	int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
	if (msgid == -1) {
		LOG_ERR("Message queue creation failed\n");
		exit(EXIT_FAILURE);
	}

	LOG_INFO("[Erogatore] Ticket system initialized. Waiting for user requests...\n");

	while (running) {
		// Wait for a user request
		TicketMessage request;
		if (msgrcv(msgid, &request, sizeof(TicketMessage) - sizeof(long), 10, 0) == -1) {
			LOG_ERR("Message receive failed\n");
			exit(EXIT_FAILURE);
		}

		int service_type = request.service_type;
		int ticket_number = tickets->ticket_number[service_type]++;

		int base_time = SERVICE_TIME[service_type];
		int variation = (base_time >= 4) ? (rand() % (base_time / 2)) - (base_time / 4) : 0;
		int estimated_time = base_time + variation;
		if (estimated_time < 1) estimated_time = 1;

		// Create and send ticket message
		TicketMessage msg;
		msg.msg_type = service_type + 1;
		msg.ticket_number = ticket_number;
		msg.estimated_time = estimated_time;

		if (msgsnd(msgid, &msg, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
			LOG_ERR("Message send failed\n");
			exit(EXIT_FAILURE);
		}

		LOG_INFO("[Erogatore] Issued ticket %d for Service: [%s] (Estimated time: %d min)\n",
		         ticket_number, SERVICE_NAMES[service_type], estimated_time);

		sleep(1);
	}

	shmdt(tickets);
	return 0;
}