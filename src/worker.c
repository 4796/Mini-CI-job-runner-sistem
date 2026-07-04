#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void* worker_loop(void *arg) {
    (void)arg; /* unused */
    while (1) {
        int job_index;
        int shut = queue_pop_blocking(&ready_queue, &job_index);
        if (shut) {
            break;
        }

        Job *j = &jobs[job_index];
        j->status = RUNNING;

        /* Prepare command with stdout and stderr redirected to the log file */
        char full_cmd[512];
        snprintf(full_cmd, sizeof(full_cmd), "%s > \"%s\" 2>&1", j->command, j->log_path);
        
        int raw_status = system(full_cmd);

        /* Decode exit status using standard POSIX macros */
        int success = (raw_status != -1) && WIFEXITED(raw_status) && (WEXITSTATUS(raw_status) == 0);
        j->status = success ? SUCCESS : FAILED;

        queue_push(&completed_queue, job_index);
    }
    return NULL;
}
