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

#include <stdio.h>
#include "statistiche.h"

void print_daily_stats(Stats *stats, int current_day, FILE *output) {


     for (int j = 0; j < NOF_WORKER_SEATS; j++) {

		stats->operator_to_sportello_ratio_today[j] = (float)stats->active_operators_today / (float)NOF_WORKER_SEATS;
     }

    //stats->services_not_offered_total = NUM_SERVICES - stats->services_offered_total;
    fprintf(output, "\n========== 📊 STATISTICS - DAY %d ==========\n\n", current_day);

    fprintf(output, "🧾 General Stats:\n");
    fprintf(output, "  - Total Clients Served         : %d\n", stats->served_clients_total);
    fprintf(output, "  - Total Services Offered        : %d\n", stats->services_offered_total);
    fprintf(output, "  - Services Not Offered Today   : %d\n", stats->services_not_offered_total);

    fprintf(output, "\n⏱️ Time Stats:\n");
    fprintf(output, "  - Avg Client Waiting Time      : %.2f min\n",
            stats->served_clients_total > 0 ? stats->total_waiting_time / stats->served_clients_total : 0.0);
    fprintf(output, "  - Avg Service Duration         : %.2f min\n",
            stats->served_clients_total > 0 ? stats->total_serving_time / stats->served_clients_total : 0.0);

    fprintf(output, "\n👥 Operator Stats:\n");
    fprintf(output, "  - Active Operators Today       : %d\n", stats->active_operators_today);
    fprintf(output, "  - Total Active Operators       : %d\n", stats->active_operators_total);
    fprintf(output, "  - Breaks Taken Today           : %d\n", stats->breaks_today);
    fprintf(output, "  - Total Breaks in Simulation   : %d\n", stats->breaks_total);

    fprintf(output, "\n🏢 Operator-to-Sportello Ratio:\n");
    for (int i = 0; i < MAX_SPORTELLI; ++i) {
        fprintf(output, "  - Sportello %d                 : %.2f\n", i, stats->operator_to_sportello_ratio_today[i]);
    }

    fprintf(output, "\n📌 Per-Service Statistics:\n");
    for (int i = 0; i < NUM_SERVICES; ++i) {
        const StatisticByService *svc = &stats->per_service[i];
        fprintf(output, "  [%s]\n", SERVICE_NAMES[i]);
        fprintf(output, "    - Clients Served             : %d\n", svc->served_clients_total);
        fprintf(output, "    - Avg Wait Time              : %.2f min\n",
                svc->served_clients_total > 0 ? svc->total_waiting_time / svc->served_clients_total : 0.0);
        fprintf(output, "    - Avg Service Time           : %.2f min\n",
                svc->served_clients_total > 0 ? svc->total_serving_time / svc->served_clients_total : 0.0);
    }

    fprintf(output, "\n===========================================\n\n");

    stats->active_operators_today = 0;
	stats->breaks_today = 0;
}