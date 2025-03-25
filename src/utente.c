#include "config.h"
#include "erogatore_ticket.h"
#include "semaphore_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

int main() {
    load_config("config/config.json");
    srand(time(NULL) ^ getpid());

    // Attach to shared queue memory
    int shmid = shmget(QUEUE_SHM_KEY, sizeof(WaitingQueue), 0666);
    if (shmid == -1) {
        LOG_ERR("Shared memory for queue access failed");
        exit(EXIT_FAILURE);
    }

    WaitingQueue *queue = (WaitingQueue *)shmat(shmid, NULL, 0);
    if (queue == (void *)-1) {
        LOG_ERR("Shared memory for queue attach failed");
        exit(EXIT_FAILURE);
    }

    // randomly select a service
    int service_type = rand() % NUM_SERVICES;

    if (queue->queue_size[service_type] >= 50) {
        LOG_WARN("[Utente %d] Queue for service %d is full. Exiting...\n", getpid(), service_type);
        return EXIT_FAILURE;
    }

    // request ticket
    int msgid_send = msgget(MSG_KEY, 0666);
    if (msgid_send == -1) {
        LOG_ERR("Message get failed for Client");
        exit(EXIT_FAILURE);
    }

    LOG_INFO("[Utente %d] Requesting ticket for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);

    TicketRequest req;
    req.msg_type = 10;
    req.service_type = service_type;

    if (msgsnd(msgid_send, &req, sizeof(TicketRequest) - sizeof(long), 0) == -1) {
        LOG_ERR("Message send failed");
        exit(EXIT_FAILURE);
    }

    // Receive ticket response
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        LOG_ERR("Message queue access failed");
        exit(EXIT_FAILURE);
    }

    TicketMessage msg;
    if (msgrcv(msgid, &msg, sizeof(TicketMessage) - sizeof(long), service_type + 1, 0) == -1) {
        LOG_ERR("Message receive failed");
        exit(EXIT_FAILURE);
    }

    LOG_INFO("[Utente %d] Received Ticket: %d for Service: [%s] (Estimated time: %d min)\n",
             getpid(), msg.ticket_number, SERVICE_NAMES[service_type], msg.estimated_time);

    // add ticket to queue
    lock_semaphore(service_type);
    int pos = queue->queue_size[service_type];
    queue->ticket_queue[service_type][pos] = msg.ticket_number;
    queue->queue_size[service_type]++;
    queue->ticket_queue[service_type][0]++;
    unlock_semaphore(service_type);

    LOG_INFO("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);

    shmdt(queue);
    return 0;
}