//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef EROGATORE_TICKET_H
#define EROGATORE_TICKET_H

// Define message structure for requests and responses
typedef struct {
    long msg_type; //the type of message received by the erogator(in our case a request of service)
    int service_type; //the actual service requested
} TicketRequest;

typedef struct {
    long msg_type;
    int ticket_number;
    pid_t pid;
    int estimated_time;
} TicketMessage;
#endif //EROGATORE_TICKET_H
