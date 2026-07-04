#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#include "job.h"
#include "parser.h"
#include "cycle_detect.h"
#include "graph.h"
#include "queue.h"
#include "scheduler.h"
#include "worker.h"
#include "report.h"
#include "utils/watchdog.h"

/* Define actual global shared variables declared in job.h */
Job     *jobs = NULL;
int      n_jobs = 0;
int      jobs_remaining = 0;
JobQueue ready_queue;
JobQueue completed_queue;

int main(int argc, char **argv) {
    /* Start watchdog at the very beginning to avoid any deadlocks or hangs */
    if (watchdog_start_exit_after_us(DEFAULT_WATCHDOG_TIMEOUT_US, 1) != 0) {
        fprintf(stderr, "Warning: failed to start deadlock watchdog thread\n");
    }

    /* 1. Validate command line arguments */
    if (argc != 2) {
        fprintf(stderr, "usage: MiniCI <config.yaml>\n");
        return 1;
    }

    /* 2. Parse configuration file (single-threaded) */
    int num_workers = DEFAULT_WORKERS_COUNT;
    if (parse_config(argv[1], &jobs, &n_jobs, &num_workers) != 0) {
        return 1;
    }

    /* 3. Detect cyclic dependencies before running any threads */
    if (graph_has_cycle(jobs, n_jobs)) {
        fprintf(stderr, "Error: cyclic dependency detected in pipeline configuration\n");
        free_jobs(jobs, n_jobs);
        return 1;
    }

    /* 4. Build children adjacency lists */
    build_children_lists(jobs, n_jobs);

    /* 4b. Create logs directory once, single-threaded */
    if (mkdir("logs", 0755) != 0 && errno != EEXIST) {
        perror("Error: cannot create logs directory");
        free_jobs(jobs, n_jobs);
        return 1;
    }

    /* 5. Initialize shared data structures */
    queue_init(&ready_queue);
    queue_init(&completed_queue);
    jobs_remaining = n_jobs;

    /* 6. Seed ready queue: jobs without any dependencies go in immediately */
    for (int i = 0; i < n_jobs; i++) {
        if (jobs[i].n_deps == 0) {
            queue_push(&ready_queue, i);
        }
    }

    /* 7. Start worker threads */
    pthread_t *workers = malloc(num_workers * sizeof(pthread_t));
    if (workers == NULL) {
        fprintf(stderr, "Error: memory allocation failed for worker threads array\n");
        queue_destroy(&ready_queue);
        queue_destroy(&completed_queue);
        free_jobs(jobs, n_jobs);
        return 1;
    }

    for (int i = 0; i < num_workers; i++) {
        if (pthread_create(&workers[i], NULL, worker_loop, NULL) != 0) {
            fprintf(stderr, "Error: failed to create worker thread %d\n", i);
            /* Shut down queue to signal started threads, join them, then clean up */
            queue_shutdown(&ready_queue);
            for (int j = 0; j < i; j++) {
                pthread_join(workers[j], NULL);
            }
            free(workers);
            queue_destroy(&ready_queue);
            queue_destroy(&completed_queue);
            free_jobs(jobs, n_jobs);
            return 1;
        }
    }

    /* 8. Start scheduler thread */
    pthread_t scheduler_thread;
    if (pthread_create(&scheduler_thread, NULL, scheduler_loop, NULL) != 0) {
        fprintf(stderr, "Error: failed to create scheduler thread\n");
        queue_shutdown(&ready_queue);
        for (int i = 0; i < num_workers; i++) {
            pthread_join(workers[i], NULL);
        }
        free(workers);
        queue_destroy(&ready_queue);
        queue_destroy(&completed_queue);
        free_jobs(jobs, n_jobs);
        return 1;
    }

    /* 9. Wait for scheduler to complete */
    pthread_join(scheduler_thread, NULL);

    /* 10. Wait for workers to complete (scheduler calls queue_shutdown before exiting) */
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }
    free(workers);

    /* 11. Generate pipeline report (thread-safe, all threads have joined) */
    int failed = generate_report(jobs, n_jobs);

    /* 12. Cleanup resource usage */
    queue_destroy(&ready_queue);
    queue_destroy(&completed_queue);
    free_jobs(jobs, n_jobs);

    return failed ? 1 : 0;
}
