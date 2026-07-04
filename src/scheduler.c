#include "scheduler.h"
#include "logger.h"
#include <stdio.h>

void propagate(int finished_idx) {
    Job *parent = &jobs[finished_idx];

    for (ListNode *cn = parent->children; cn != NULL; cn = cn->next) {
        int c = cn->value;
        Job *child = &jobs[c];

        if (child->status != PENDING) {
            /* The child is already terminal (e.g., SKIPPED via another failed parent) */
            continue;
        }

        if (parent->status == SUCCESS) {
            child->unresolved_deps--;
            if (child->unresolved_deps == 0) {
                queue_push(&ready_queue, c); /* Push child to ready queue when all dependencies succeed */
            }
        } else {
            /* Parent is FAILED or SKIPPED -> child is automatically SKIPPED */
            child->status = SKIPPED;
            jobs_remaining--;
            log_write_skipped(c);
            propagate(c); /* Recursively skip descendants */
        }
    }
}

void* scheduler_loop(void *arg) {
    (void)arg; /* unused */
    while (1) {
        int job_index;
        int shut = queue_pop_blocking(&completed_queue, &job_index);
        if (shut) {
            break; /* safety-net for shutdown signal */
        }

        jobs_remaining--;
        propagate(job_index);

        if (jobs_remaining == 0) {
            queue_shutdown(&ready_queue); /* signal all workers that execution is done */
            break;
        }
    }
    return NULL;
}
