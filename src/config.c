#include "config.h"
#include <string.h>

int NOF_WORKERS = DEFAULT_NOF_WORKERS;
int NOF_USERS = DEFAULT_NOF_USERS;
int NOF_WORKER_SEATS = DEFAULT_NOF_WORKER_SEATS;
int SIM_DURATION = DEFAULT_SIM_DURATION;
int N_NANO_SECS = DEFAULT_N_NANO_SECS;
int EXPLODE_THRESHOLD = DEFAULT_EXPLODE_THRESHOLD;
double P_SERV_MIN = DEFAULT_P_SERV_MIN;
double P_SERV_MAX = DEFAULT_P_SERV_MAX;
const int SERVICE_TIME[NUM_SERVICES] = {10, 8, 6, 8, 20, 20};

void load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file, using default values");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *equal_sign = strchr(line, '='); // Find '=' in the line
        if (!equal_sign) continue; // Skip lines without '='

        *equal_sign = '\0'; // Splits string at '=' (replaces it with null terminator)
        char *key = line;
        char *value = equal_sign + 1; // Moves to the value part

        // Remove trailing newline if present
        value[strcspn(value, "\r\n")] = 0;

        if (strcmp(key, "NOF_WORKERS") == 0)
            NOF_WORKERS = atoi(value);
        else if (strcmp(key, "NOF_USERS") == 0)
            NOF_USERS = atoi(value);
        else if (strcmp(key, "NOF_WORKER_SEATS") == 0)
            NOF_WORKER_SEATS = atoi(value);
        else if (strcmp(key, "SIM_DURATION") == 0)
            SIM_DURATION = atoi(value);
        else if (strcmp(key, "N_NANO_SECS") == 0)
            N_NANO_SECS = atoi(value);
        else if (strcmp(key, "P_SERV_MIN") == 0)
            P_SERV_MIN = atof(value);
        else if (strcmp(key, "P_SERV_MAX") == 0)
            P_SERV_MAX = atof(value);
        else if (strcmp(key, "EXPLODE_THRESHOLD") == 0)
            EXPLODE_THRESHOLD = atoi(value);
    }

    fclose(file);
}




