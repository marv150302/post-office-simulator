//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef SPORTELLO_H
#define SPORTELLO_H
#include "config.h"

// Structure for sportelli (counters)
typedef struct {
    int service_type[MAX_SPORTELLI]; // each counter is assigned to a specific service
    int available[MAX_SPORTELLI];    // 1 = available, 0 = occupied
    int assigned_operator[MAX_SPORTELLI];
    int ready[MAX_SPORTELLI];
    int sportelli_ready; //flag to indicate if the counter is ready
    int assigned_operator_count;
} SportelloStatus;

#endif //SPORTELLO_H
