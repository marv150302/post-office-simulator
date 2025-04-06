//
// Created by Marvel  Asuenimhen  on 02/04/25.
//

#ifndef UTENTE_H
#define UTENTE_H
#include <stdlib.h>
#include "direttore.h"
#include "sportello.h"
#include "erogatore_ticket.h"
#include "statistiche.h"
#include "memory_handler.h"
#include <stdbool.h>

typedef struct {
	int requested_service;
	int arrival_hour;
	int arrival_minute;
	int N_REQUEST;
	int *requests;
} Utente;

TicketMessage get_ticket(int service, TicketSystem *); //function to get the tickets
void wait_for_ticket_machine(TicketSystem *); // function to wait for ticket machine to be free
void release_ticket_machine(TicketSystem *); //function to release ticket machine


bool should_stay_home(void); //function to check wether the client stays home or not
void initialize_shared_memory(Utente *utente, Direttore **direttore,
                              WaitingQueue **queue, TicketSystem **ticket_machine,
                              SportelloStatus **sportello, Stats **stats);

void check_requested_services(Utente *utente, SportelloStatus *sportello, Stats *stats);

void generate_appointment(Utente *utente, int min_hour, int max_hour);

void wait_until_appointment(Utente *utente);

void handle_service_requests(Utente utente, WaitingQueue *queue, TicketSystem *ticket_machine,
							Stats *stats, int next_day); //function to handle service requets
void cleanup(Utente utente, void *queue, void *direttore, void *ticket_machine);
#endif //UTENTE_H
