//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#ifndef DIRETTORE_H
#define DIRETTORE_H

#include <stdlib.h>
//to track the children processes
#define MAX_CHILDREN 100
#define DIRETTORE_SHM_KEY 5678  // message Queue Key

typedef struct {

	pid_t child_pids[MAX_CHILDREN];
	int child_proc_count; //number of child processes
	int operator_count; //number of operators
	int client_count; //number of clients
	int killed_added_users;
} Direttore;

typedef enum {
	TERMINATION_NONE,
	TERMINATION_TIMEOUT,
	TERMINATION_EXPLODE
} TerminationReason;

//helper function to start all processes
void start_all_processes(Direttore* direttore);
//helper function to kill all processe
void kill_all_processes(Direttore* direttore);
//helper function to initialize all processes
void initialize_all_semaphores(void);
//helper function to clean up all semaphores
void cleanup_all_semaphores(void);
//function to assign a service to operators
void assign_service_to_sportello(void*);


#endif //DIRETTORE_H
