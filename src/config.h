#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "shared_time.h"
#include "../libs/cJSON/cJSON.h"

#define NUM_SERVICES 6 // number of services provided
#define MAX_SPORTELLI 5 // maximum number of counters
#define MAX_CLIENT_FOR_SERVICE 10

#define SHM_KEY 1234  // shared Memory Key
#define MSG_KEY 5678  // message Queue Key

#define QUEUE_SHM_KEY 6789 // shared memory key for waiting queue
#define SEM_KEY_BASE 9000 // semaphore key


// Default values (used only if config file is missing)
#define DEFAULT_NOF_WORKERS 5
#define DEFAULT_NOF_USERS 10
#define DEFAULT_NOF_WORKER_SEATS 3
#define DEFAULT_SIM_DURATION 3
#define DEFAULT_BREAK_PROBABILITY 10
#define DEFAULT_N_NANO_SECS 20
#define DEFAULT_P_SERV_MIN 0.2
#define DEFAULT_P_SERV_MAX 0.8
#define DEFAULT_EXPLODE_THRESHOLD 50
#define DEFAULT_NOF_PAUSE 2
#define MAX_CLIENTS 50
#define MAX_SERVICE_NAME_LEN 64 //the max length of each service name

#define DIRETTORE_SEMAPHORE_KEY 100
#define OPERATORE_SEMAPHORE_KEY 200
#define SPORTELLO_SEMAPHORE_KEY 300
#define QUEUE_SEMAPHORE_KEY 400

#define SIM_TIME_FMT "[Day %02d - %02d:%02d] "
#define SIM_TIME_ARGS(i) ((i) / (24 * 60) + 1), (((i) / 60) % 24), ((i) % 60)


// Terminal color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"


#define LOG_INFO(fmt, ...) \
do { \
if (sim_time == NULL) attach_sim_time(); \
printf(COLOR_GREEN "[INFO] [Day %02d - %02d:%02d] " fmt COLOR_RESET "\n", \
sim_time ? sim_time->current_day : 0, \
sim_time ? sim_time->current_hour : 0, \
sim_time ? sim_time->current_minute : 0, \
##__VA_ARGS__); \
} while (0)

#define LOG_WARN(fmt, ...) \
do { \
if (sim_time == NULL) attach_sim_time(); \
printf(COLOR_YELLOW "[WARN] [Day %02d - %02d:%02d] " fmt COLOR_RESET "\n", \
sim_time ? sim_time->current_day : 0, \
sim_time ? sim_time->current_hour : 0, \
sim_time ? sim_time->current_minute : 0, \
##__VA_ARGS__); \
} while (0)

#define LOG_ERR(fmt, ...) \
do { \
if (sim_time == NULL) attach_sim_time(); \
fprintf(stderr, COLOR_RED "[ERROR] [Day %02d - %02d:%02d] " fmt COLOR_RESET "\n", \
sim_time ? sim_time->current_day : 0, \
sim_time ? sim_time->current_hour : 0, \
sim_time ? sim_time->current_minute : 0, \
##__VA_ARGS__); \
} while (0)

// structure for user queue
typedef struct {

    int ticket_queue[NUM_SERVICES][MAX_CLIENT_FOR_SERVICE]; // Queue for each service (max 10 users per service)
    int queue_size[NUM_SERVICES]; // Number of users waiting per service
    int served[MAX_CLIENTS];
} WaitingQueue;



// (will be set by `load_config()`)
extern int NOF_WORKERS;
extern int NOF_USERS;
extern int NOF_WORKER_SEATS;
extern int SIM_DURATION;
extern int N_NANO_SECS;
extern int EXPLODE_THRESHOLD;
extern int NOF_PAUSE;
extern double P_SERV_MIN;
extern double BREAK_PROBABILITY;
extern double P_SERV_MAX;
extern char SERVICE_NAMES[NUM_SERVICES][MAX_SERVICE_NAME_LEN];
extern int SERVICE_TIME[NUM_SERVICES];




void load_config(const char *filename);
void load_services(cJSON *root);

#endif // CONFIG_H
