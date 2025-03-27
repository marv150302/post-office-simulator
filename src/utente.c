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
#include "direttore.h"
#include "memory_handler.h"
#include <string.h>

int main(int argc, char *argv[]) {

    srand(time(NULL) ^ getpid());
    int shmid_direttore = create_shared_memory(DIRETTORE_KEY, sizeof(Direttore), "Direttore");
    Direttore *direttore = (Direttore *) attach_shared_memory(shmid_direttore, "Direttore");


    if (argc > 2 && strcmp(argv[2], "--from-direttore") == 0) {
        LOG_WARN("Client Launched by direttore");
    } else {
        LOG_WARN("Client Launched manually from terminal");
        // Save PID
        if (direttore->child_proc_count < MAX_CHILDREN) {
            direttore->child_pids[direttore->child_proc_count++] = getpid();
        }
    }
    load_config("config/config.json");



    int shmid_erogatore = create_shared_memory(SHM_KEY, sizeof(TicketSystem), "Erogatore");
    TicketSystem *tickets = (TicketSystem *)attach_shared_memory(shmid_erogatore, "Erogatore");

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

    if (queue->queue_size[service_type] >= MAX_CLIENTS) {
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

    tickets->client_served[service_type][queue->ticket_queue[service_type][0]] = 0;

    //
    queue->served[direttore->client_count] = 0;
    direttore->client_count++;
    //
    unlock_semaphore(service_type);

    LOG_INFO("[Utente %d] Waiting in line for Service: [%s]\n", getpid(), SERVICE_NAMES[service_type]);

    while (tickets->client_served[service_type][queue->ticket_queue[service_type][0]] == 0) {
        printf("client: %d", tickets->client_served[service_type][queue->ticket_queue[service_type][0]]);
        sleep(1);

    } //let the client wait till he's served

    LOG_INFO("[Utente %d] has been served\n", getpid());

    //i need to add the case where a client has another service requirment
    shmdt(queue);
    return 0;
}