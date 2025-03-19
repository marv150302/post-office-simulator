#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

// number of services
#define NUM_SERVICES 6

// service types
enum ServiceType {
    SERVICE_PACKAGE = 0,             // Invio e ritiro pacchi
    SERVICE_LETTERS = 1,             // Invio e ritiro lettere e raccomandate
    SERVICE_BANK = 2,                // Prelievi e versamenti Bancoposta
    SERVICE_BILLS = 3,               // Pagamento bollettini postali
    SERVICE_FINANCIAL_PRODUCTS = 4,  // Acquisto prodotti finanziari
    SERVICE_JEWELRY = 5              // Acquisto orologi e braccialetti
};

// Average service processing times (minutes)
extern const int SERVICE_TIME[NUM_SERVICES];

// Structure to store ticket system in shared memory
typedef struct {
    int ticket_number[NUM_SERVICES];  // Ticket counters per service
} TicketSystem;

// Structure for user queue
typedef struct {
    int ticket_queue[NUM_SERVICES][50]; // Queue for each service (max 50 users per service)
    int queue_size[NUM_SERVICES]; // Number of users waiting per service
} WaitingQueue;

#define SHM_KEY 1234  // Shared Memory Key
#define MSG_KEY 5678  // Message Queue Key

// Default values (used only if config file is missing)
#define DEFAULT_NOF_WORKERS 5
#define DEFAULT_NOF_USERS 10
#define DEFAULT_NOF_WORKER_SEATS 3
#define DEFAULT_SIM_DURATION 3
#define DEFAULT_N_NANO_SECS 20
#define DEFAULT_P_SERV_MIN 0.2
#define DEFAULT_P_SERV_MAX 0.8
#define DEFAULT_EXPLODE_THRESHOLD 50

// Global variables (will be set by `load_config()`)
extern int NOF_WORKERS;
extern int NOF_USERS;
extern int NOF_WORKER_SEATS;
extern int SIM_DURATION;
extern int N_NANO_SECS;
extern int EXPLODE_THRESHOLD;
extern double P_SERV_MIN;
extern double P_SERV_MAX;

// Function prototype
void load_config(const char *filename);

#endif // CONFIG_H
