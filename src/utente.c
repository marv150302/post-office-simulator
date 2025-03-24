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

    load_config("config/config.json");

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

    if (queue->queue_size[service_type] >= 50) {
        fprintf(stderr, "[Utente %d] Queue for service %d is full. Exiting...\n", getpid(), service_type);
        return EXIT_FAILURE;
    }

    // request ticket
    int msgid_send = msgget(MSG_KEY, 0666);
    if (msgid_send == -1) {

      perror("Message get failed for Customer");
      exit(EXIT_FAILURE);
    }

    printf("[Utente %d] Requesting ticket for service %d...\n", getpid(), service_type);
    TicketRequest req;
    req.msg_type = service_type;

    if(msgsnd(msgid_send, &req, sizeof(TicketRequest) - sizeof(long), 0) == -1) {
      perror("Message send failed");
      exit(EXIT_FAILURE);
    }


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

    printf("[Utente %d] Received Ticket: %d for Service: [%s] (Estimated time: %d min) - \n",
           getpid(), msg.ticket_number, SERVICE_NAMES[service_type], msg.estimated_time);

    // Add ticket to the queue
    int pos = queue->queue_size[service_type]; //get the queue number for the required service
    queue->ticket_queue[service_type][pos] = msg.ticket_number; // add the ticket number for the service
    queue->queue_size[service_type]++; // increase the queue size for the service
    queue->ticket_queue[service_type][0]++;

    //printf("Queue size %d.\n", queue->queue_size[service_type]);

    //printf("[Utente %d] To be served at [COUNTER: %d] for [SERVICE: %d] ...\n", getpid(), service_type);
    printf("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);

    shmdt(queue);
    return 0;
}
