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

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {

    printf("Received signal %d, shutting down [TICKET EROGATOR]\n", sig);
    running = 0;
}

int main() {

    signal(SIGTERM, handle_sigterm);
    //create and attach shared memory for ticket system
    int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
    TicketSystem *tickets = (TicketSystem *)attach_shared_memory(shmid_erogatore, "Erogatore");

    srand((unsigned) time(NULL) ^ getpid());


    for (int i = 0; i < NUM_SERVICES; i++) {
        tickets->ticket_number[i] = 1;
    }

    // Create message queue for ticket requests
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Message queue creation failed");
        exit(EXIT_FAILURE);
    }

    printf("[Erogatore] Ticket system initialized. Waiting for user requests...\n");

    while (running) {
        // Wait for a user request
        TicketRequest request;
        if (msgrcv(msgid, &request, sizeof(TicketRequest) - sizeof(long), 10, 0) == -1) {
            perror("Message receive failed");
            exit(EXIT_FAILURE);
        }

        int service_type = request.msg_type - 1;  // Convert message type to index
        int ticket_number = tickets->ticket_number[service_type]++;
        int estimated_time = SERVICE_TIME[service_type] + (rand() % (SERVICE_TIME[service_type] / 2)) - (SERVICE_TIME[service_type] / 4);

        // Create and send ticket message
        TicketMessage msg;
        msg.msg_type = service_type + 1;
        msg.ticket_number = ticket_number;
        msg.estimated_time = estimated_time;

        if (msgsnd(msgid, &msg, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
            perror("Message send failed");
            exit(EXIT_FAILURE);
        }

        printf("[Erogatore] Issued ticket %d for service %d (Estimated time: %d min)\n",
               ticket_number, service_type, estimated_time);
    }

    shmdt(tickets);
    return 0;
}
