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


volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
	LOG_WARN("[Utente %d] Received signal %d, shutting down\n", getpid(), sig);
	running = 0;
}

int main(int argc, char *argv[]) {
	signal(SIGTERM, handle_sigterm);
	signal(SIGILL, handle_sigterm);

	attach_sim_time();
	srand(time(NULL) ^ getpid());

	load_config("config/config.json");

	if (should_stay_home()) return 0;

	int shmid_direttore = get_shared_memory(DIRETTORE_SHM_KEY, "Direttore");
	Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");


	if (argc == 3 && strcmp(argv[2], "--from-direttore") == 0) {
		LOG_WARN("Client Launched by direttore");
	} else {
		LOG_WARN("Client Launched manually from terminal");

		// save PID

		lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		if (direttore->child_proc_count < MAX_CHILDREN) {
			direttore->child_pids[direttore->child_proc_count++] = getpid();
			//unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		}
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	}

	int shmid_queue = get_shared_memory(QUEUE_SHM_KEY, "WaitingQueue");
	WaitingQueue *queue = (WaitingQueue *) attach_shared_memory(shmid_queue, "WaitingQueue");

	int shmid_erogatore = get_shared_memory(TICKET_EROGATOR_SHM_KEY, "Erogatore");
	TicketSystem *ticket_machine = (TicketSystem *) attach_shared_memory(shmid_erogatore, "Erogatore");

	int shmid_sportello = get_shared_memory(SPORTELLO_SHM_KEY, "Sportello");
	SportelloStatus *sportello = (SportelloStatus *) attach_shared_memory(shmid_sportello, "Sportello");

	int shmid_stats = get_shared_memory(STATISTIC_SHM_KEY, "Statistics");
	Stats *stats = (Stats *) attach_shared_memory(shmid_stats, "Statistics");

	Utente utente;
	int min_hour = sim_time->current_hour;
	int max_hour = CLOSING_HOUR - 1; // last valid full hour


	// Random hour from current to closing
	generate_appointment(&utente, min_hour, max_hour);
	wait_until_appointment(&utente);

	utente.N_REQUEST = 1 + rand() % N_REQUESTS;
	utente.requests = malloc(sizeof(int) * utente.N_REQUEST);

	for (int i = 0; i < utente.N_REQUEST; i++) {
		utente.requests[i] = rand() % NUM_SERVICES; //getting the user requests
	}
	utente.requested_service = rand() % NUM_SERVICES; // randomly select a service

	LOG_WARN("[UTENTE %d]  WILL REQUEST : [%d] SERVICES", getpid(), utente.N_REQUEST);
	//chek if the services you requested are handled
	for (int j = 0; j < utente.N_REQUEST; j++) {
		int requested_service = utente.requests[j];
		int service_offered = 0;
		lock_semaphore(SPORTELLO_SEMAPHORE_KEY);
		for (int i = 0; i < NOF_WORKER_SEATS; i++) {
			if (sportello->service_type[i] == requested_service) {
				service_offered = 1;
			};
		}
		unlock_semaphore(SPORTELLO_SEMAPHORE_KEY);

		if (!service_offered) {
			lock_semaphore(STATISTIC_SEMAPHORE_KEY);
			stats->services_not_offered_total++;
			unlock_semaphore(STATISTIC_SEMAPHORE_KEY);
			LOG_WARN("The Service %s is not handled today, try tomorrow!", SERVICE_NAMES[requested_service]);
		}
	}


	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	if (direttore->client_count >= MAX_CLIENTS) {
		LOG_ERR("[Utente %d] MAX_CLIENTS (%d) reached. Exiting...\n", getpid(), MAX_CLIENTS);
		unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
		exit(EXIT_FAILURE);
	}

	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);

	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	int next_day = sim_time->current_day + 1;
	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	handle_service_requests(utente, queue, ticket_machine, stats, next_day);

	cleanup(utente, queue, ticket_machine, stats);
	return 0;
}


void handle_service_requests(Utente utente, WaitingQueue *queue, TicketSystem *ticket_machine,
                             Stats *stats, int next_day) {
	for (int i = 0; i < utente.N_REQUEST && running; i++) {
		int requested_service = utente.requests[i];

		lock_semaphore(requested_service);
		lock_semaphore(QUEUE_SEMAPHORE_KEY);

		int pos = queue->queue_size[requested_service];
		if (pos >= MAX_CLIENT_FOR_SERVICE) {
			LOG_ERR("[Utente %d] Queue overflow for service %d", getpid(), requested_service);
			unlock_semaphore(QUEUE_SEMAPHORE_KEY);
			unlock_semaphore(requested_service);
			continue;
		}

		TicketMessage msg = get_ticket(requested_service, ticket_machine);

		queue->ticket_queue[requested_service][pos] = msg.ticket_number;
		queue->queue_size[requested_service]++;
		queue->served[requested_service][pos] = 0;

		unlock_semaphore(QUEUE_SEMAPHORE_KEY);
		unlock_semaphore(requested_service);

		LOG_INFO("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[requested_service]);

		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);

		while (running && !queue->served[requested_service][pos]) {
			lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
			int current_day = sim_time->current_day;
			unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

			if (current_day >= next_day) {
				LOG_INFO("[UTENTE %d] Exited because the office closed\n", getpid());
				lock_semaphore(STATISTIC_SEMAPHORE_KEY);
				stats->services_not_offered_total++;
				unlock_semaphore(STATISTIC_SEMAPHORE_KEY);
				running = 0;
				//cleanup(utente, queue, direttore, ticket_machine);
				return;
			}
			usleep(100000);
		}

		LOG_INFO("[Utente %d] HAS BEEN SERVED\n", getpid());

		clock_gettime(CLOCK_MONOTONIC, &end);

		double elapsed = (end.tv_sec - start.tv_sec) +
		                 (end.tv_nsec - start.tv_nsec) / 1e9;
		elapsed /= 60;

		lock_semaphore(STATISTIC_SEMAPHORE_KEY);
		stats->total_waiting_time += elapsed;
		stats->per_service[requested_service].total_waiting_time += elapsed;
		unlock_semaphore(STATISTIC_SEMAPHORE_KEY);

		lock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
		ticket_machine->nof_clients_waiting--;
		unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
	}

	//cleanup(utente, queue, direttore, ticket_machine);
}

