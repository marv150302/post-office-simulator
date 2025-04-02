#include "config.h"
#include "erogatore_ticket.h"
#include "semaphore_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include "direttore.h"
#include "memory_handler.h"
#include <string.h>

int main(int argc, char *argv[]) {


	attach_sim_time();
	srand(time(NULL) ^ getpid());

	load_config("config/config.json");

	//choose a random value between p_SERVE_MAX and P_SERV_MIN
	double p_serv = P_SERV_MIN + ((double) rand() / RAND_MAX) * (P_SERV_MAX - P_SERV_MIN);

	const double roll = (double)rand() / RAND_MAX;

	//if the client choose to stay home
	if (roll >= p_serv) {

		//LOG_WARN("Client [%d] CHOOSE TO STAY AT HOME", getpid());
		//return 0;
	}

	int shmid_direttore = create_shared_memory(DIRETTORE_KEY, sizeof(Direttore), "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");


	if (argc > 2 && strcmp(argv[2], "--from-direttore") == 0) {
		LOG_WARN("Client Launched by direttore");
	} else {
		LOG_WARN("Client Launched manually from terminal");
		// Save PID
		lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		if (direttore->child_proc_count < MAX_CHILDREN) {


			direttore->child_pids[direttore->child_proc_count++] = getpid();
			//unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		}
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	}

	int shmid_queue = create_shared_memory(QUEUE_SHM_KEY, sizeof(WaitingQueue), "WaitingQueue");
	WaitingQueue *queue = (WaitingQueue *) attach_shared_memory(shmid_queue, "WaitingQueue");

	int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
	TicketSystem *ticket_machine = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	// randomly select a service
	int service_type = rand() % NUM_SERVICES;


	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	if (direttore->client_count >= MAX_CLIENTS) {
		LOG_ERR("[Utente %d] MAX_CLIENTS (%d) reached. Exiting...\n", getpid(), MAX_CLIENTS);
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		exit(EXIT_FAILURE);
	}
	int client_index = direttore->client_count++;
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);

	if (queue->queue_size[service_type] >= MAX_CLIENT_FOR_SERVICE) {
		LOG_WARN("[Utente %d] Queue for service %d is full. Exiting...\n", getpid(), service_type);
		shmdt(queue);
		return EXIT_FAILURE;
	}

	// request ticket
	int msgid = msgget(MSG_KEY, 0666);
	if (msgid == -1) {
		LOG_ERR("Message get failed for [CLIENT]");
		exit(EXIT_FAILURE);
	}

	LOG_INFO("[Utente %d] Requesting ticket for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);

	TicketMessage req;
	req.msg_type = 10;
	req.service_type = service_type;
	req.pid = getpid();

	if (msgsnd(msgid, &req, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
		LOG_ERR("Message send failed");
		exit(EXIT_FAILURE);
	}


	// int in_use = tickets->in_use;
	while (ticket_machine->in_use==1) {

		sleep(1);
	}

	ticket_machine->in_use = 1;


	// Receive ticket response
	TicketMessage msg;
	if (msgrcv(msgid, &msg, sizeof(TicketMessage) - sizeof(long), getpid(), 0) == -1) {
		LOG_ERR("Ticket retrieval failed for [UTENTE %d]", getpid());
		exit(EXIT_FAILURE);
	}


	LOG_INFO("[Utente %d] Received Ticket: %d for Service: [%s] (Estimated time: %d min)\n",
	         getpid(), msg.ticket_number, SERVICE_NAMES[service_type], msg.estimated_time);

	ticket_machine->in_use = 0; //reset the usage of the ticket machine

	// add ticket to queue
	lock_semaphore(service_type);

	int pos = queue->queue_size[service_type];
	if (pos >= MAX_CLIENT_FOR_SERVICE) {
		LOG_ERR("[Utente %d] Queue position overflow for service %d", getpid(), service_type);
		shmdt(queue);
		exit(EXIT_FAILURE);
	}
	queue->ticket_queue[service_type][pos] = msg.ticket_number;
	queue->queue_size[service_type]++;
	queue->served[client_index] = 0;

	unlock_semaphore(service_type);

	LOG_INFO("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);


	while (queue->ticket_queue[service_type][pos] != -1) {//while the ticket is being served

		sleep(1);
	} //let the client wait till he's served

	LOG_INFO("[Utente %d] HAS BEEN SERVED\n", getpid());

	//i need to add the case where a client has another service requirment
	shmdt(queue);
	shmdt(direttore);
	shmdt(ticket_machine);
	return 0;
}
