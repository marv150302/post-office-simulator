#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

// Define message structure
typedef struct {
    long msg_type;
    int ticket_number;
    int estimated_time;
} TicketMessage;

int main() {
    srand(time(NULL)); // Initialize random seed

    // Create shared memory for ticket system
    int shmid = shmget(SHM_KEY, sizeof(TicketSystem), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Shared memory creation failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    TicketSystem *tickets = (TicketSystem *)shmat(shmid, NULL, 0);
    if (tickets == (void *)-1) {
        perror("Shared memory attach failed");
        exit(EXIT_FAILURE);
    }

    // Initialize ticket counters
    for (int i = 0; i < NUM_SERVICES; i++) {
        tickets->ticket_number[i] = 1;
    }

    // Create message queue
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Message queue creation failed");
        exit(EXIT_FAILURE);
    }

    printf("[Erogatore] Ticket system initialized. Waiting for users...\n");

    while (1) {
        // Simulate user requests for a random service
        int service_type = rand() % NUM_SERVICES;

        // Generate ticket number
        int ticket_number = tickets->ticket_number[service_type]++;

        // Generate random service time (±50% variation)
        int avg_time = SERVICE_TIME[service_type];
        int estimated_time = avg_time + (rand() % (avg_time / 2)) - (avg_time / 4);

        // Create ticket message
        TicketMessage msg;
        msg.msg_type = service_type + 1;
        msg.ticket_number = ticket_number;
        msg.estimated_time = estimated_time;

        // Send ticket to the message queue
        if (msgsnd(msgid, &msg, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
            perror("Message send failed");
            exit(EXIT_FAILURE);
        }

        printf("[Erogatore] Issued ticket %d for service %d (Estimated time: %d min)\n",
               ticket_number, service_type, estimated_time);

        sleep(1); // Simulate real-world delay
    }

    // Detach shared memory
    shmdt(tickets);

    return 0;
}
