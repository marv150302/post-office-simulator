//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#ifndef DIRETTORE_H
#define DIRETTORE_H

#include <stdlib.h>

#define MAX_CHILDREN 100
pid_t child_pids[MAX_CHILDREN];
int child_count = 0;

//function to create and start a process
pid_t start_process(const char *name, const char *path, int arg);

#endif //DIRETTORE_H
