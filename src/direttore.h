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
//helper function to start all processes
void start_all_processes();
//helper function to kill all processe
void kill_all_processes();
//helper function to initialize all processes
void initialize_all_semaphores(void);
//helper function to clean up all semaphores
void cleanup_all_semaphores(void);

#endif //DIRETTORE_H
