//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "memory_handler.h"
#include "config.h"  // Include your logging macros if not globally included

// Function to create shared memory and return shmid
int create_shared_memory(key_t key, size_t size, const char *name) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        LOG_ERR("[%s] shared memory creation failed: ", name);
        perror("");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

// Function to attach shared memory and return pointer
void* attach_shared_memory(int shmid, const char *name) {
    void *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        LOG_ERR("[%s] shared memory attach failed: ", name);
        perror("");
    } else {
        LOG_INFO("[%s] shared memory attached successfully.\n", name);
    }
    return shm_ptr;
}

// Function to detach shared memory
void detach_shared_memory(void *shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        LOG_ERR("Shared memory detach failed");
        perror("");
        exit(EXIT_FAILURE);
    }
}

// Function to remove shared memory
void remove_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        LOG_ERR("Shared memory remove failed");
        perror("");
        exit(EXIT_FAILURE);
    }
}

//function for cleaning the shared memory
void clean_shared_memory(int key) {

    int shmid = shmget(key, 0, 0666);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
}

void clean_message_queue(int key) {

    int msgid = msgget(key, 0666);
    if (msgid != -1) msgctl(msgid, IPC_RMID, NULL);
}