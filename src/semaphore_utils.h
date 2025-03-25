//
// Created by Marvel  Asuenimhen  on 24/03/25.
//

#ifndef SEMAPHORE_UTILS_H
#define SEMAPHORE_UTILS_H

#include <sys/sem.h>
#include "config.h"

// Lock (P operation)
void lock_semaphore(int service_type);

// Unlock (V operation)
void unlock_semaphore(int service_type);

void initialize_semaphores(int service_type);

void cleanup_semaphores(int service_type);

#endif //SEMAPHORE_UTILS_H
