#ifndef STATISTICHE_H
#define STATISTICHE_H

#include "config.h"
#define  STATISTIC_SHM_KEY 4400


typedef struct {
	/**/
	int served_clients_total; // total clients served per service, operator
	float avg_clients_served_per_day; // average clients served per day (for this service)
	/**/
	float total_waiting_time; // sum of all wait times for this service for clients
	/**/
	double total_serving_time; // sum of all service durations for this service
	float avg_waiting_time_per_day;
	float avg_serving_time_per_day;
} StatisticByService;

typedef struct {

	int served_clients_total; //operator
	int services_offered_total; //sportello
	int services_not_offered_total; //num_services - service_offered_total
	//float avg_clients_served_per_day; //served_clients_total / simulation_days, direttore
	//float avg_services_offered_per_day; //services_offered_total / simulation_days, direttore
	//float avg_services_not_offered_per_day; //services_not_offered_total / simulation_days, direttore
	double total_waiting_time; // sum of wait times for all users, utente, used to calculate avg
	double total_serving_time; // sum of service times for all users, utente, used to calculate avg
	//float avg_waiting_time_per_day;
	//float avg_serving_time_per_day;
	int active_operators_today; //operator, needs to be reset by direttore at the beginneing of the day
	int active_operators_total; //operator,
	int breaks_today; //operator, needs to be reset by direttore at the beginneing of the day
	int breaks_total; //operator
	float operator_to_sportello_ratio_today[MAX_SPORTELLI];
	StatisticByService per_service[NUM_SERVICES];
} Stats;



void initialize_stats(Stats *);

void print_daily_stats(Stats *stats, int current_day, FILE *output);

void write_statistics_to_file(Stats *stats);
#endif //STATISTICHE_H
