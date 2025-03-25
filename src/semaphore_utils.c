#include "semaphore_utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

void lock_semaphore(int service_type) {
	int semid = semget(SEM_KEY_BASE + service_type, 1, 0666);
	if (semid == -1) {
		LOG_ERR("lock_semaphore: semget failed\n");
		perror("");
		exit(EXIT_FAILURE);
	}

	struct sembuf op = {0, -1, 0}; // decrement
	if (semop(semid, &op, 1) == -1) {
		LOG_ERR("lock_semaphore: semop failed\n");
		perror("");
		exit(EXIT_FAILURE);
	}
}

void unlock_semaphore(int service_type) {
	int semid = semget(SEM_KEY_BASE + service_type, 1, 0666);
	if (semid == -1) {
		LOG_ERR("unlock_semaphore: semget failed\n");
		perror("");
		exit(EXIT_FAILURE);
	}

	struct sembuf op = {0, 1, 0}; // increment
	if (semop(semid, &op, 1) == -1) {
		LOG_ERR("unlock_semaphore: semop failed\n");
		perror("");
		exit(EXIT_FAILURE);
	}
}

void initialize_semaphores(int service_type) {
	int semid = semget(SEM_KEY_BASE + service_type, 1, IPC_CREAT | 0666);
	if (semid == -1) {
		LOG_ERR("Semaphore creation failed for service %d\n", service_type);
		perror("");
		exit(EXIT_FAILURE);
	}
	if (semctl(semid, 0, SETVAL, 1) == -1) {
		LOG_ERR("Semaphore initialization failed for service %d\n", service_type);
		perror("");
		exit(EXIT_FAILURE);
	}
	LOG_INFO("[Semaphore] Initialized for service %d\n", service_type);
}

void cleanup_semaphores(int service_type) {
	int semid = semget(SEM_KEY_BASE + service_type, 1, 0666);
	if (semid != -1) {
		semctl(semid, 0, IPC_RMID);
		LOG_INFO("[Semaphore] Cleaned up for service %d\n", service_type);
	}
}