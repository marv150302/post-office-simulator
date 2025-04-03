#include <stdio.h>
#include "statistiche.h"
#include "semaphore_utils.h"



void initialize_stats(Stats *stats) {
    lock_semaphore(STATISTIC_SEMAPHORE_KEY);

    stats->served_clients_total = 0;
    stats->services_offered_total = 0;
    stats->services_not_offered_total = 0;

    stats->total_waiting_time = 0.0;
    stats->total_serving_time = 0.0;

    stats->active_operators_today = 0;
    stats->active_operators_total = 0;

    stats->breaks_today = 0;
    stats->breaks_total = 0;

    for (int i = 0; i < MAX_SPORTELLI; i++) {
        stats->operator_to_sportello_ratio_today[i] = 0.0f;
    }

    for (int i = 0; i < NUM_SERVICES; i++) {
        stats->per_service[i].served_clients_total = 0;
        stats->per_service[i].total_waiting_time = 0;
        stats->per_service[i].total_serving_time = 0;
    }

    unlock_semaphore(STATISTIC_SEMAPHORE_KEY);
}