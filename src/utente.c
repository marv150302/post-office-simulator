#include "config.h"
#include "erogatore_ticket.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

int main() {

    srand(time(NULL) ^ getpid()); //allows for a better range of random numbers
    // Attach to the shared queue memory
    int shmid = shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), 0666);
    if (shmid == -1) {
        perror("Shared memory access failed");
        exit(EXIT_FAILURE);
    }
    WaitingQueue *queue = (WaitingQueue *)shmat(shmid, NULL, 0);
    if (queue == (void *)-1) {
        perror("Shared memory attach failed");
        exit(EXIT_FAILURE);
    }

    // the user selects a service randomly
    int service_type = rand() % NUM_SERVICES;
    printf("[Utente %d] Requesting ticket for service %d...\n", getpid(), service_type);

    // Get ticket from message queue
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        perror("Message queue access failed");
        exit(EXIT_FAILURE);
    }

    TicketMessage msg;
    if (msgrcv(msgid, &msg, sizeof(TicketMessage) - sizeof(long), service_type + 1, 0) == -1) {
        perror("Message receive failed");
        exit(EXIT_FAILURE);
    }

    printf("[Utente %d] Received Ticket: %d for service %d (Estimated time: %d min)\n",
           getpid(), msg.ticket_number, service_type, msg.estimated_time);

    // Add ticket to the queue
    int pos = queue->queue_size[service_type]; //get the queue number for the required service
    queue->ticket_queue[service_type][pos] = msg.ticket_number; // add the ticket number for the service
    queue->queue_size[service_type]++; // increase the queue size for the service

    printf("[Utente %d] Waiting in line for service %d...\n", getpid(), service_type);

    shmdt(queue);
    return 0;
}
