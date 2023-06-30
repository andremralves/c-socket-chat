#include <string.h>
#include <time.h>

#include "utils.h"

char *get_current_time() {
    time_t curr_time = time(NULL);
    return strtok(ctime(&curr_time), "\n");
}

