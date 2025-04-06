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
    fprintf(output, "\n==========  STATISTICS - DAY %d ==========\n\n", current_day);

    fprintf(output, " General Stats:\n");
    fprintf(output, "  - Total Clients Served         : %d\n", stats->served_clients_total);
    fprintf(output, "  - Total Services Offered        : %d\n", stats->services_offered_total);
    fprintf(output, "  - Services Not Offered Today   : %d\n", stats->services_not_offered_total);

    fprintf(output, "\n️ Time Stats:\n");
    fprintf(output, "  - Avg Client Waiting Time      : %.2f min\n",
            stats->served_clients_total > 0 ? stats->total_waiting_time / stats->served_clients_total : 0.0);
    fprintf(output, "  - Avg Service Duration         : %.2f min\n",
            stats->served_clients_total > 0 ? stats->total_serving_time / stats->served_clients_total : 0.0);

    fprintf(output, "\n Operator Stats:\n");
    fprintf(output, "  - Active Operators Today       : %d\n", stats->active_operators_today);
    fprintf(output, "  - Total Active Operators       : %d\n", stats->active_operators_total);
    fprintf(output, "  - Breaks Taken Today           : %d\n", stats->breaks_today);
    fprintf(output, "  - Total Breaks in Simulation   : %d\n", stats->breaks_total);

    fprintf(output, "\n Operator-to-Sportello Ratio:\n");
    for (int i = 0; i < MAX_SPORTELLI; ++i) {
        fprintf(output, "  - Sportello %d                 : %.2f\n", i, stats->operator_to_sportello_ratio_today[i]);
    }

    fprintf(output, "\n Per-Service Statistics:\n");
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




}

void write_statistics_to_file(Stats *stats) {

  // === CSV Logging (column-oriented) ===
    FILE *csv = fopen("./csv_logs/statistiche.csv", "a");
    if (csv) {
        static int csv_header_written = 0;

        if (!csv_header_written && ftell(csv) == 0) {
            fprintf(csv, "Day,TotalClientsServed,TotalServicesOffered,ServicesNotOffered,");
            fprintf(csv, "AvgClientWaitTime,AvgServiceTime,ActiveOperatorsToday,TotalActiveOperators,BreaksToday,BreaksTotal,");

            // Sportello headers
            for (int i = 0; i < MAX_SPORTELLI; ++i) {
                fprintf(csv, "Sportello%d_Ratio,", i);
            }

            // Per-service stats
            for (int i = 0; i < NUM_SERVICES; ++i) {
                fprintf(csv, "%s_Served,%s_AvgWait,%s_AvgService,", SERVICE_NAMES[i], SERVICE_NAMES[i], SERVICE_NAMES[i]);
            }

            fprintf(csv, "\n");
            csv_header_written = 1;
        }

        // Write values
        fprintf(csv, "%d,%d,%d,", stats->served_clients_total, stats->services_offered_total, stats->services_not_offered_total);

        double avg_wait = stats->served_clients_total > 0 ? stats->total_waiting_time / stats->served_clients_total : 0.0;
        double avg_service = stats->served_clients_total > 0 ? stats->total_serving_time / stats->served_clients_total : 0.0;

        fprintf(csv, "%.2f,%.2f,", avg_wait, avg_service);
        fprintf(csv, "%d,%d,%d,%d,", stats->active_operators_today, stats->active_operators_total,
                stats->breaks_today, stats->breaks_total);

        // Sportello data
        for (int i = 0; i < MAX_SPORTELLI; ++i) {
            fprintf(csv, "%.2f,", stats->operator_to_sportello_ratio_today[i]);
        }

        // Per-service data
        for (int i = 0; i < NUM_SERVICES; ++i) {
            const StatisticByService *svc = &stats->per_service[i];
            double svc_wait = svc->served_clients_total > 0 ? svc->total_waiting_time / svc->served_clients_total : 0.0;
            double svc_serv = svc->served_clients_total > 0 ? svc->total_serving_time / svc->served_clients_total : 0.0;

            fprintf(csv, "%d,%.2f,%.2f,", svc->served_clients_total, svc_wait, svc_serv);
        }

        fprintf(csv, "\n");
        fclose(csv);
    } else {
        perror("Failed to open statistics CSV file");
    }
}