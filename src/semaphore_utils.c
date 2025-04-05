#include "semaphore_utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// Common helper for key
static int get_semaphore_key(int identifier) {

	return SEM_KEY_BASE + identifier;
}

void lock_semaphore(int identifier) {
	int key = get_semaphore_key(identifier);
	int semid = semget(key, 1, 0666);
	if (semid == -1) {
		LOG_ERR("lock_semaphore FOR KEY %d: semget failed", key);
		perror("");
		exit(EXIT_FAILURE);
	}

	struct sembuf op = {0, -1, 0};

	while (semop(semid, &op, 1) == -1) {
		if (errno == EINTR) {
			LOG_WARN("lock_semaphore: interrupted by signal, retrying...");
			continue;
		}
		LOG_ERR("lock_semaphore: semop failed");
		perror("");
		exit(EXIT_FAILURE);
	}
}

void unlock_semaphore(int identifier) {
	int key = get_semaphore_key(identifier);
	int semid = semget(key, 1, 0666);
	if (semid == -1) {
		LOG_ERR("unlock_semaphore FOR KEY %d: semget failed", key);
		perror("");
		exit(EXIT_FAILURE);
	}

	struct sembuf op = {0, 1, 0}; // increment
	if (semop(semid, &op, 1) == -1) {
		LOG_ERR("unlock_semaphore: semop failed");
		perror("");
		exit(EXIT_FAILURE);
	}
}

void initialize_semaphores(int identifier) {
	int key = get_semaphore_key(identifier);
	int semid = semget(key, 1, IPC_CREAT | 0666);
	if (semid == -1) {
		LOG_ERR("Semaphore creation failed for identifier %d (key %d)", identifier, key);
		perror("");
		exit(EXIT_FAILURE);
	}
	if (semctl(semid, 0, SETVAL, 1) == -1) {
		LOG_ERR("Semaphore initialization failed for identifier %d (key %d)", identifier, key);
		perror("");
		exit(EXIT_FAILURE);
	}
	LOG_WARN("[Semaphore] Initialized for key %d (identifier %d)", key, identifier);
}

void cleanup_semaphores(int identifier) {
	int key = get_semaphore_key(identifier);
	int semid = semget(key, 1, 0666);
	if (semid == -1) {
		LOG_WARN("[Semaphore] No semaphore found for key %d (identifier %d)", key, identifier);
		perror("semget");
		return;
	}

	if (semctl(semid, 0, IPC_RMID) == -1) {
		LOG_ERR("[Semaphore] Failed to remove semaphore for key %d (identifier %d)", key, identifier);
		perror("semctl IPC_RMID");
	} else {
		LOG_INFO("[Semaphore] Cleaned up for key %d (identifier %d)", key, identifier);
	}
}