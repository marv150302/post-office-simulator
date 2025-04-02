//
// Created by Marvel  Asuenimhen  on 31/03/25.
//

#include "shared_time.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

SharedTime *sim_time = NULL;


void attach_sim_time(void) {
	if (sim_time != NULL) return; // already attached

	int shmid = shmget(SHARED_TIME_KEY, sizeof(SharedTime),  0666 | IPC_CREAT);
	if (shmid == -1) {
		perror("[SharedTime] Failed to get shared memory");
		exit(EXIT_FAILURE);
	}

	sim_time = (SharedTime *) shmat(shmid, NULL, 0);
	if (sim_time == (void *) -1) {
		perror("[SharedTime] Failed to attach to shared memory");
		sim_time = NULL;
		exit(EXIT_FAILURE);
	}
}