bool should_stay_home() {
	attach_sim_time();
	load_config("config/config.json");

	double p_serv = P_SERV_MIN + ((double) rand() / RAND_MAX) * (P_SERV_MAX - P_SERV_MIN);
	const double roll = (double) rand() / RAND_MAX;

	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	if (roll >= p_serv || (sim_time->current_hour == CLOSING_HOUR - 1 && sim_time->current_minute >= 40)) {
		LOG_WARN(" [UTENTE %d] Is staying home", getpid());
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		return true;
	}
	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	return false;
}

void generate_appointment(Utente *utente, int min_hour, int max_hour) {
	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	utente->arrival_hour = min_hour + rand() % (max_hour - min_hour + 1);
	// If user will arrive in the same hour, we make sure the minute is in the future
	if (utente->arrival_hour == sim_time->current_hour) {
		utente->arrival_minute = sim_time->current_minute + rand() % (60 - sim_time->current_minute);
	} else {
		utente->arrival_minute = rand() % 60;
	}

	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

	char minute_buf[3]; // made of 2 digits + null terminator
	snprintf(minute_buf, sizeof(minute_buf), "%02d", utente->arrival_minute);
	LOG_WARN("UTENTE[%d] Booked an appointment for %d:%s", getpid(), utente->arrival_hour, minute_buf);
}

void wait_until_appointment(Utente *utente) {


	lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	int current_day = sim_time->current_day;
	int next_day = current_day + 1;
	unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);
	//wait for your appointment
	do {
		lock_semaphore(SIM_TIME_SEMAPHORE_KEY);
		int hour = sim_time->current_hour;
		int minute = sim_time->current_minute;
		current_day = sim_time->current_day;
		unlock_semaphore(SIM_TIME_SEMAPHORE_KEY);

		if (current_day >= next_day) {
			running = 0;

			break;
		}
		if (hour < utente->arrival_hour ||
		    (hour == utente->arrival_hour && minute < utente->arrival_minute)) {
			sleep(1);
		} else {
			break;
		}
	} while (running);
}

TicketMessage get_ticket(int service, TicketSystem *ticket_machine) {
	TicketMessage msg;

	TicketMessage req;
	req.msg_type = 10;
	req.service_type = service;
	req.pid = getpid();


	int msgid = msgget(MSG_KEY, 0666);
	if (msgid == -1) {
		LOG_ERR("Message get failed for [UTENTE %d]", getpid());
		exit(EXIT_FAILURE);
	}

	LOG_INFO("[Utente %d] Requesting ticket for Service: [%s]\n", getpid(), SERVICE_NAMES[service]);
	if (msgsnd(msgid, &req, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
		LOG_ERR("Message send failed");
		exit(EXIT_FAILURE);
	}


	wait_for_ticket_machine(ticket_machine); //wait for the ticket machine to be free


	if (msgrcv(msgid, &msg, sizeof(TicketMessage) - sizeof(long), getpid(), 0) == -1) {
		release_ticket_machine(ticket_machine);
		LOG_ERR("Ticket retrieval failed for [UTENTE %d]", getpid());
		exit(EXIT_FAILURE);
	}

	LOG_INFO("[Utente %d] Received Ticket: %d for Service: [%s] (Estimated time: %d min)\n",
	         getpid(), msg.ticket_number, SERVICE_NAMES[service], msg.estimated_time);

	release_ticket_machine(ticket_machine);
	unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
	return msg;
}

void wait_for_ticket_machine(TicketSystem *tm) {
	while (running) {
		lock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
		if (tm->in_use == 0) {
			tm->in_use = 1;
			unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
			return;
		}
		unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
		usleep(100000); // 100 ms
	}
}

void release_ticket_machine(TicketSystem *tm) {
	lock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
	tm->in_use = 0;
	unlock_semaphore(TICKET_EROGATOR_SEMAPHORE_KEY);
}

void cleanup(Utente utente, void *queue, void *direttore, void *ticket_machine) {



	if (utente.requests!=NULL)free(utente.requests);
	detach_shared_memory(queue);
	detach_shared_memory(direttore);
	detach_shared_memory(ticket_machine);
}
