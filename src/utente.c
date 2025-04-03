#include "config.h"
#include "erogatore_ticket.h"
#include "semaphore_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "utente.h"
#include <signal.h>
#include "statistiche.h"
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include "direttore.h"
#include "memory_handler.h"
#include <string.h>

#include "sportello.h"

int main(int argc, char *argv[]) {


	attach_sim_time();
	srand(time(NULL) ^ getpid());

	load_config("config/config.json");

	//choose a random value between p_SERVE_MAX and P_SERV_MIN
	double p_serv = P_SERV_MIN + ((double) rand() / RAND_MAX) * (P_SERV_MAX - P_SERV_MIN);

	const double roll = (double)rand() / RAND_MAX;

	//if the client choose to stay home instead of going to the office
	if (roll >= p_serv) {

		//LOG_WARN("Client [%d] CHOOSE TO STAY AT HOME", getpid());
		//return 0;
	}

	int shmid_direttore = get_shared_memory(DIRETTORE_SHM_KEY, "Direttore");
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

	int shmid_queue = get_shared_memory(QUEUE_SHM_KEY, "WaitingQueue");
	WaitingQueue *queue = (WaitingQueue *) attach_shared_memory(shmid_queue, "WaitingQueue");

	int shmid_erogatore = get_shared_memory(SHM_KEY, "Erogatore");
	TicketSystem *ticket_machine = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	int shmid_stats = get_shared_memory(STATISTIC_SHM_KEY, "Statistics");
	Stats *stats = (Stats *) attach_shared_memory(shmid_stats, "Statistics");


	int min_hour = sim_time->current_hour;
	int max_hour = CLOSING_HOUR - 1; // last valid full hour

	// Random hour from current to closing


	Utente utente;
	utente.requested_service = rand() % NUM_SERVICES; // randomly select a service
	int serving = 0;
    //chek if the service is handled
	lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	for (int i = 0; i < NOF_WORKER_SEATS; i++) {

		printf("Sportello service type inside of utente %d \n", sportello->service_type[i]);
		if (sportello->service_type[i]==utente.requested_service) {
			serving=1;
			break;
		} ;
	}
	unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);
	if (!serving) {

		LOG_WARN("The Service %s is not handled today, try tomorrow!", SERVICE_NAMES[utente.requested_service]);
		return 0;
	}


	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	utente.arrival_hour = min_hour + rand() % (max_hour - min_hour + 1);
	// If user will arrive in the same hour, we make sure the minute is in the future
	if (utente.arrival_hour == sim_time->current_hour) {
		utente.arrival_minute = sim_time->current_minute + rand() % (60 - sim_time->current_minute);
	} else {
		utente.arrival_minute = rand() % 60;
	}

	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	char minute_buf[3]; // 2 digits + null terminator
	snprintf(minute_buf, sizeof(minute_buf), "%02d", utente.arrival_minute); //ensures a 0 in front of numbers less than 10
	LOG_WARN("UTENTE[%d] Booked an appointment for %d:%s", getpid(), utente.arrival_hour, minute_buf);

	//wait for your appointment
	do {
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		int hour = sim_time->current_hour;
		int minute = sim_time->current_minute;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

		if (hour < utente.arrival_hour ||
			(hour == utente.arrival_hour && minute < utente.arrival_minute)) {
			sleep(1); // simulate wait until arrival time
			} else {
				break; // time to enter!
			}
	} while (1);


	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	if (direttore->client_count >= MAX_CLIENTS) {
		LOG_ERR("[Utente %d] MAX_CLIENTS (%d) reached. Exiting...\n", getpid(), MAX_CLIENTS);
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		exit(EXIT_FAILURE);
	}
	int client_index = direttore->client_count++;
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);

	if (queue->queue_size[utente.requested_service] >= MAX_CLIENT_FOR_SERVICE) {
		LOG_WARN("[Utente %d] Queue for service %d is full. Exiting...\n", getpid(), utente.requested_service);
		shmdt(queue);
		return EXIT_FAILURE;
	}

	// request ticket
	int msgid = msgget(MSG_KEY, 0666);
	if (msgid == -1) {
		LOG_ERR("Message get failed for [CLIENT]");
		exit(EXIT_FAILURE);
	}

	LOG_INFO("[Utente %d] Requesting ticket for Service: [%s]\n", getpid(), SERVICE_NAMES[utente.requested_service]);

	TicketMessage req;
	req.msg_type = 10;
	req.service_type = utente.requested_service;
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
	         getpid(), msg.ticket_number, SERVICE_NAMES[utente.requested_service], msg.estimated_time);

	ticket_machine->in_use = 0; //reset the usage of the ticket machine

	// add ticket to queue
	lock_semaphore(utente.requested_service);

	int pos = queue->queue_size[utente.requested_service];
	if (pos >= MAX_CLIENT_FOR_SERVICE) {
		LOG_ERR("[Utente %d] Queue position overflow for service %d", getpid(), utente.requested_service);
		shmdt(queue);
		exit(EXIT_FAILURE);
	}
	lock_semaphore(QUEUE_SEMAPHORE_KEY);
	queue->ticket_queue[utente.requested_service][pos] = msg.ticket_number;
	queue->queue_size[utente.requested_service]++;
	queue->served[client_index] = 0;
	lock_semaphore(QUEUE_SEMAPHORE_KEY);

	unlock_semaphore(utente.requested_service);

	LOG_INFO("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[utente.requested_service]);


	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	int next_day = sim_time->current_day+1;
	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	struct timespec start, end;
	// get current time before loop
	clock_gettime(CLOCK_MONOTONIC, &start);

	while (queue->ticket_queue[utente.requested_service][pos] != -1) {//while the ticket is being served



		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);

		int current_day = sim_time->current_day;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		//if the day has ended we need to exit
		if (current_day == next_day) {

			shmdt(queue);
			shmdt(direttore);
			shmdt(ticket_machine);
			LOG_INFO("[UTENTE %d]has exited the office.\n", getpid());
			break;
		}
	} //let the client wait till he's served

	// get current time after loop
	clock_gettime(CLOCK_MONOTONIC, &end);

	double elapsed = (end.tv_sec - start.tv_sec) +
				 (end.tv_nsec - start.tv_nsec) / 1e9;

	lock_semaphore(STATISTIC_SEMAPHORE_KEY);
	stats->total_waiting_time += (elapsed - msg.estimated_time);
	unlock_semaphore(STATISTIC_SEMAPHORE_KEY);

	LOG_INFO("[Utente %d] HAS BEEN SERVED\n", getpid());


	shmdt(queue);
	shmdt(direttore);
	shmdt(ticket_machine);
	return 0;
}
