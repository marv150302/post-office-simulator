//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/shm.h>
#include "memory_handler.h"
#include "config.h"  // Include your logging macros if not globally included

// function to create shared memory and return shmid
int create_shared_memory(key_t key, size_t size, const char *name) {


    //printf("[DEBUG] Requesting SHM for %s: size = %zu\n", name, size);
    int existing_shmid = shmget(key, 0, 0666);
    if (existing_shmid != -1) {
        shmctl(existing_shmid, IPC_RMID, NULL);
    }
    int shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        LOG_ERR("shared memory creation failed for [%s] : %s",name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

// function to attach shared memory and return pointer
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

// function to detach shared memory
void detach_shared_memory(void *shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        LOG_ERR("Shared memory detach failed");
        perror("");
        exit(EXIT_FAILURE);
    }
}

// function to remove shared memory
void remove_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        LOG_ERR("Shared memory remove failed");
        perror("");
        exit(EXIT_FAILURE);
    }
}

//function for cleaning the shared memory
void clean_shared_memory(int key, const char* name) {
    int shmid = shmget(key, 0, 0666);
    if (shmid == -1) {
        LOG_WARN("No shared memory found for key: %s", name);
        return;
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    } else {
        LOG_WARN("Shared memory removed successfully (NAME: %s).", name);
    }
}
int get_shared_memory(key_t key, const char *name){


  int shmid = shmget(key, 0, 0666);
    if (shmid == -1) {
        LOG_ERR("[%s] Failed to get shared memory", name);
        perror("");
        exit(EXIT_FAILURE);
    }
  return shmid;
}


void clean_message_queue(int key) {

    int msgid = msgget(key, 0666);
    if (msgid != -1) msgctl(msgid, IPC_RMID, NULL);
}