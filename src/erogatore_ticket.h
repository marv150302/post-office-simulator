//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef EROGATORE_TICKET_H
#define EROGATORE_TICKET_H

//used  to send and receive ticket messages from "erogatore ticket"
typedef struct {

    long msg_type; //the type of message received by the erogator(in our case a request of service)
    int service_type; //the actual service requested
    int ticket_number;
    int estimated_time;
} TicketMessage;

// structure to store ticket system in shared memory
typedef struct {

    int ticket_number[NUM_SERVICES];  // Ticket counters per service
} TicketSystem;

#endif //EROGATORE_TICKET_H
