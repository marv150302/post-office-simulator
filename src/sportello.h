//
// Created by Marvel  Asuenimhen  on 19/03/25.
//

#ifndef SPORTELLO_H
#define SPORTELLO_H
#include "config.h"

#define SPORTELLO_SHM_KEY 7891 // counter's shared memory key
// Structure for sportelli (counters)
typedef struct {
    int service_type[MAX_SPORTELLI]; // Each counter is assigned to a specific service
    int available[MAX_SPORTELLI];    // 1 = available, 0 = occupied
    int assigned_operator[MAX_SPORTELLI];
} SportelloStatus;

#endif //SPORTELLO_H
