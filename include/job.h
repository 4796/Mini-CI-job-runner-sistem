#ifndef JOB_H
#define JOB_H

#include "list_node.h"
#include "queue.h"

/* Default configuration constants */
#define DEFAULT_WATCHDOG_TIMEOUT_US (120U * 1000U * 1000U) /* 2 minutes in microseconds */
#define DEFAULT_WORKERS_COUNT 4

typedef enum {
    PENDING,
    RUNNING,
    SUCCESS,
    FAILED,
    SKIPPED
} JobStatus;

typedef struct {
    char  name[64];
    char  command[256];

    int  *dep_indices;      /* array of indices in jobs[] that this job depends on */
    int   n_deps;

    ListNode *children;     /* linked list (value = job_index) of jobs depending on this one */
    int   n_children;       /* optional, for reporting/debugging */

    int   unresolved_deps;  /* remaining unresolved dependencies; modified ONLY by scheduler */
    JobStatus status;       /* modified ONLY by scheduler (worker writes RUNNING/SUCCESS/FAILED) */

    char  log_path[256];    /* logs/<name>.log */
} Job;

/* Global shared state */
extern Job     *jobs;
extern int      n_jobs;
extern int      jobs_remaining;
extern JobQueue ready_queue;
extern JobQueue completed_queue;
extern pthread_mutex_t console_lock;

void free_jobs(Job *jobs, int n);

#endif /* JOB_H */
