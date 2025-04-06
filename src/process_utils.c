//
// Created by Marvel  Asuenimhen  on 04/04/25.
//

#include "process_utils.h"


pid_t start_process(const char *name, const char *path, int arg, Direttore* direttore) {
	pid_t pid = fork();
	if (pid < 0) {
		LOG_ERR("Fork failed\n");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
		char arg_str[10];
		snprintf(arg_str, sizeof(arg_str), "%d", arg);
		execl(path, name, arg_str, "--from-direttore", NULL);
		perror("Exec failed");
		exit(EXIT_FAILURE);
	}

	lock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	// save PID
	if (direttore->child_proc_count < MAX_CHILDREN) {

		direttore->child_pids[direttore->child_proc_count++] = pid;

	}
	unlock_semaphore(DIRETTORE_SEMAPHORE_KEY);
	return pid;
}
