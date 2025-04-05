//
// Created by Marvel  Asuenimhen  on 04/04/25.
//

#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include "direttore.h"
#include "semaphore_utils.h"

//function to create and start a process
pid_t start_process(const char *name, const char *path, int arg, Direttore* direttore);
#endif //PROCESS_UTILS_H
