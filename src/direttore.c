#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void start_process(const char *name, const char *path) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execl(path, name, NULL);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    load_config("config/config.txt");

    printf("Starting simulation...\n");

    start_process("erogatore_ticket", "./bin/erogatore_ticket");

    for (int i = 0; i < NOF_WORKERS; i++) {
        start_process("operatore", "./bin/operatore");
    }

    for (int i = 0; i < NOF_USERS; i++) {
        start_process("utente", "./bin/utente");
    }

    for (int i = 0; i < NOF_WORKER_SEATS; i++) {
        start_process("sportello", "./bin/sportello");
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);

    printf("Simulation finished.\n");
    return 0;
}
