//
// Created by Marvel  Asuenimhen  on 31/03/25.
//

// shared_time.h
#ifndef SHARED_TIME_H
#define SHARED_TIME_H

#include "config.h"  // for SIM_TIME_KEY or define the key here
#define SHARED_TIME_KEY 1543 // simulation shared time memory key
typedef struct {
	int current_day;
	int current_hour;
	int current_minute;
} SharedTime;

extern SharedTime *sim_time;

void attach_sim_time();

#endif // SHARED_TIME_H