//
// Created by Marvel  Asuenimhen  on 20/03/25.
//

#ifndef MEMORY_HANDLER_H
#define MEMORY_HANDLER_H

#include <sys/ipc.h>
#include <sys/shm.h>


int create_shared_memory(key_t key, size_t size, const char *name); //function to create or get memory id
void* attach_shared_memory(int shmid, const char *name); //function to attach memory
void detach_shared_memory(void *shm_ptr); //function to detach shared memory
void remove_shared_memory(int shmid); //function to remove shared memory
void clean_shared_memory(int key); //function for cleaning the shared memory
void clean_message_queue(int key); //function to clean message queue
#endif //MEMORY_HANDLER_H
