//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef EROGATORE_TICKET_H
#define EROGATORE_TICKET_H

//used  to send and receive ticket messages from "erogatore ticket"
typedef struct {

    long msg_type; //the type of message received by the erogator(in our case 10 for a request of service)
    int service_type; //the actual service requested
    int ticket_number;
    int estimated_time;
    pid_t pid;
} TicketMessage;

// structure to store ticket system in shared memory
typedef struct {

    int ticket_number[NUM_SERVICES];  // Ticket counters per service
    int in_use; // to allow one user at once to use the machine
} TicketSystem;

#endif //EROGATORE_TICKET_H
