//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef EROGATORE_TICKET_H
#define EROGATORE_TICKET_H

// Define message structure for requests and responses
typedef struct {
    long msg_type; // Service type requested
} TicketRequest;

typedef struct {
    long msg_type;
    int ticket_number;
    int estimated_time;
} TicketMessage;
#endif //EROGATORE_TICKET_H
