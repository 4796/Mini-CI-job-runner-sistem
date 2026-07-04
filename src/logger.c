#include "logger.h"
#include <stdio.h>

void log_write_skipped(int job_index) {
    Job *j = &jobs[job_index];
    FILE *f = fopen(j->log_path, "w");
    if (f != NULL) {
        fprintf(f, "SKIPPED: one or more dependencies did not succeed\n");
        fclose(f);
    }
}
