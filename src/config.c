#include "config.h"
#include "../libs/cJSON/cJSON.h"
#include <string.h>
#include <stdlib.h>

int NOF_WORKERS = 0;
int NOF_USERS = 0;
int NOF_WORKER_SEATS = 0;
int SIM_DURATION = 0;
int N_NANO_SECS = 0;
int EXPLODE_THRESHOLD = 0;
double P_SERV_MIN = 0.0;
double P_SERV_MAX = 0.0;

const int SERVICE_TIME[NUM_SERVICES] = {10, 8, 6, 8, 20, 20};

void load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file, using default values");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *json_data = malloc(length + 1);
    if (!json_data) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }

    fread(json_data, 1, length, file);
    json_data[length] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        free(json_data);
        return;
    }

    cJSON *val;

    if ((val = cJSON_GetObjectItem(root, "NOF_WORKERS")) && cJSON_IsNumber(val))
        NOF_WORKERS = val->valueint;

    if ((val = cJSON_GetObjectItem(root, "NOF_USERS")) && cJSON_IsNumber(val))
        NOF_USERS = val->valueint;

    if ((val = cJSON_GetObjectItem(root, "NOF_WORKER_SEATS")) && cJSON_IsNumber(val))
        NOF_WORKER_SEATS = val->valueint;

    if ((val = cJSON_GetObjectItem(root, "SIM_DURATION")) && cJSON_IsNumber(val))
        SIM_DURATION = val->valueint;

    if ((val = cJSON_GetObjectItem(root, "N_NANO_SECS")) && cJSON_IsNumber(val))
        N_NANO_SECS = val->valueint;

    if ((val = cJSON_GetObjectItem(root, "P_SERV_MIN")) && cJSON_IsNumber(val))
        P_SERV_MIN = val->valuedouble;

    if ((val = cJSON_GetObjectItem(root, "P_SERV_MAX")) && cJSON_IsNumber(val))
        P_SERV_MAX = val->valuedouble;

    if ((val = cJSON_GetObjectItem(root, "EXPLODE_THRESHOLD")) && cJSON_IsNumber(val))
        EXPLODE_THRESHOLD = val->valueint;

    cJSON_Delete(root);
    free(json_data);


}
