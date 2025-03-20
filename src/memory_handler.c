//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "memory_handler.h"

// Function to create shared memory and return shmid
int create_shared_memory(key_t key, size_t size, const char *name) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "%s shared memory create failed: ", name);
        perror("");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

// Function to attach shared memory and return pointer
void* attach_shared_memory(int shmid, const char *name) {
    void *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        fprintf(stderr, "%s shared memory create failed: ", name);
        perror("");
    }
    //printf("%s shared memory attached successfully.\n", name);
    return shm_ptr;
}

//functio to detach shared memory
void detach_shared_memory(void *shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        perror("Shared memory detach failed");
        exit(EXIT_FAILURE);
    }
}

//function to remove shared memory
void remove_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Shared memory remove failed");
        exit(EXIT_FAILURE);
    }
}