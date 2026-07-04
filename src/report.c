#include "report.h"
#include <stdio.h>

int generate_report(Job *jobs, int n_jobs) {
    if (jobs == NULL) {
        return 0;
    }

    int success_count = 0;
    int failed_count = 0;
    int skipped_count = 0;
    int pending_count = 0;
    int running_count = 0;

    printf("\n=== MiniCI Pipeline Report ===\n");
    for (int i = 0; i < n_jobs; i++) {
        const char *status_str = "UNKNOWN";
        switch (jobs[i].status) {
            case PENDING:
                status_str = "PENDING";
                pending_count++;
                break;
            case RUNNING:
                status_str = "RUNNING";
                running_count++;
                break;
            case SUCCESS:
                status_str = "SUCCESS";
                success_count++;
                break;
            case FAILED:
                status_str = "FAILED";
                failed_count++;
                break;
            case SKIPPED:
                status_str = "SKIPPED";
                skipped_count++;
                break;
        }
        printf("%-10s %s\n", jobs[i].name, status_str);
    }

    printf("\nTotal: %d | Success: %d | Failed: %d | Skipped: %d", 
           n_jobs, success_count, failed_count, skipped_count);
    if (pending_count > 0 || running_count > 0) {
        printf(" | Pending: %d | Running: %d", pending_count, running_count);
    }
    printf("\n");

    return failed_count;
}
